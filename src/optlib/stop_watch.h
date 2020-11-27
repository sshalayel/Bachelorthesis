#ifndef STOP_WATCH_H
#define STOP_WATCH_H

#include <cassert>
#include <chrono>

/// Helper class that measures wallclock time from construction up to a call of elapsed().
struct stop_watch
{
    using time_point =
      std::chrono::time_point<std::chrono::high_resolution_clock>;
    time_point start;

    enum stop_watch_options
    {
        SET_TO_ZERO,
        CONTINUE,
    };

    stop_watch();
    double elapsed(stop_watch_options reset = CONTINUE);
};

#endif
