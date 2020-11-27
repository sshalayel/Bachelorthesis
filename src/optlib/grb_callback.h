#ifndef GRB_CALLBACK_H
#define GRB_CALLBACK_H

#include "bound_helper.h"
#include "coordinates.h"
#include "cut_chooser.h"
#include "cut_helper.h"
#include "grb_cut_chooser.h"
#include "grb_linear_expression.h"
#include "gurobi_c++.h"
#include "linear_cut_chooser.h"
#include "randomisation.h"
#include "stop_watch.h"
#include <algorithm>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

/// Enum that controls the behaviour of the callback.
enum slave_callback_options : int
{
    RANDOMISE = 1 << 0,
    ROUNDING_DOWN = 1 << 1,
    LAZY_TANGENTS = 1 << 2,
};

/// Helper struct that can increase or decrease the rate at which something happens (e.g. randomisation).
class rate
{
  public:
    rate(double every_n_times, double factor);
    /// Number of calls after which rate is reset to 0.
    double max_value;
    /// Used to decide how fast the rate should be growing or shrinking for successful/unsuccessful randomisations.
    double factor;
    /// Adapt max_value when randomisation is successful.
    void on_success();
    /// Adapt max_value when randomisation is unsuccessful.
    void on_failure();
    /// Increases current_value and returns if randomisation should be tried out.
    bool attempt();

  protected:
    /// Number of calls before next
    unsigned current_value;
};

grb_resettable_callback*
grb_no_op_callback();

/// A Gurobi callback that randomises solutions, round them down, generates cuts and puts (non)optimal MIP-solutions in the pool.
template<typename GRBAsyncSlave>
class grb_slave_async_cb : public grb_to_file
{
  public:
    grb_slave_async_cb(GRBAsyncSlave& self,
                       std::ostream& os,
                       std::mutex& os_mutex,
                       slave_cut_options slave_cuts,
                       std::string prefix_for_output,
                       slave_callback_options options);

    virtual ~grb_slave_async_cb() override;
    void reset() override;

    /// The slave using this callback.
    GRBAsyncSlave& self;
    /// Lock the output to avoid making it unreadable.
    std::mutex& os_mutex;

    ///Needed for grb_cut_chooser
    GRBEnv env;

    /// Contains the values of the binary variables in the model, refresh with fetch_values.
    symmetric_arr<proxy_arr<double>> values;
    /// Contains the values of the diameter variables in the model, refresh with fetch_values.
    symmetric_arr_2d<arr, double> diameter_values;
    /// Contains the values of the squared variables in the model as a tof, refresh with fetch_values.
    arr_1d<proxy_arr, double> squared_values;
    /// Contains the values of the x-representant, refresh with fetch_values.
    double representant_x;
    /// Contains the values of the x-representant, refresh with fetch_values.
    double negative_representant_x;

    /// Does all the randomisation stuff in a separate class (without templates).
    randomisation randomiser;

    /// Needed when dumping stats to disk.
    std::string prefix_for_output;

    /// Contains the improved objective obtained by some randomisation. Stops cuts until it is taken as an incumbent.
    std::optional<double> randomisation_in_progress = std::nullopt;
    std::optional<slave_callback_options> randomisation_in_progress_reason =
      std::nullopt;
    stop_watch incumbent_apparition_time;

    /// Use a randomized solution every n callback call.
    rate randomisation_rate{ 10.0, 1.4 };

    /// Use a tangent cut every n callback call.
    rate tangent_rate{ 10.0, 1.4 };

    /// Use a rounded down solution every n callback call.
    rate rounding_down_rate{ 10.0, 1.4 };

    /// Use a cut every n callback call.
    rate cut_rate{ 5.0, 1.5 };

    /// Use a randomized solution every n callback call.
    unsigned calls_since_last_randomization = 0;

    /// Used to return results asynchronously and to add cuts for relaxed-solutions.
    virtual void callback() override;
    /// Returns the biggest integer that is less then the root of square.
    static unsigned get_lowest_integer_root(double square);

    /// Used during randomisation.
    time_of_flight randomized{ 0, 0, {} };

    /// Used during cut-generation.
    std::unique_ptr<cut_chooser> chooser;

    /// Enables or disables cuts.
    slave_cut_options slave_cuts;

    /// Used during cut-generation.
    cut_helper ch;

    /// This enum decides which values to fetch in fetch_values, multiple options can be combined via |.
    enum values : int
    {
        BINARY = 1,
        DIAMETER = 1 << 1,
        SQUARED = 1 << 2,
        RELAXATION = 1 << 3,
        REPRESENTANTS = 1 << 4,
    };
    /// Fetches values from the model, cf. the enum called values.
    void fetch_values(int values);

    /// Adds the cuts into the model, false if current solution is feasible.
    void add_cuts(cut_statistics& added_cuts,
                  std::optional<std::reference_wrapper<time_of_flight>>
                    override_current_result,
                  slave_cut_options slave_cuts);

    /// Adds the tangent inequality as lazy constraint.
    bool add_lazy_tangent(cut_statistics& added_cuts, bool relaxation);

    /// Uses the randomisation class to generate a randomised solution from relaxation values and gives it to Gurobi with GRBCallback::setSolution(...).
    void set_randomized_solution();
    /// Generate a heuristic solution from relaxation values by rounding the d_{ij} down and gives it to Gurobi with GRBCallback::setSolution(...).
    void set_rounded_down_solution();

    slave_callback_options options;
};

template<typename GRBAsyncSlave>
grb_slave_async_cb<GRBAsyncSlave>::grb_slave_async_cb(
  GRBAsyncSlave& self,
  std::ostream& os,
  std::mutex& os_mutex,
  slave_cut_options slave_cuts,
  std::string prefix_for_output,
  slave_callback_options options)
  : grb_to_file(os)
  , self(self)
  , os_mutex(os_mutex)
  , values(0, 0, 0)
  , diameter_values(0, 0)
  , squared_values(0)
  , randomiser(values, values, self.mapping(), self.element_pitch_in_tacts)
  , prefix_for_output(prefix_for_output)
  , slave_cuts(slave_cuts)
  , ch(self.element_pitch_in_tacts, self.mapping())
  , options(options)
{}

template<typename GRBAsyncSlave>
grb_slave_async_cb<GRBAsyncSlave>::~grb_slave_async_cb()
{}

template<typename GRBAsyncSlave>
void
grb_slave_async_cb<GRBAsyncSlave>::reset()
{
    randomisation_in_progress = std::nullopt;
}

template<typename GRBAsyncSlave>
void
grb_slave_async_cb<GRBAsyncSlave>::fetch_values(int v)
{
    const bool relaxed = v & RELAXATION;

    const unsigned elements = self.vars_proxy.dim2;
    const unsigned samples = self.vars_proxy.dim3;

    if (v & REPRESENTANTS) {
        representant_x = relaxed ? getNodeRel(self.representant_x)
                                 : getSolution(self.representant_x);
    }

    if (v & BINARY) {
        values.realloca(
          elements,
          elements,
          samples,
          relaxed ? getNodeRel(self.vars_proxy.data, self.vars_proxy.size())
                  : getSolution(self.vars_proxy.data, self.vars_proxy.size()),
          true);
    }

    if (v & DIAMETER) {
        diameter_values.realloca(
          elements,
          elements,
          relaxed ? getNodeRel(self.diameter_vars_proxy.data,
                               self.diameter_vars_proxy.size())
                  : getSolution(self.diameter_vars_proxy.data,
                                self.diameter_vars_proxy.size()),
          true);
    }
    if (v & SQUARED) {
        squared_values.realloca(elements,
                                relaxed
                                  ? getNodeRel(self.squared_vars_proxy.data,
                                               self.squared_vars_proxy.size())
                                  : getSolution(self.squared_vars_proxy.data,
                                                self.squared_vars_proxy.size()),
                                true);
    }
}

template<typename GRBAsyncSlave>
void
grb_slave_async_cb<GRBAsyncSlave>::set_rounded_down_solution()
{
    fetch_values(DIAMETER | RELAXATION);
    for (unsigned i = 0; i < diameter_values.dim1; i++) {
        for (unsigned j = 0; j < diameter_values.dim2; j++) {
            const double rounded_down = std::floor(diameter_values(i, j));
            setSolution(self.diameter_vars_proxy(i, j), rounded_down);
        }
    }

    double improved_objective = useSolution();
    if (improved_objective == GRB_INFINITY) {
        rounding_down_rate.on_failure();
        os << " ==== (CALLBACK) ==== A rounded_down solution was found but "
              "not accepted by gurobi!"
           << std::endl;
    } else {
        rounding_down_rate.on_success();
        os << " ==== (CALLBACK) ==== A rounded_down solution with improved "
              "objective "
           << improved_objective << " (compared to current best "
           << getDoubleInfo(GRB_CB_MIPNODE_OBJBST)
           << ") was accepted by gurobi!" << std::endl;
        randomisation_in_progress = improved_objective;
        randomisation_in_progress_reason =
          slave_callback_options::ROUNDING_DOWN;
        incumbent_apparition_time.elapsed(stop_watch::SET_TO_ZERO);
    }
}

template<typename GRBAsyncSlave>
void
grb_slave_async_cb<GRBAsyncSlave>::set_randomized_solution()
{
    fetch_values(BINARY | RELAXATION);

    randomized.realloca(values.dim1, values.dim2);

    randomiser.update(values, self.vars_objective);
    if (!randomiser.randomise(randomized)) {
        os << " ==== (CALLBACK) ==== Randomised a solution outside of ROI!"
           << std::endl;
        return;
    }

    for (unsigned i = 0; i < randomized.dim1; i++) {
        const unsigned idx =
          self.revert_distance_mapping(randomized.at(i, i)).value();
        setSolution(self.vars_proxy(i, i, idx), 1.0);
    }

    double improved_objective = useSolution();
    if (improved_objective == GRB_INFINITY) {
        randomisation_rate.on_failure();
        os << " ==== (CALLBACK) ==== A randomised solution was found but "
              "not accepted by gurobi!"
           << std::endl;
    } else {
        randomisation_rate.on_success();
        os << " ==== (CALLBACK) ==== A randomised solution with improved "
              "objective "
           << improved_objective << " (compared to current best "
           << getDoubleInfo(GRB_CB_MIPNODE_OBJBST)
           << ") was accepted by gurobi (after "
           << getDoubleInfo(GRB_CB_RUNTIME) << " seconds of computing)!"
           << std::endl;
        randomisation_in_progress = improved_objective;
        randomisation_in_progress_reason = slave_callback_options::RANDOMISE;
        incumbent_apparition_time.elapsed(stop_watch::SET_TO_ZERO);
    }
}

/// Used for debugging : compares 2 cut chooser (grb_cut_chooser and linear_cut_chooser) and prints something if they do not compute the same thing.
struct compare_cut_chooser : cut_chooser
{
    compare_cut_chooser(GRBEnv& e, std::string file);

    /// The first cut_chooser to compare.
    grb_cut_chooser gcc;
    /// The second cut_chooser to compare.
    linear_cut_chooser lcc;

    /// The prefix of the location of the output.
    std::string prefix_for_output;

    /// Synchronisation primitive that only will be added in c++20.
    struct barrier
    {
        /// Threads waiting at barrier.
        unsigned capacity;
        /// Threads needed until barrier opens.
        const unsigned initial_capacity;
        /// State of the barrier : threads can enter (until the barrier is full) or leave (until the barrier is empty).
        enum status
        {
            ENTERING,
            LEAVING
        } status = ENTERING;
        /// Locks internal state.
        std::mutex capacity_mutex;
        /// For threads waiting to enter when status is LEAVING.
        std::condition_variable entering_cv;
        /// For threads waiting to leave when status is ENTERING.
        std::condition_variable leaving_cv;

        barrier(unsigned capacity);
        /// Wait until initial_capacity threads entered and left before opening.
        void sync();
    };

    std::vector<double> gcc_times;
    std::vector<double> lcc_times;

    void print_stat(std::string filename);
    unsigned choose_from(accumulated_cuts& ac,
                         slave_problem::relaxation_values v,
                         callback add_cut) override;
};

template<typename GRBAsyncSlave>
bool
grb_slave_async_cb<GRBAsyncSlave>::add_lazy_tangent(cut_statistics& added_cuts,
                                                    bool relaxation)
{
    stop_watch sw;
    fetch_values(REPRESENTANTS | BINARY | DIAMETER | SQUARED |
                 (relaxation ? RELAXATION : 0));

    grb_linear_expression_visitor visitor{ self.variables() };
    unsigned current_tangents = added_cuts.tangent;
    ch.create_tangent_cuts(
      added_cuts,
      { representant_x, values, diameter_values, squared_values },
      [&](linear_expression_constraint c) { return addLazy(visitor(c)); });

    if (added_cuts.tangent > current_tangents) {
        tangent_rate.on_success();
        os << " ==== (LAZY-TANGENTS) Added "
           << added_cuts.tangent - current_tangents << " lazy constraints in "
           << sw.elapsed() << " seconds. ====" << std::endl;
        return true;
    } else {
        os << " ==== (LAZY-TANGENTS-FAIL) Added no tangents ";
        tangent_rate.on_failure();
        return false;
    }
}

template<typename GRBAsyncSlave>
void
grb_slave_async_cb<GRBAsyncSlave>::add_cuts(
  cut_statistics& added_cuts,
  std::optional<std::reference_wrapper<time_of_flight>> override_current_result,
  slave_cut_options slave_cuts)
{
    stop_watch sw;

    fetch_values(REPRESENTANTS | BINARY | DIAMETER | SQUARED | RELAXATION);
    slave_problem::distance_mapping m = self.mapping();

    assert_that(m.to_distance, "Slave-mapping is invalid!");
    assert_that(m.to_index, "Slave-mapping is invalid!");

    std::unique_ptr<cut_chooser> chooser;

    if (!chooser) {
        switch (slave_cuts) {
            case slave_cut_options::ILP_CHOOSER: {
                chooser =
                  std::unique_ptr<cut_chooser>(new grb_cut_chooser{ env });
                break;
            }
            case slave_cut_options::GREEDY_CHOOSER: {
                chooser =
                  std::unique_ptr<cut_chooser>(new linear_cut_chooser());
                break;
            }
            case slave_cut_options::COMPARE_BOTH_CHOOSER: {
                chooser = std::unique_ptr<cut_chooser>(
                  new compare_cut_chooser{ env, prefix_for_output });
                break;
            }
            default:
                assert(false);
        }
    }
    grb_linear_expression_visitor visitor{ self.variables() };

    ch.create_cuts(
      added_cuts,
      { representant_x, values, std::nullopt, squared_values },
      [&](linear_expression_constraint c) { return addCut(visitor(c)); },
      *chooser);

    if (added_cuts.total() > 0) {
        cut_rate.on_success();
        os << " ==== (CUTS) Added " << added_cuts.total() << " cuts in "
           << sw.elapsed() << " seconds. ====" << std::endl;
    } else {
        cut_rate.on_failure();
        os << " ==== (CUTS) Added no cuts in " << sw.elapsed()
           << " seconds. ====" << std::endl;
    }
}

template<typename GRBAsyncSlave>
void
grb_slave_async_cb<GRBAsyncSlave>::callback()
{
    try {
        if (where == GRB_CB_MESSAGE) { // Lock when writing output.
            std::unique_lock l(os_mutex);
            //stop_watch sw;
            grb_to_file::callback();
            //std::cout << "Printed Gurobi Message in " << sw.elapsed()
            //<< "seconds!" << std::endl;
        } else if (where == GRB_CB_MIPSOL) {
            //stop_watch sw;
            // MIP solution callback
            const double obj = getDoubleInfo(GRB_CB_MIPSOL_OBJ);
            if (obj > self.threshold) {
                cut_statistics total_added_cuts;
                if ((options & slave_callback_options::LAZY_TANGENTS) &&
                    add_lazy_tangent(total_added_cuts, false)) {
                    if (randomisation_in_progress) {
                        randomisation_in_progress.reset();
                        // fail 2 times to undo the on_success() that was executed before.
                        if (randomisation_in_progress_reason.value() ==
                            slave_callback_options::RANDOMISE) {
                            randomisation_rate.on_failure();
                            randomisation_rate.on_failure();
                        } else {
                            assert_that(
                              randomisation_in_progress_reason.value() ==
                                slave_callback_options::ROUNDING_DOWN,
                              "Invalid heuristic?!");
                            rounding_down_rate.on_failure();
                            rounding_down_rate.on_failure();
                        }
                    }

                    std::unique_lock l(os_mutex);
                    os
                      << " ==== (LAZY-TANGENTS) Ignored GRB_CB_MIPSOL-solution "
                         "with objective "
                      << obj
                      << " as it was cutted by lazy tangent! ====" << std::endl;

                    fetch_values(REPRESENTANTS | BINARY | DIAMETER | SQUARED);
                    time_of_flight tof(
                      values.dim1, values.dim2, representant_x);

                    self.fill_tof(std::ref(values),
                                  std::ref(diameter_values),
                                  std::ref(squared_values),
                                  representant_x,
                                  tof,
                                  false);
                    for (unsigned i = 0; i < values.dim1; i++) {
                        if (diameter_values(i, i) >=
                            std::sqrt(squared_values(i))) {
                            os << " ==== (LAZY-TANGENTS) ==== d_" << i
                               << " vs sqrt(q_" << i << ") " << std::setw(10)
                               << diameter_values(i, i) << std::setw(10)
                               << std::sqrt(squared_values(i)) << std::endl;
                        }
                        //os << "d_" << i << ", " << i << " was "
                        //   << diameter_values(i, i) //<< ", q_" << i << " was "
                        //                            // << squared_values(i)
                        //   << ", sqrt(q_" << i << ") was "
                        //   << std::sqrt(squared_values(i)) << " and x was "
                        //   << representant_x << std::endl;
                    }
                    os << tof << std::endl << std::endl;

                } else {
                    fetch_values(REPRESENTANTS | BINARY | DIAMETER | SQUARED);
                    time_of_flight tof(
                      values.dim1, values.dim2, representant_x);

                    self.fill_tof(std::ref(values),
                                  std::ref(diameter_values),
                                  std::ref(squared_values),
                                  representant_x,
                                  tof);
                    self.solution_callback(
                      { std::move(tof),
                        {
                          obj,
                          getDoubleInfo(GRB_CB_RUNTIME),
                          getDoubleInfo(GRB_CB_MIPSOL_OBJBST),
                          getDoubleInfo(GRB_CB_MIPSOL_OBJBND),
                          getDoubleInfo(GRB_CB_MIPSOL_NODCNT),
                          getIntInfo(GRB_CB_MIPSOL_SOLCNT),
                        } });
                    //std::cout << "Got an (approximate) MIP-solution in "
                    //<< sw.elapsed() << "seconds!" << std::endl;
                }
            }
        } else if (where == GRB_CB_MIPNODE &&
                   getIntInfo(GRB_CB_MIPNODE_STATUS) == GRB_OPTIMAL) {
            //stop_watch sw;
            ///check if some randomisation/rounded down value is waiting to become accepted as gurobi incumbent.
            const double obj = getDoubleInfo(GRB_CB_MIPNODE_OBJBST);
            if (randomisation_in_progress && *randomisation_in_progress > obj) {
                std::unique_lock l(os_mutex);
                //wait until randomised value gets accepted as incumbent.
                os << "==== (CB) Waiting for incumbent " << obj
                   << " to become greater than " << *randomisation_in_progress
                   << " (elapsed time: " << incumbent_apparition_time.elapsed()
                   << " seconds) ====" << std::endl;
                return;
            }

            cut_statistics total_added_cuts;
            if ((options & slave_callback_options::RANDOMISE) &&
                randomisation_rate.attempt()) {
                set_randomized_solution();
            } else if ((options & slave_callback_options::ROUNDING_DOWN) &&
                       rounding_down_rate.attempt()) {
                set_rounded_down_solution();
            } else {
                // if no heuristic generated : create cuts!
                if ((options & slave_callback_options::LAZY_TANGENTS) &&
                    tangent_rate.attempt()) {
                    add_lazy_tangent(total_added_cuts, true);
                }
                if (slave_cuts != slave_cut_options::OFF &&
                    cut_rate.attempt()) {
                    add_cuts(total_added_cuts, std::nullopt, slave_cuts);
                }
            }
            //std::cout << "Working over a fractional MIP-solution during " << sw.elapsed()
            //<< "seconds!" << std::endl;
        }
    } catch (GRBException& e) {
        std::cerr << "Exception in Callback!" << std::endl;
        std::cerr << e.getErrorCode() << ": " << e.getMessage() << std::endl;
    }
}

#endif // GRB_CALLBACK_H
