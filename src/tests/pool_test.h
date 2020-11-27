#include "../optlib/constraint_pool.h"
#include <cxxtest/TestSuite.h>
#include <list>
#include <numeric>

class pool_test : public CxxTest::TestSuite
{
  public:
    void test_list_checker()
    {
        std::list<int> l(10);
        std::iota(l.begin(), l.end(), 0);

        unsigned counter = 0;

        list_predicate_checker checker(l, [&](int i) {
            ++counter;
            return i == 42;
        });

        TS_ASSERT(!checker.check());
        l.push_back(11);
        TS_ASSERT(!checker.check());
        size_t before = l.size();
        l.push_back(42);
        TS_ASSERT(checker.check());
        TS_ASSERT_EQUALS(l.size(), before);

        TS_ASSERT_EQUALS(counter, l.size() + 1);
        unsigned old_counter = counter;
        l.push_back(42);
        l.push_back(42);
        l.push_back(42);
        TS_ASSERT(checker.check());
        TS_ASSERT_EQUALS(l.size(), before);
        TS_ASSERT_EQUALS(counter, old_counter + 3);
    }
};
