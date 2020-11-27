
#include "stop_watch.h"

stop_watch::stop_watch()
  : start(std::chrono::high_resolution_clock::now())
{}

double
stop_watch::elapsed(stop_watch_options opt)
{
    time_point now = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff = now - start;
    switch (opt) {
        case SET_TO_ZERO:
            start = now;
            break;
        case CONTINUE:
            break;
        default:
            assert(false);
    }
    return diff.count();
}
