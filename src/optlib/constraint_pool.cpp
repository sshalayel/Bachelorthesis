
#include "constraint_pool.h"

constraint_pool::constraint_pool() {}

void
constraint_pool::add(column&& c, unsigned slave_id)
{
    std::unique_lock l{ pool_mutex };
    pool.push_back({ std::move(c), slave_id });
    actual_unconsumed++;

    // notify one because there is only one master/consument
    pool_not_empty.notify_one();
}

void
constraint_pool::set_current_slave_id(unsigned slave_id)
{
    std::unique_lock l{ pool_mutex };
    this->slave_id = slave_id;
}

void
constraint_pool::get_statistics(unsigned& total_consumed,
                                unsigned& consumed_from_old_slave,
                                unsigned& actual_unconsumed)
{
    std::unique_lock l{ pool_mutex };
    total_consumed = this->total_consumed;
    consumed_from_old_slave = this->consumed_from_old_slave;
    actual_unconsumed = this->actual_unconsumed;
}
