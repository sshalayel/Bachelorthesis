#include "linear_cut_chooser.h"

linear_cut_chooser::constraint_part::constraint_part(unsigned index)
  : index(index)
{}

std::weak_ptr<linear_cut_chooser::constraint_part>
linear_cut_chooser::constraint_part::next_z()
{
    // last element from a z_list
    if (neighbours.empty()) {
        return std::weak_ptr<linear_cut_chooser::constraint_part>();
    }
    assert_that(neighbours.size() == 1, "Only call this method on z!");
    auto ret = neighbours.back().z;
    assert_that(ret.lock(), "Only call this method on z!");
    return ret;
}

std::optional<double>
linear_cut_chooser::constraint_part::all_z_f_in_intersection_non_choosen(
  slave_problem::relaxation_values values,
  unsigned element,
  neighbour other)
{
    double ret = 0;
    std::weak_ptr<constraint_part> current_z = other.z;
    for (unsigned i = 0; i < other.z_count; i++) {
        switch (current_z.lock()->taken) {
            case constraint_part::NOT_CHOOSEN: {
                return {};
            }
            case constraint_part::CHOOSEN: {
                /// dont add this to ret, as its cost is already covered by somebody else
                break;
            }
            case constraint_part::NOT_DECIDED_YET: {
                ret += values.binary(element, element, current_z.lock()->index);
                break;
            }
            default:
                assert(false);
        }
        current_z = current_z.lock()->next_z();
    }
    return ret;
}

void
linear_cut_chooser::constraint_part::choose_all_z_f_in_intersection(
  constraint_part::neighbour other)
{
    std::weak_ptr<constraint_part> current_z = other.z;
    for (unsigned i = 0; i < other.z_count; i++) {
        current_z.lock()->taken = constraint_part::CHOOSEN;
        current_z = current_z.lock()->next_z();
    }
}

using bk = linear_cut_chooser::bookkeeping;

void
bk::insert_without_duplicates_from(indexes& indexes)
{
    assert_that(indexes.K.size() == indexes.L.size(), "Invalid Input!");
    assert_that(indexes.L.size() == indexes.F.size(), "Invalid Input!");

    xs.reserve(indexes.K.size());
    ys.reserve(indexes.K.size());
    zs.reserve(2.0 * indexes.K.size()); // approximation of number of z-entries.

    for (unsigned i = 0; i < indexes.K.size(); i++) {
        if (std::get<bool>(
              k_indexes_to_positions.insert({ indexes.K[i], xs.size() }))) {
            xs.push_back(std::make_shared<constraint_part>(indexes.K[i]));
        }
        if (std::get<bool>(
              l_indexes_to_positions.insert({ indexes.L[i], ys.size() }))) {
            ys.push_back(std::make_shared<constraint_part>(indexes.L[i]));
        }
        assert_that(!indexes.F[i].empty(), "Invalid Input : no z!");

        std::weak_ptr<constraint_part> last;
        std::weak_ptr<constraint_part> new_last;
        for (unsigned f : indexes.F[i]) {
            auto entry = f_indexes_to_positions.insert({ f, zs.size() });

            if (!std::get<bool>(entry)) {
                // check if already added
                // entry is a pair<iterator, bool>, but dereferencing iterator returns a pair <key, value>
                new_last = zs[std::get<1>(*std::get<0>(entry))];
            } else {
                // add it
                zs.push_back(std::make_shared<constraint_part>(f));
                new_last = zs.back();
            }
            if (auto p = last.lock()) {
                if (p->neighbours.empty() && new_last.lock()) {
                    p->neighbours.push_back({ {}, new_last, 1 });
                }
            }
            last = new_last;
            new_last.reset();
        }
    }
}

void
bk::set_left_to_smallest(std::weak_ptr<constraint_part>& a,
                         std::weak_ptr<constraint_part> b)
{
    // no a => b smaller
    if (!a.lock()) {
        a = b;
        return;
    }
    // no b : nothing to do
    if (!b.lock()) {
        return;
    }
    // a bigger then b : swap
    if (a.lock().get() > b.lock().get()) {
        a = b;
    }
}

void
bk::set_left_to_greatest(unsigned& a, unsigned b)
{
    a = std::max(a, b);
}

void
bk::add_connections_from(indexes& indexes)
{
    for (unsigned i = 0; i < indexes.K.size(); i++) {
        const unsigned x_pos = k_indexes_to_positions[indexes.K[i]];
        const unsigned y_pos = l_indexes_to_positions[indexes.L[i]];

        const unsigned z_pos = f_indexes_to_positions[indexes.F[i].front()];

        constraint_part::neighbour n{
            ys[y_pos],
            zs[z_pos],
            (unsigned)indexes.F[i].size(),
        };

        xs[x_pos]->neighbours.push_back(n);
        n.x_or_y = xs[x_pos];
        ys[y_pos]->neighbours.push_back(n);
    }
}

void
bk::sort(slave_problem::relaxation_values v, current_elements& current_elements)
{
    std::sort(
      xs.begin(),
      xs.end(),
      [&](std::weak_ptr<constraint_part> a, std::weak_ptr<constraint_part> b) {
          return sort_helper(v, current_elements.j, a, b);
      });
    std::sort(
      ys.begin(),
      ys.end(),
      [&](std::weak_ptr<constraint_part> a, std::weak_ptr<constraint_part> b) {
          return sort_helper(v, current_elements.h, a, b);
      });
    std::sort(
      zs.begin(),
      zs.end(),
      [&](std::weak_ptr<constraint_part> a, std::weak_ptr<constraint_part> b) {
          return sort_helper(v, current_elements.i, a, b);
      });
}

bk::bookkeeping(current_elements& current_elements,
                indexes& indexes,
                slave_problem::relaxation_values v)
  : relaxation_values(v)
  , current_indexes(indexes)
  , elements(current_elements)
{
    insert_without_duplicates_from(indexes);

    add_connections_from(indexes);

    sort(v, current_elements);
}

std::optional<bk::pointer>&
bk::get_greatest_pointer()
{
    std::reference_wrapper<std::optional<pointer>> best = x ? x : (y ? y : z);
    if (!best.get()) {
        return best.get();
    }
    if (y && relaxation_value_of(*y) > relaxation_value_of(*best.get())) {
        best = y;
    }
    if (z && relaxation_value_of(*z) > relaxation_value_of(*best.get())) {
        best = z;
    }
    return best.get();
}

void
bk::advance(std::optional<bk::pointer>& p)
{
    assert_that(p, "Cannot advance a pointer that already reached the end!");

    if (p) {
        p->position++;
        unsigned end;
        switch (p->name) {
            case pointer::X: {
                end = xs.size();
                break;
            }
            case pointer::Y: {
                end = ys.size();
                break;
            }
            case pointer::Z: {
                end = zs.size();
                break;
            }
        }
        if (p->position >= end) {
            p.reset();
        }
    }
}

double
bk::relaxation_value_of(pointer p)
{
    unsigned element;
    unsigned index;
    switch (p.name) {
        case pointer::X: {
            element = elements.j;
            index = xs[p.position]->index;
            break;
        }
        case pointer::Y: {
            element = elements.h;
            index = ys[p.position]->index;
            break;
        }
        case pointer::Z: {
            element = elements.i;
            index = zs[p.position]->index;
            break;
        }
        default:
            assert(false);
    }
    return relaxation_values.binary(element, element, index);
}

bool
bk::sort_helper(slave_problem::relaxation_values v,
                unsigned e,
                std::weak_ptr<constraint_part> a,
                std::weak_ptr<constraint_part> b)
{
    const double a_val = v.binary(e, e, a.lock()->index);
    const double b_val = v.binary(e, e, b.lock()->index);
    return a_val > b_val;
}

bool
linear_cut_chooser::choose_from(
  current_elements current_elements,
  indexes indexes,
  slave_problem::relaxation_values relaxation_values,
  callback add_cut)
{
    bookkeeping bk(current_elements, indexes, relaxation_values);
    while (auto& pointer = bk.get_greatest_pointer()) {
        switch (pointer->name) {
            case bookkeeping::pointer::X:
            case bookkeeping::pointer::Y: {
                std::shared_ptr<constraint_part> current =
                  pointer->name == bookkeeping::pointer::X
                    ? bk.xs[pointer->position]
                    : bk.ys[pointer->position];
                assert_that(current->neighbours.size() > 0,
                            "Cannot have a variable that does not participate "
                            "in any constraint!");
                // case one : no neighbour is already selected and i have better relaxation then all to_be_decided neighbours
                // case two : neighbour y_l selected BUT all z_f (corresponding to constraints with y_l and x_k) are selected.
                bool can_select_current = true;

                for (auto neighbour : current->neighbours) {
                    switch (neighbour.x_or_y.lock()->taken) {
                            // case one
                        case constraint_part::CHOOSEN: {
                            // if all z_f are non-choosen (case two) and their relaxation_value is smaller then the pointers : choose them all.
                            std::optional<double> relaxation_value_of_all_z_f =
                              current->all_z_f_in_intersection_non_choosen(
                                bk.relaxation_values, bk.elements.i, neighbour);
                            const bool can_be_choosen =
                              relaxation_value_of_all_z_f &&
                              bk.relaxation_value_of(*pointer) >
                                *relaxation_value_of_all_z_f;
                            if (can_be_choosen) {
                                current->choose_all_z_f_in_intersection(
                                  neighbour);
                            } else {
                                can_select_current = false;
                            }
                            break;
                        }
                        case constraint_part::NOT_DECIDED_YET:
                        case constraint_part::NOT_CHOOSEN: {
                            /// nothing to do here
                            break;
                        }
                    }
                    if (!can_select_current) {
                        break;
                    }
                }
                //write result
                current->taken = can_select_current
                                   ? constraint_part::CHOOSEN
                                   : constraint_part::NOT_CHOOSEN;
                break;
            }
            case bookkeeping::pointer::Z: {
                constraint_part::status& current =
                  bk.zs[pointer->position]->taken;
                if (current == constraint_part::NOT_DECIDED_YET) {
                    current = constraint_part::NOT_CHOOSEN;
                }
                break;
            }
        }
        bk.advance(pointer);
    }
    //iterate over bk.xs,ys, zs and generate the cut.
    linear_expression lhs = lef::create_constant(0.0);
    linear_expression rhs = lef::create_constant(1.0);

    double lhs_value = bk.collect(bk.xs, current_elements.j, lhs) +
                       bk.collect(bk.ys, current_elements.h, lhs);
    double rhs_value = bk.collect(bk.zs, current_elements.i, rhs);

    linear_expression_constraint lec = lhs <= rhs;

    if (intermediate_results) {
        intermediate_results(lhs_value - rhs_value, lec);
    }

    if (lhs_value > rhs_value + 1.0) {
        add_cut(lec);
        return true;
    }
    return false;
}

double
bk::collect(std::vector<std::shared_ptr<constraint_part>>& cps,
            unsigned element,
            linear_expression& append_here)
{
    double ret = 0.0;
    for (auto cp : cps) {
        assert_that(cp->taken != constraint_part::NOT_DECIDED_YET,
                    "Algorithm didn't run until end?!");
        if (cp->taken == constraint_part::CHOOSEN) {
            append_here +=
              lef::create_binary_variable(element, element, cp->index);
            ret += relaxation_values.binary(element, element, cp->index);
        }
    }
    return ret;
}

unsigned
linear_cut_chooser::choose_from(accumulated_cuts& ac,
                                slave_problem::relaxation_values v,
                                callback add_cut)
{
    unsigned added_cuts = 0;
    for (unsigned i = 0; i < ac.dim1; i++) {
        for (unsigned j = i + 1; j < ac.dim2; j++) {
            for (unsigned k = j + 1; k < ac.dim3; k++) {
                if (!ac(i, j, k).empty()) {
                    if (choose_from({ i, j, k }, ac(i, j, k), v, add_cut)) {
                        added_cuts++;
                    }
                }
            }
        }
    }
    return added_cuts;
}
