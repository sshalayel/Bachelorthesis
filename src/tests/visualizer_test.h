#include "../optlib/grb_column_generation.h"
#include "../optlib/iterator.h"
#include "../optlib/reader.h"
#include "../optlib/visualizer.h"
#include <cxxtest/TestSuite.h>
#include <sstream>

class visualizer_test : public CxxTest::TestSuite
{
  private:
    void tof_helper(unsigned diagonal_entries[],
                    unsigned diagonal_length,
                    time_of_flight& tof)
    {

        tof.realloca(1, diagonal_length, diagonal_length);

        for (unsigned i = 0; i < diagonal_length; i++) {
            for (unsigned j = 0; j < diagonal_length; j++) {
                tof.at(i, j) = (diagonal_entries[i] + diagonal_entries[j]) / 2;
            }
        }
    }

    void helper(unsigned (*f)(unsigned i, unsigned j))
    {
        config c;
        c.x_position = 0.0;
        c.pitch = 1.2e-3;
        c.sampling_rate = 20e6;
        c.wave_speed = 6350;
        c.elements = 4;
        c.wave_length = 0.00127;
        c.samples = 124;
        c.reference_samples = 64;
        c.verbose = true;
        c.slavestop = -GRB_INFINITY;
        c.output = "";

        arr<> dummy(0, 0, 0, nullptr);
        arr<> dummy2(0, 0, 0, nullptr);
        grb_cg cg(c, dummy, dummy2); //verbose

        time_of_flight tof(4, 4, {});
        for (unsigned i = 0; i < tof.senders; i++) {
            for (unsigned j = 0; j < tof.receivers; j++) {
                tof.at(i, j) = f(i, j);
            }
        }
        std::vector<time_of_flight> tofvec;
        tofvec.push_back(std::move(tof));
        std::vector<double> amp;
        amp.push_back(42.0);

        std::stringstream ss;
        cg.dump(ss, amp, container_input_iterator{ tofvec });

        //std::cout << ss.str();

        visualizer v(4);
        v.load(ss, -1);

        //make sure the loaded values are all correctly loaded
        TS_ASSERT(v.loaded);
        TS_ASSERT_EQUALS(v.c.x_position, 0.0);
        TS_ASSERT_EQUALS(v.c.pitch, 1.2e-3);
        TS_ASSERT_EQUALS(v.c.sampling_rate, 20e6);
        TS_ASSERT_EQUALS(v.c.wave_speed, 6350);
        TS_ASSERT_EQUALS(v.c.elements, 4);
        TS_ASSERT_EQUALS(v.c.wave_length, 0.00127);
        TS_ASSERT_EQUALS(v.c.samples, 124);
        TS_ASSERT_EQUALS(v.c.reference_samples, 64);
        TS_ASSERT_EQUALS(v.tofs.size(), 1);
        TS_ASSERT_EQUALS(v.tofs[0].at(2, 2), f(2, 2));
        TS_ASSERT_EQUALS(v.tofs[0].at(3, 3), f(3, 3));
        TS_ASSERT_EQUALS(v.tofs[0].at(0, 3), f(0, 3));
        TS_ASSERT_EQUALS(v.tofs[0].at(3, 0), f(3, 0));
        TS_ASSERT_EQUALS(v.vals.size(), 1);
        TS_ASSERT_EQUALS(v.vals[0], 1.0); // 42 gets normalized to 1.0
    }

  public:
    //taken from one cgdump
    constexpr static const unsigned data[] = {
        303, 302, 301, 301, 301, 300, 300, 300, 300, 300, 300, 300, 301, 301,
        302, 302, 302, 301, 300, 300, 300, 299, 299, 299, 299, 299, 299, 300,
        300, 300, 301, 302, 301, 300, 300, 299, 299, 299, 299, 298, 298, 299,
        299, 299, 299, 300, 300, 301, 301, 300, 299, 299, 299, 298, 298, 298,
        298, 298, 298, 299, 299, 299, 300, 301, 301, 300, 299, 299, 299, 298,
        298, 298, 298, 298, 298, 298, 299, 299, 300, 300, 300, 299, 299, 298,
        298, 298, 297, 297, 297, 297, 298, 298, 298, 299, 299, 300, 300, 299,
        299, 298, 298, 297, 297, 297, 297, 297, 297, 298, 298, 299, 299, 300,
        300, 299, 298, 298, 298, 297, 297, 297, 297, 297, 297, 298, 298, 298,
        299, 300, 300, 299, 298, 298, 298, 297, 297, 297, 297, 297, 297, 298,
        298, 298, 299, 300, 300, 299, 299, 298, 298, 297, 297, 297, 297, 297,
        297, 298, 298, 299, 299, 300, 300, 299, 299, 298, 298, 298, 297, 297,
        297, 297, 298, 298, 298, 299, 299, 300, 300, 300, 299, 299, 298, 298,
        298, 298, 298, 298, 298, 298, 299, 299, 300, 300, 301, 300, 299, 299,
        299, 298, 298, 298, 298, 298, 298, 299, 299, 299, 300, 301, 301, 300,
        300, 299, 299, 299, 299, 298, 298, 299, 299, 299, 299, 300, 300, 301,
        302, 301, 300, 300, 300, 299, 299, 299, 299, 299, 299, 300, 300, 300,
        301, 301, 302, 302, 301, 301, 300, 300, 300, 300, 300, 300, 300, 300,
        301, 301, 301, 302
    };
    static const size_t data_size = sizeof(data) / sizeof(unsigned);

  public:
    void test_simple()
    {
        helper([](unsigned i, unsigned j) { return i * 42 + j; });
    }

    void test_tact_coords_to_tof()
    {
        std::stringstream ss;
        ss << "#probe_x_pos;element_pitch;sampling_rate;wave_"
              "celerity;virtual_elements;elements;wave_length;"
              "measurement_samples;reference_signal_samples\n"
              "0;1;1;1;4;4;0.2;400;3;\n";

        visualizer v{ 1 };
        v.load(ss, -1);

        const unsigned elements = 4;

        time_of_flight tof(elements, elements, {});
        v.c.tact_coords_to_tof(0.0, 0.0, tof);

        for (unsigned i = 0; i < tof.senders; i++) {
            for (unsigned j = 0; j < tof.receivers; j++) {
                TS_ASSERT_EQUALS(tof.at(i, j), i + j);
            }
        }

        v.c.tact_coords_to_tof(-5.0, 0.0, tof);

        for (unsigned i = 0; i < tof.senders; i++) {
            for (unsigned j = 0; j < tof.receivers; j++) {
                TS_ASSERT_EQUALS(tof.at(i, j), 10.0 + i + j);
            }
        }

        v.c.tact_coords_to_tof(5.0, 0.0, tof);

        for (unsigned i = 0; i < tof.senders; i++) {
            for (unsigned j = 0; j < tof.receivers; j++) {
                TS_ASSERT_EQUALS(tof.at(i, j), 5 - i + 5 - j);
            }
        }

        v.c.tact_coords_to_tof(5.0, 5.0, tof);

        for (unsigned i = 0; i < tof.senders; i++) {
            for (unsigned j = 0; j < tof.receivers; j++) {
                const double x1 = 5 - i;
                const double x2 = 5 - j;
                const double y = 5.0;
                TS_ASSERT_EQUALS(tof.at(i, j),
                                 std::floor(std::sqrt(x1 * x1 + y * y) +
                                            std::sqrt(x2 * x2 + y * y)));
            }
        }
    }

    /// Testcase from 240.239.238.236.simplexoid.fail.ggb, going from left base {0,1} to the representative one.
    // Here, the 3. constraint is not feasible for {0, 1} (while 0,1 and 2 are feasible).
    //void test_next_basis()
    //{
    //    using basis = basis;

    //    const basis a = { 0, true, 1, true };
    //    simplexoid si{ 7.56, 1.0 };

    //    time_of_flight tof(0, 0, nullptr, false, {});
    //    unsigned data[] = { 240, 239, 238, 236 };
    //    tof_helper(data, sizeof(data) / sizeof(unsigned), tof);
    //    si.tof = &tof;

    //    basis current = a;
    //    TS_ASSERT(si.next_basis(3, 3, current));
    //    TS_ASSERT_EQUALS(current, (basis{ 1, true, 3, true }));
    //    TS_ASSERT(si.next_basis(2, 3, current));
    //    TS_ASSERT_EQUALS(current, (basis{ 1, true, 2, true }));
    //    TS_ASSERT(si.next_basis(3, 3, current));
    //    TS_ASSERT_EQUALS(current, (basis{ 2, true, 3, false }));

    //    //the same thing again but with next_feasible_basis

    //    current = a;
    //    TS_ASSERT(si.next_feasible_basis(4, current));
    //    TS_ASSERT_EQUALS(current, (basis{ 2, true, 3, false }));

    //    TS_ASSERT_DELTA(
    //      si.representant_distance(basis{ 2, true, 3, false }, 0), 240.7, 0.5);
    //    TS_ASSERT_DELTA(
    //      si.representant_distance(basis{ 2, true, 3, false }, 1), 239.2, 0.5);
    //    TS_ASSERT_DELTA(
    //      si.representant_distance(basis{ 2, true, 3, false }, 2), 238, 0.5);
    //    TS_ASSERT_DELTA(
    //      si.representant_distance(basis{ 2, true, 3, false }, 3), 237, 0.5);
    //}

    void test_bfs_cross()
    {
        const unsigned image_size = 50;

        std::deque<pixel> q;
        q.push_back({ 25, 25, 0 });
        q.push_back({ 23, 27, 1 });
        q.push_back({ 25, 4, 2 });
        q.push_back({ 24, 42, 3 });

        BFS bfs{ q,
                 [](pixel p) {
                     return p.x >= 0 && p.y >= 0 && p.x < image_size &&
                            p.y < image_size &&
                            (2 * p.x == image_size || 2 * p.y == image_size);
                 },
                 (unsigned)q.size() };

        bfs.run();

        TS_ASSERT_EQUALS(bfs.colored_pixels[0].size(), image_size * 2 - 1);
        TS_ASSERT_EQUALS(bfs.colored_pixels[1].size(),
                         1); // the added pixel is not checked and colored
        TS_ASSERT_EQUALS(bfs.colored_pixels[2].size(), image_size * 2 - 1);
        TS_ASSERT_EQUALS(bfs.colored_pixels[3].size(),
                         image_size *
                           2); // the added pixel is not checked and colored
    }

    void test_coords()
    {
        visualizer v{ 1 };

        std::stringstream ss;
        ss << "#probe_x_pos;element_pitch;sampling_rate;wave_"
              "celerity;virtual_elements;elements;wave_length;"
              "measurement_samples;reference_signal_samples\n"
              "0;1;1;1;4;4;0.2;400;3;\n";

        double x, y;
        unsigned i, j;
        v.load(ss, 0.1);
        v.compute(500, 500, false);
        v.c.pixel_to_tact_coords(500, 500, 50, 30, x, y);
        v.c.tact_coords_to_pixel(500, 500, x, y, i, j);

        TS_ASSERT_EQUALS(i, 50);
        TS_ASSERT_EQUALS(j, 30);
    }

    void test_simplexoid()
    {
        const double pitch = 2.0 * 1.2e-3 * 20e6 / 6350.0;
        simplexoid s{ pitch, 1.0 };

        time_of_flight tof(16, 16, {});
        std::copy(data, data + tof.size(), tof.data);

        basis b;
        s.run(tof, b);

        double x, y;

        TS_ASSERT(s.representant_to_coord((basis{ 1, false, 6, false }), x, y));

        TS_ASSERT_DELTA(x, 29.105, .1);
        TS_ASSERT_DELTA(y, 148.6, .1);

        visualizer v{ 1 };
        std::stringstream ss;
        ss << "#probe_x_pos;element_pitch;sampling_rate;wave_celerity;virtual_"
              "elements;elements;wave_length;measurement_samples;reference_"
              "signal_samples\n"
              "0.044;0.0012;20000000;6350;16;16;0.00127;656;64;";

        v.load(ss, 0.1);
        //TS_ASSERT_EQUALS(v.c.x_position, 0.044);
        TS_ASSERT_EQUALS(v.c.pitch, 0.0012);
        TS_ASSERT_EQUALS(v.c.sampling_rate, 20e6);
        TS_ASSERT_EQUALS(v.c.wave_speed, 6350);
        TS_ASSERT_EQUALS(v.c.elements, 16);
        TS_ASSERT_EQUALS(v.c.wave_length, 0.00127);
        TS_ASSERT_EQUALS(v.c.samples, 656);
        TS_ASSERT_EQUALS(v.c.reference_samples, 64);

        TS_ASSERT(v.reflector_in_tact_coords(tof, x, y));
    }

    // 4 * 4 pixels, reflector with tof = 1 (one sender/receiver-pair), pixelheight = 3/8, pixelwidth = 3/16
    // | 0 | R | 0 | R |
    // | 0 | R | 0 | R |
    // | 0 | 0 | R | 0 |
    // | 0 | 0 | 0 | 0 |
    void test_reflector_in_pixel()
    {
        const unsigned width = 4;
        const unsigned height = 4;

        bool pattern[width][height] = {
            {
              0,
              0,
              0,
              0,
            },
            {
              1,
              0,
              0,
              0,
            },
            {
              0,
              1,
              1,
              0,
            },
            {
              1,
              0,
              0,
              0,
            },
        };

        std::stringstream ss("#probe_x_pos;element_pitch;sampling_rate;wave_"
                             "celerity;virtual_elements;elements;wave_length;"
                             "measurement_samples;reference_signal_samples\n"
                             "0;0;1;1;1;1;0.2;3;3;\n"
                             "1;1;\n");

        visualizer v(1);
        v.load(ss, -1);

        unsigned problematic_reflectors = v.compute(width, height);
        TS_ASSERT_EQUALS(0, problematic_reflectors);
        if (problematic_reflectors > 0) {
            return;
        }

        /// One pixel (given by simplexoid) is accepted even if its placed on one of its neighbours.
        bool found_seed = false;

        for (unsigned i = 0; i < width; i++) {
            for (unsigned j = 0; j < height; j++) {
                if (pattern[i][j]) {
                    TS_ASSERT_DIFFERS(0.0, v.intensities.at(i, j));
                } else {
                    bool wrong = v.intensities.at(i, j) == 0.0;
                    if (wrong) {
                        if (found_seed) {
                            TS_ASSERT_EQUALS(0.0, v.intensities.at(i, j));
                        } else {
                            found_seed = true;
                        }
                    }
                }
            }
        }

        {
            double x, y;
            unsigned i, j;
            time_of_flight tof(v.c.elements, v.c.elements, {});

            i = 1, j = 0;
            v.c.pixel_to_tact_coords(width, height, i, j, x, y);
            v.c.tact_coords_to_tof(x, y, tof);
            TS_ASSERT(v.reflector_in_tact_coords(tof, x, y));
        }

        {
            time_of_flight& tof = v.tofs[0];
            double x, y;

            for (unsigned i = 0; i < width; i++) {
                for (unsigned j = 0; j < height; j++) {
                    std::stringstream ss;
                    ss << pattern[i][j] << " (" << i << ", " << j << ")";
                    v.pixel_to_coords(i, j, x, y);
                    TSM_ASSERT_EQUALS(ss.str(),
                                      v.reflector_in_tact_coords(tof, x, y),
                                      pattern[i][j]);
                }
            }
        }
    }

    void test_union_find()
    {
        const unsigned size = 10;
        proxy_arr<UnionFindElement<pixel>> x(1, size, size);
        for (unsigned i = 0; i < size; i++) {
            for (unsigned j = 0; j < size; j++) {
                x(0, i, j).data = { i, j };
            }
        }

        x(0, 1, 1).unite(x(0, 1, 0));

        TS_ASSERT(&x(0, 1, 1).parent->get() == &x(0, 1, 0) ||
                  &x(0, 1, 0).parent->get() == &x(0, 1, 1));

        //test unions
        for (unsigned i = 1; i < size; i++) {
            x(0, 0, i).unite(x(0, 0, 0));

            TS_ASSERT_DIFFERS(x(0, 0, i).parent, std::nullopt);
        }

        //test finds
        for (unsigned i = 0; i < size; i++) {
            TSM_ASSERT_EQUALS(std::to_string(x(0, 0, i).find().get().data->x) +
                                ", " +
                                std::to_string(x(0, 0, i).find().get().data->y),
                              &(x(0, 0, i).find().get()),
                              &(x(0, 0, 0)));
        }

        //test parents
        for (unsigned i = 1; i < size; i++) {
            TS_ASSERT_EQUALS(&x(0, 0, i).parent->get(), &(x(0, 0, 0)));
        }
    }
};
