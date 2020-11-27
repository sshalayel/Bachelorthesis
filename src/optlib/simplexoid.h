#ifndef SIMPLEXOID_H
#define SIMPLEXOID_H

#include "coordinates.h"
#include <cmath>
#include <functional>
#include <set>

/// Represents a point defined by 2 cutting arcs for some time-of-flight.
struct basis
{
    /// The circles defining the basis.
    unsigned b0, b1;
    /// Using the lower or the upper circle.
    bool lower0, lower1;

    ///Creates an basis with uninitialised entries .
    basis() {}
    basis(unsigned b0, bool lower0, unsigned b1, bool lower1)
    {
        if (b0 < b1) {
            this->b0 = b0;
            this->lower0 = lower0;
            this->b1 = b1;
            this->lower1 = lower1;
        } else {
            this->b0 = b1;
            this->lower0 = lower1;
            this->b1 = b0;
            this->lower1 = lower0;
        }
    }

    /// Can be used to compare 2 basis together.
    bool operator==(const basis b) const
    {
        return b0 == b.b0 && b1 == b.b1 && lower0 == b.lower0 &&
               lower1 == b.lower1;
    }

    /// Can be used to compare 2 basis together.
    bool operator<(const basis b) const
    {
        if (b0 != b.b0) {
            return b0 < b.b0;
        }
        if (b1 != b.b1) {
            return b1 < b.b1;
        }
        if (lower0 != b.lower0) {
            return lower0 < b.lower0;
        }
        if (lower1 != b.lower1) {
            return lower1 < b.lower1;
        }

        return false;
    }

    /// Make sure that the assumption of a basis is holding.
    bool valid() const { return b0 < b1; }

    /// Dumps this into stream for debugging purposes.
    std::ostream& dump(std::ostream& os);
};

std::ostream&
operator<<(std::ostream& os, basis b);

/// The logic for the simplex-like algorithm that chooses 2 representants for one time-of-flight.
class simplexoid
{
  public:
    enum direction
    {
        LEFT,
        RIGHT
    };

    simplexoid(double double_element_pitch_in_tacts, double overlap);

    double double_element_pitch_in_tacts;
    const time_of_flight* tof = nullptr;

    double overlap;

    /// The start of the algorithm. b0 and b1 will be initialised to 0 and 1. Returns false for incorrect tofs.
    bool run(const time_of_flight& tof, basis& b);
    /// Helper function for the length of the third upper side of the triangle spanned by 2 representants a and b.
    double upper_side(basis b);
    /// Distance between the point defined by representants (b0, b1) and a potential new representant.
    double representant_distance(basis b, unsigned representant);
    /// Computes the cosinus of the angle spanned between the representant, the 0th element and the 1th element.
    double cosinus_at_0th_element(basis b);
    /// Computes the cosinus of the angle spanned between the representant, the 0th element and the 1th element. Returns false when computation failed (cos is bigger then 1 or smaller then -1).
    bool cosinus_at_0th_element(basis b, double& cos);
    ///Makes sure that the tof is valid for basis b (That is, constraints of b cut themselves).
    bool current_tof_valid_for(basis b);

    /// Transforms 2 representants into one pixel. Assumes that coordinate (0,0) is the element at b0 and returns a results in tacts.
    bool representant_to_coord(basis b, double& x_coord, double& y_coord);

    /// Goes to the next (nearest to b) basis that is feasible for infeasible_n.
    bool next_basis(unsigned infeasible_n, unsigned until_n, basis& b);

    /// Goes to next basis that is left/right and feasible for all constraints < n. Changes b.b0 for right and b.b1 for left.
    bool next_feasible_basis(unsigned n, basis& b);

    /// Checks if basis (b0,b1) is feasible for the first n entries of the current tof. Returns n when feasible and one broken constraint if infeasible in the range [0, n[.
    void feasible_n(basis b, unsigned n, unsigned& broken, bool& lower_broken);

    /// Like feasible_n but only counts the number of broken constraints for one basis.
    unsigned broken_n(basis b, unsigned n);

    /// A relative position of some arc to some basis.
    enum feasibility
    {
        FEASIBLE,
        INFEASIBLE_ABOVE,
        INFEASIBLE_BELOW,
    };

    /// Returns the position of the arc given by constraint c relative to the basis b.
    feasibility feasibility_for_c(basis b, unsigned c);

    /// The diameter of the arc given by the first basis entry.
    double b0_diameter(basis b);

    /// The diameter of the arc given by the second basis entry.
    double b1_diameter(basis b);

    /// Used by b0 and b1_diameter.
    double diameter(basis b, bool b0);
};
#endif
