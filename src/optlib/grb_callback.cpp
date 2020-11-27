#include "grb_callback.h"

rate::rate(double every_n_times, double factor)
  : max_value(every_n_times)
  , factor(factor)
  , current_value(0)
{}

void
rate::on_success()
{
    max_value /= factor;
}

void
rate::on_failure()
{
    max_value *= factor;
};

bool
rate::attempt()
{
    if (current_value++ > max_value) {
        current_value = 0;
        return true;
    } else {
        return false;
    }
}

grb_resettable_callback*
grb_no_op_callback()
{
    return nullptr;
}

compare_cut_chooser::compare_cut_chooser(GRBEnv& e,
                                         std::string prefix_for_output)
  : gcc(e)
  , prefix_for_output(prefix_for_output)
{}

compare_cut_chooser::barrier::barrier(unsigned capacity)
  : capacity(0)
  , initial_capacity(capacity)
{}

void
compare_cut_chooser::barrier::sync()
{
    std::unique_lock l(capacity_mutex);
    /// Wait until barrier is open and places are free.
    entering_cv.wait(
      l, [&]() { return status == ENTERING && capacity < initial_capacity; });

    capacity++;

    /// Wait until barrier is full.
    leaving_cv.wait(
      l, [&]() { return status == LEAVING || capacity == initial_capacity; });
    status = LEAVING;
    leaving_cv.notify_all();

    assert_that(capacity > 0, "Leaving without entering?");
    capacity--;

    if (capacity == 0) {
        status = ENTERING;
        entering_cv.notify_all();
    }
}

void
compare_cut_chooser::print_stat(std::string filename)
{
    assert_that(lcc_times.size() == gcc_times.size(),
                "Both should have the same size!");
    std::ofstream of(filename, std::iostream::app);
    for (unsigned i = 0; i < lcc_times.size(); i++) {
        of << lcc_times[i] << ";" << gcc_times[i] << ";\n";
    }
    lcc_times.clear();
    gcc_times.clear();
}

unsigned
compare_cut_chooser::choose_from(accumulated_cuts& ac,
                                 slave_problem::relaxation_values v,
                                 callback add_cut)
{
    barrier barrier{ 2 };
    double objective_gcc = -1.0;
    double objective_lcc = -2.0;
    linear_expression_constraint gcc_cut;
    linear_expression_constraint lcc_cut;
    stop_watch gcc_sw;
    stop_watch lcc_sw;
    bool already_printed = false;
    // All generated cuts.
    int total_count = 0;
    // All cuts where linear_cut_chooser and grb_cut_chooser didnt return the same thing.
    int missed_count = 0;
    // All cuts where linear_cut_chooser didnt generate the cut but grb_cut_chooser did.
    int disappearing_count = 0;
    std::mutex output;
    auto intermediate_results_callback =
      [&](double& out,
          linear_expression_constraint& lec_out,
          stop_watch& swe,
          std::vector<double>& times) {
          return [&](double objective, linear_expression_constraint lec) {
              {
                  std::unique_lock l(output);
                  out = objective;
                  lec_out = lec;
                  already_printed = false;
                  times.push_back(swe.elapsed(stop_watch::SET_TO_ZERO));
              }

              barrier.sync();
              {
                  std::unique_lock l(output);
                  bool correct =
                    std::fabs(objective_gcc - objective_lcc) < 0.001;
                  if (!already_printed) {
                      already_printed = true;
                      if (objective_gcc > 1.0) {
                          total_count++;
                          if (objective_lcc <= 1.0) {
                              disappearing_count++;
                          }
                      }
                      if (!correct) {
                          missed_count++;
                          //std::cerr << objective_gcc << " VS " << objective_lcc
                          //<< std::endl;

                          ///gcc_cut.dump(std::cerr);
                          ///std::cerr << " VS " << std::endl;
                          ///lcc_cut.dump(std::cerr);
                          ///std::cerr << std::endl << std::endl;
                          //assert(correct);
                      }
                  }
              }
              barrier.sync();
              {
                  std::unique_lock l(output);
                  swe.elapsed(stop_watch::SET_TO_ZERO);
                  already_printed = false;
              }
          };
      };

    // avoid adding cuts two times and dataraces.
    auto do_not_add_cuts = [](linear_expression_constraint lec) {};

    gcc.intermediate_results =
      intermediate_results_callback(objective_gcc, gcc_cut, gcc_sw, gcc_times);
    lcc.intermediate_results =
      intermediate_results_callback(objective_lcc, lcc_cut, lcc_sw, lcc_times);

    std::thread t([&] { return gcc.choose_from(ac, v, do_not_add_cuts); });
    unsigned result = lcc.choose_from(ac, v, add_cut);
    t.join();

    print_stat(prefix_for_output + ".cutstats");

    std::cerr << "Disappearing/Inequal objectives/Total :" << disappearing_count
              << "/" << missed_count << "/" << total_count << std::endl;

    return result;
}
