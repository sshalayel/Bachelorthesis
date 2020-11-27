#ifndef SLAVE_OUTPUT_SETTINGS_H
#define SLAVE_OUTPUT_SETTINGS_H

#include <chrono>
#include <condition_variable>
#include <mutex>

/// Needed to control how often the separate Printing thread for the slaves output can print.
struct slave_output_settings
{
    slave_output_settings(std::chrono::milliseconds printing_interval);

    /// Represents when the slave is allowed to print, set from outside.
    bool can_print = false;
    /// Corresponding lock, given from outside.
    std::mutex mutex;
    /// Corresponding condition, given from outside.
    std::condition_variable condition_variable;
    /// How often the slave should print when the master waits for an slave solution, given from outside.
    std::chrono::milliseconds printing_interval;
};

#endif
