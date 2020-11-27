#include "../optlib/arr.h"
#include "../optlib/bound_helper.h"
#include "../optlib/config.h"
#include "../optlib/cut_helper.h"
#include "../optlib/linear_expression.h"
#include "../optlib/statistics.h"
#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <numeric>
#include <optional>
#include <string>

class linear_expression_test : public CxxTest::TestSuite
{
  public:
    void test_bound_helper()
    {
        bound_helper<unsigned> bounds{ 14.2848 };
        bound_helper<unsigned>::variable i{ 0, 402 };
        bound_helper<unsigned>::variable j{ 4, 406 };
        bound_helper<unsigned>::variable h{ 11, 417 };

        double lower, upper;
        double squared_lower, squared_upper;
        auto test_combination = [&](bound_helper<unsigned>::variable i,
                                    bound_helper<unsigned>::variable j,
                                    bound_helper<unsigned>::variable h,
                                    std::string message) {
            bounds.get_corrected<double>(
              i.idx, j, h, { squared_lower, squared_upper });
            lower = std::floor(std::sqrt(squared_lower));
            upper = std::floor(std::sqrt(squared_upper));
            TSM_ASSERT_LESS_THAN_EQUALS(message.c_str(), lower, i.diameter);
            TSM_ASSERT_LESS_THAN_EQUALS(message.c_str(), i.diameter, upper);
        };

        // h 3. position
        test_combination(i, j, h, "i--j--h");
        test_combination(j, i, h, "j--i--h");
        test_combination(i, h, j, "i--h--j");
        test_combination(j, h, i, "j--h--i");
        test_combination(h, i, j, "h--i--j");
        test_combination(h, j, i, "h--j--i");
    }

    void test_linear_expressions()
    {
        linear_expression c = linear_expression_factory::create_constant(3.14);
        linear_expression v =
          linear_expression_factory::create_binary_variable(1, 2, 3);

        {
            linear_expression sum = c + v;
            linear_expression z = linear_expression_factory::create_constant(0);
            linear_expression sum2 = z;
            sum2 += c + v;
            TS_ASSERT_EQUALS(c->coefficient, 3.14);
            TS_ASSERT_EQUALS(std::get<variable>(v->v).i, 1);
            TS_ASSERT_EQUALS(std::get<variable>(v->v).j, 2);
            TS_ASSERT_EQUALS(std::get<variable>(v->v).k, 3);

            TS_ASSERT_THROWS_NOTHING(std::get<constant>(std::get<binary_linear_expression>(sum->v).lhs->v));
            TS_ASSERT_THROWS_NOTHING(std::get<variable>(std::get<binary_linear_expression>(sum->v).rhs->v));
            TS_ASSERT_THROWS_NOTHING(std::get<binary_linear_expression>(std::get<binary_linear_expression>(sum2->v).rhs->v));
        }
        TS_ASSERT_EQUALS(std::get<variable>(v->v).i, 1);
        TS_ASSERT_EQUALS(std::get<variable>(v->v).j, 2);
        TS_ASSERT_EQUALS(std::get<variable>(v->v).k, 3);
    }
};
