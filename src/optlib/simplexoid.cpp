#include "simplexoid.h"

std::ostream&
basis::dump(std::ostream& os)
{
    os << "(" << b0 << (lower0 ? "-" : "+") << ", " << b1
       << (lower1 ? "-" : "+") << ")";
    return os;
}

std::ostream&
operator<<(std::ostream& os, basis b)
{
    return b.dump(os);
}

simplexoid::simplexoid(double pitch, double overlap)
  : double_element_pitch_in_tacts(pitch)
  , overlap(overlap)
{}

double
simplexoid::upper_side(basis b)
{
    return std::fabs((double)b.b0 - b.b1) * double_element_pitch_in_tacts;
}

double
simplexoid::representant_distance(basis basis, unsigned representant)
{
    assert(basis.valid() && tof && tof->senders > representant);

    if (basis.b0 == representant) {
        return b0_diameter(basis);
    }
    if (basis.b1 == representant) {
        return b1_diameter(basis);
    }

    const double upper = upper_side(basis);
    const double right = b1_diameter(basis);
    const double left = b0_diameter(basis);

    //three cases
    //representant is left or middle
    if (representant < basis.b1) {
        const double cos = coordinate_tools::cosinus_gamma(upper, right, left);

        const double big_upper = upper_side({ representant,
                                              true /* this bool is ignored */,
                                              basis.b1,
                                              basis.lower1 });
        const double squared_ret =
          coordinate_tools::squared_length_of_c(big_upper, right, cos);
        const double ret = std::sqrt(squared_ret);

        return ret;
    } else { // representant is right
        double cos = coordinate_tools::cosinus_gamma(upper, left, right);

        const double big_upper =
          upper_side({ basis.b0,
                       basis.lower0,
                       representant,
                       true /* this bool is ignored */ });

        const double squared_ret =
          coordinate_tools::squared_length_of_c(big_upper, left, cos);
        const double ret = std::sqrt(squared_ret);

        return ret;
    }
}

unsigned
simplexoid::broken_n(basis b, unsigned n)
{
    unsigned ret = 0;
    for (unsigned j = 0; j < n; j++) {
        if (feasibility_for_c(b, j) != FEASIBLE) {
            ret++;
        }
    }
    return ret;
}

simplexoid::feasibility
simplexoid::feasibility_for_c(basis b, unsigned j)
{
    const double j_theory = representant_distance(b, j);

    const double current_lower = tof->at(j, j) - overlap;
    const double current_upper = tof->at(j, j) + overlap;

    if (!(current_upper >= j_theory)) {
        return INFEASIBLE_ABOVE;
    } else if (!(current_lower <= j_theory)) {
        return INFEASIBLE_BELOW;
    } else {
        return FEASIBLE;
    }
}

void
simplexoid::feasible_n(basis b,
                       unsigned n,
                       unsigned& broken,
                       bool& lower_broken)
{
    for (unsigned j = 0; j < n; j++) {
        switch (feasibility_for_c(b, j)) {
            case INFEASIBLE_ABOVE:
                broken = j;
                lower_broken = false;
                return;
            case INFEASIBLE_BELOW:
                broken = j;
                lower_broken = true;
                return;
            case FEASIBLE:
                continue;
            default:
                assert(false);
        }
    }
    broken = n;
}

bool
simplexoid::run(const time_of_flight& tof, basis& b)
{
    assert(tof.senders > 0 && "Invalid time_of_flight!");

    this->tof = &tof;

    if (tof.senders == 1) {
        b = { 0, true, 0, true };
        return true;
    }

    b = { 0, true, 1, true };

    for (unsigned i = 2; i < tof.senders; i++) {
        unsigned n = i + 1;

        // change basis if not feasible
        if (!next_feasible_basis(n, b)) {
            return false;
        }
    }
    return true;
}

bool
simplexoid::current_tof_valid_for(basis b)
{
    const double left = b0_diameter(b);
    const double upper = upper_side(b);
    const double right = b1_diameter(b);
    const double cos = coordinate_tools::cosinus_gamma(left, upper, right);
    const bool ret = -1 <= cos && cos <= 1;
    //std::cout << b;
    //if (ret) {
    //    std::cout << " passed cos-test with cos = " << cos << std::endl;
    //} else {
    //    std::cout << " failed cos-test with cos = " << cos << std::endl;
    //}
    return ret;
}

bool
simplexoid::cosinus_at_0th_element(basis b, double& cos_gamma)
{
    assert(tof && b.valid());

    if (b.b0 == 0) {
        cos_gamma = coordinate_tools::cosinus_gamma(
          b0_diameter(b), upper_side(b), b1_diameter(b));

    } else {
        // cosinus of the angle spanned between the representant, the b.b0th element and the b.b1th element.
        double cos = coordinate_tools::cosinus_gamma(
          b0_diameter(b), upper_side(b), b1_diameter(b));

        basis from_0th_to_b0{ 0, true, b.b0, b.lower0 };

        // cos (pi - x) = -cos(x)
        // distance from representant to 0th element.
        double x = coordinate_tools::squared_length_of_c(
          upper_side(from_0th_to_b0), b0_diameter(b), -cos);

        cos_gamma = coordinate_tools::cosinus_gamma(
          upper_side(from_0th_to_b0), std::sqrt(x), b0_diameter(b));
    }

    return cos_gamma <= 1 && cos_gamma >= -1;
}

double
simplexoid::cosinus_at_0th_element(basis b)
{
    double cos_gamma;
    assert(cosinus_at_0th_element(b, cos_gamma));
    return cos_gamma;
}

bool
simplexoid::next_basis(unsigned infeasible_n, unsigned until_n, basis& b)
{
    assert(tof);

    // 4 new possible basis, take the one that is most feasible
    const basis candidates[] = {
        { b.b0, b.lower0, infeasible_n, true },
        { b.b0, b.lower0, infeasible_n, false },
        { b.b1, b.lower1, infeasible_n, true },
        { b.b1, b.lower1, infeasible_n, false },
        { b.b0, !b.lower0, infeasible_n, true },
        { b.b0, !b.lower0, infeasible_n, false },
        { b.b1, !b.lower1, infeasible_n, true },
        { b.b1, !b.lower1, infeasible_n, false },
    };

    std::optional<unsigned> best;
    std::optional<unsigned> best_basis;

    for (unsigned i = 0; i < sizeof(candidates) / sizeof(basis); i++) {
        unsigned current = broken_n(candidates[i], until_n);
        //std::cout << "Trying out candidate " << candidates[i]
        //          << " with broken_n = " << current
        //          << " of a total of n = " << until_n << std::endl;
        if (current_tof_valid_for(candidates[i]) &&
            (!best || current < *best)) {
            best_basis = i;
            best = current;
            //    std::cout << "candidate " << candidates[i] << " became new best!"
            //              << std::endl;
        } else {
            //    std::cout << "candidate " << candidates[i] << " failed!"
            //              << std::endl;
        }
    }

    if (best_basis) {
        b = candidates[*best_basis];
        //std::cout << "New candidate is " << b << std::endl;
        return true;
    } else {
        //std::cout << "No new feasible basis found!" << std::endl;
        return false;
    }
}

bool
simplexoid::next_feasible_basis(unsigned n, basis& b)
{
    assert(tof && tof->senders >= n);

    bool ret = true;
    unsigned broken;
    bool lower_broken;

    std::set<basis> already_used_basis;

    do {
        feasible_n(b, n, broken, lower_broken);

        // asserts that : current tof is valid (cosinus-wise) and that this doesn't diverge for strange tofs.
        if (!already_used_basis.insert(b).second) {
            std::cerr << "Endless loop detected in simplexoid!" << std::endl;
            return false;
        }
    } while (broken != n && (ret = next_basis(broken, n, b)));

    return ret;
}

bool
simplexoid::representant_to_coord(basis b, double& x, double& y)
{
    if (b.b0 == b.b1) {
        assert(tof->senders == 1 && "Why are 2 representants not equal?");

        x = 0;
        y = b0_diameter(b) / 2.0;

        return true;
    }

    double cos;
    bool ret = cosinus_at_0th_element(b, cos);

    // else acos will return nan.
    if (!ret) {
        return false;
    }

    double gamma = std::acos(cos);
    constexpr unsigned _0th_element = 0;
    double side = representant_distance(b, _0th_element);

    x = cos * side / 2.0;
    y = std::sin(gamma) * side / 2.0;
    return true;
}

double
simplexoid::diameter(basis b, bool b0)
{
    bool lower = b0 ? b.lower0 : b.lower1;
    double value = b0 ? tof->at(b.b0, b.b0) : tof->at(b.b1, b.b1);
    double current_overlap = lower ? -0.5 : 0.5;
    return value + current_overlap;
}

double
simplexoid::b0_diameter(basis b)
{
    return diameter(b, true);
}

double
simplexoid::b1_diameter(basis b)
{
    return diameter(b, false);
}
