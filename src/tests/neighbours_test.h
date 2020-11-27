
#include "../optlib/neighbours.h"
#include "../optlib/visualizer.h"
#include <cxxtest/TestSuite.h>
#include <numeric>

class neighbours_test : public CxxTest::TestSuite
{
  private:
    static void print(tree<int>& t)
    {
        std::cout << "[" << t.address << "]"
                  << "(" << t.data << ") {\n";
        std::for_each(t.childrens.begin(), t.childrens.end(), print);
        std::cout << "}" << std::endl;
    }

    //TODO: fix neighbours if they are needed someday, make sure not to swap pitch and double_pitch!
//  public:
//    void test_tof_feasibility()
//    {
//        const unsigned elements = 16;
//
//        time_of_flight evil_tof = { elements, elements };
//
//        std::fill(evil_tof.begin(), evil_tof.end(), 300);
//        evil_tof.at(3, 3) = 310;
//
//        neighbours n{ 7.559 };
//
//        TS_ASSERT(!n.tof_feasible_in_range(
//          evil_tof.diagonal_begin(), evil_tof.diagonal_end(), elements / 2));
//
//        //diagonal taken from one cgdump
//        std::vector<unsigned> data = {
//            //621, 620, 620, 620, 619, 619, 619, 619,
//            //619, 619, 619, 620, 620, 620, 621, 622,
//            241, 239, 238, 236, 236, 235, 234, 234,
//            234, 234, 235, 235, 236, 238, 239, 241,
//        };
//
//        time_of_flight nice_tof(elements, elements);
//
//        std::copy(data.begin(), data.end(), nice_tof.diagonal_begin());
//
//        ////TS_ASSERT(n.tof_feasible_in_range(data.begin(), data.end(), elements));
//
//        unsigned count_neighbours = 0;
//        neighbours{ 7.559 }.all_in_circle(
//          nice_tof, 3, [&](tree<unsigned>& node) {
//              if (node.depth < elements) {
//                  return;
//              }
//              ++count_neighbours;
//              std::list<unsigned> data;
//              node.data_until_leaf(data);
//              //TODO
//              //for (unsigned i : data) {
//              //std::cout << i << ", ";
//              //}
//              //std::cout << "\n";
//          });
//        //        TS_FAIL("If we use a radius of 3 we get " +
//        //                std::to_string(count_neighbours) +
//        //                " neighbours. Looks like it doesnt scale at all.");
//    }
//
//    void test_tof_feasibility2()
//    {
//        const unsigned elements = 16;
//
//        time_of_flight evil_tof = { elements, elements };
//
//        std::fill(evil_tof.begin(), evil_tof.end(), 300);
//        evil_tof.at(3, 3) = 310;
//
//        neighbours n{ 7.559 };
//
//        TS_ASSERT(!n.tof_feasible_in_range(
//          evil_tof.diagonal_begin(), evil_tof.diagonal_end(), elements / 2));
//
//        //diagonal taken from one cgdump
//        std::vector<unsigned> data = {
//            //621, 620, 620, 620, 619, 619, 619, 619,
//            //619, 619, 619, 620, 620, 620, 621, 622,
//            241, 239, 238, 236, 236, 235, 234, 234,
//            234, 234, 235, 235, 236, 238, 239, 241,
//        };
//
//        time_of_flight nice_tof(elements, elements);
//
//        std::copy(data.begin(), data.end(), nice_tof.diagonal_begin());
//
//        ////TS_ASSERT(n.tof_feasible_in_range(data.begin(), data.end(), elements));
//
//        unsigned count_neighbours = 0;
//        neighbours{ 7.559 }.all_in_circle(
//          nice_tof, 1, [&](tree<unsigned>& node) {
//              if (node.depth < elements) {
//                  return;
//              }
//              ++count_neighbours;
//              std::list<unsigned> data;
//              node.data_until_leaf(data);
//              //TODO
//              //for (unsigned i : data) {
//              //std::cout << i << ", ";
//              //}
//              //std::cout << "\n";
//          });
//        TS_ASSERT_EQUALS(count_neighbours, elements * elements);
//    }
//
//    void test_tof_feasibility3()
//    {
//        const unsigned elements = 16;
//
//        time_of_flight evil_tof = { elements, elements };
//
//        std::fill(evil_tof.begin(), evil_tof.end(), 300);
//        evil_tof.at(3, 3) = 310;
//
//        neighbours n{ 7.559 };
//
//        TS_ASSERT(!n.tof_feasible_in_range(
//          evil_tof.diagonal_begin(), evil_tof.diagonal_end(), elements / 2));
//
//        //diagonal taken from one cgdump
//        std::vector<unsigned> data = {
//            //621, 620, 620, 620, 619, 619, 619, 619,
//            //619, 619, 619, 620, 620, 620, 621, 622,
//            241, 239, 238, 236, 236, 235, 234, 234,
//            234, 234, 235, 235, 236, 238, 239, 241,
//        };
//
//        time_of_flight nice_tof(elements, elements);
//
//        std::copy(data.begin(), data.end(), nice_tof.diagonal_begin());
//
//        unsigned count_neighbours = 0;
//        tree<unsigned> root;
//        builder b{ root,
//                   nice_tof.diagonal_begin(),
//                   nice_tof.diagonal_end(),
//                   [&](tree<unsigned>& node, unsigned current) {
//                       return true;
//                   } };
//        b.build(1);
//
//        std::ofstream f("./tex/plot_neighbours.tex");
//        tikz_tof_printer p(f, 0.02, 0.01, 1.2e-3, 6350 / 20e6, elements);
//        unsigned leafs = std::distance(root.leaf_begin(), root.leaf_end());
//        double current = 0;
//        color_palette palette;
//
//        std::for_each(
//          root.leaf_begin(), root.leaf_end(), [&](tree<unsigned>& node) {
//              std::list<unsigned> data;
//              node.data_until_leaf(data);
//              double current_color = current++ / leafs;
//
//              p.add_tof(data.begin(),
//                        data.end(),
//                        palette.red(current_color),
//                        palette.green(current_color),
//                        palette.blue(current_color),
//                        false);
//          });
//        TS_ASSERT_EQUALS(count_neighbours, elements * elements);
//    }
//
//    void test_tree_builder_iterator()
//    {
//        tree t{ 10 };
//        const unsigned length = 4;
//
//        std::vector<int> data(length, 50);
//
//        builder(t,
//                data.begin(),
//                data.end(),
//                [](tree<int>& t, int start) -> bool { return start % 4 == 0; })
//          .build(length);
//
//        unsigned count = 0;
//
//        for (tree<int>::iterator it = t.begin(); it != t.end(); ++it) {
//            count++;
//            TS_ASSERT((*it).data == 48 || (*it).data == 52);
//        }
//        TS_ASSERT_EQUALS(count, std::pow(2, length + 1) - 2);
//
//        count = 0;
//
//        for (tree<int>::iterator it = t.leaf_begin(); it != t.leaf_end();
//             ++it) {
//            count++;
//            TS_ASSERT((*it).data == 48 || (*it).data == 52);
//        }
//        TS_ASSERT_EQUALS(count, std::pow(2, length));
//
//        tree<int>& first = *t.leaf_begin();
//
//        std::list<int> should_be_48;
//        first.data_until_leaf(should_be_48);
//        TS_ASSERT_EQUALS(should_be_48.size(), 4);
//        for (int i : should_be_48) {
//            TS_ASSERT_EQUALS(i, 48);
//        }
//    }
};
