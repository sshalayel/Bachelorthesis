#ifndef TOF_BUILDER_H
#define TOF_BUILDER_H

#include "coordinates.h"

/// Used to construct a tof, makes sure that added entries respect the bounds of already present entries.
struct tof_builder
{
    /// Needed to compute bounds.
    const double squared_pitch;
    /// Needed to compute bounds.
    bound_helper<unsigned> bounds;
    /// The computed tof, will be moven away after building.
    time_of_flight tof;
    /// Bookkeeping of already set entries.
    arr_2d<arr, bool> setted_entries;

    /// Last used bounds (avoid recomputing bounds when using set directly after get_bounds_for).
    unsigned checked_i;
    /// Last used bounds (avoid recomputing bounds when using set directly after get_bounds_for).
    unsigned checked_j;
    /// Last used bounds (avoid recomputing bounds when using set directly after get_bounds_for).
    unsigned last_lower_bound;
    /// Last used bounds (avoid recomputing bounds when using set directly after get_bounds_for).
    unsigned last_upper_bound;

    /// Bounds dictated by already set entries.
    void get_bounds_for(unsigned i,
                        unsigned j,
                        unsigned& lower_bound,
                        unsigned& upper_bound);

    /// Sets an entry, returns true if successful.
    bool set(unsigned i, unsigned j, unsigned distance);
};

#endif
