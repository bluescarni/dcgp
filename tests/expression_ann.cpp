#include <random>
#define BOOST_TEST_MODULE dcgp_expression_ann_test
#include <algorithm>
#include <audi/back_compatibility.hpp>
#include <audi/io.hpp>
#include <boost/test/unit_test.hpp>
#include <stdexcept>

#include <dcgp/expression_ann.hpp>
#include <dcgp/kernel_set.hpp>
using namespace dcgp;

void test_against_numerical_derivatives(unsigned n, unsigned m, unsigned r, unsigned c, unsigned lb,
                                        std::vector<unsigned> arity, unsigned seed,
                                        expression_ann::loss_type loss_e)
{
    std::mt19937 gen(seed);
    // Random distributions
    std::normal_distribution<> norm{0., 1.};
    std::uniform_int_distribution<unsigned> random_seed(2, 1654636360u);
    // Kernel functions
    kernel_set<double> ann_set({"sig", "tanh", "ReLu", "ELU", "ISRU", "sum"});
    // a random dCGPANN
    expression_ann ex(n, m, r, c, lb, arity, ann_set(), random_seed(gen));
    // Since weights and biases are, by default, set to ones, we randomize them
    ex.randomise_weights(0, 1., random_seed(gen));
    ex.randomise_biases(0, 1., random_seed(gen));
    auto orig_w = ex.get_weights();
    auto orig_b = ex.get_biases();
    // Numerical derivative eps
    auto eps = 1e-4;
    // Input value
    auto in = std::vector<double>(ex.get_n(), norm(gen));
    // Output value desired (supervised signal)
    auto out = std::vector<double>(ex.get_m(), norm(gen));
    if (loss_e == expression_ann::loss_type::CE) {
        // we normalize to probabilities
        double cumout = std::accumulate(out.begin(), out.end(), 0.);
        std::transform(out.begin(), out.end(), out.begin(), [cumout](double x) { return x / cumout; });
    }

    // Compute the loss and the gradients
    double value = 0.;
    std::vector<double> gweights(ex.get_weights().size(), 0.);
    std::vector<double> gbiases(ex.get_biases().size(), 0.);
    ex.d_loss(value, gweights, gbiases, in, out, loss_e);
    // Compute only the loss
    auto loss = ex.loss(in, out, loss_e);
    // We check the loss is equal when computed in both ways
    BOOST_CHECK_CLOSE(value, loss, 1e-12);

    // We check against numerical diff
    // first the weights
    ex.set_weights(orig_w);
    ex.set_biases(orig_b);
    for (decltype(ex.get_weights().size()) i = 0u; i < ex.get_weights().size(); ++i) {
        // We compute the numerical derivative with eps as a step
        ex.set_weights(orig_w);
        auto tmp = ex.get_weight(i);
        auto h = std::max(1., std::abs(tmp)) * eps;
        ex.set_weight(i, tmp + h);
        auto val = ex.loss(in, out, loss_e);
        ex.set_weight(i, tmp - h);
        auto val2 = ex.loss(in, out, loss_e);

        auto abs_diff = std::abs(((val - val2) / 2. / h - gweights[i]));
        auto rel_diff = abs_diff / std::abs(gweights[i]);

        // Since numerical differentiation sucks we look (brute force) for a better step for numerical
        // differentiation
        auto bval = val;
        auto bval2 = val2;
        auto best = rel_diff;
        if (!(rel_diff < 0.05 || abs_diff < 1e-8)) {
            h = 10.;
            for (auto j = 0u; j < 6; ++j) {
                ex.set_weights(orig_w);
                tmp = ex.get_weight(i);
                h = h * 0.01; // will generate 0.1, 0.001, ...., 0.000000001
                ex.set_weight(i, tmp + h);
                val = ex.loss(in, out, loss_e);
                ex.set_weight(i, tmp - h);
                val2 = ex.loss(in, out, loss_e);
                abs_diff = std::abs(((val - val2) / 2. / h - gweights[i]));
                rel_diff = abs_diff / std::abs(gweights[i]);

                if (rel_diff < best) {
                    best = rel_diff;
                    bval = val;
                    bval2 = val2;
                }
                if (rel_diff < 0.05 || abs_diff == 0) {
                    break;
                }
            }
        }

        // We test the results.
        // If there is a numerical difference when computing ex(x) and ex(x+h)
        if (bval != bval2) {
            BOOST_CHECK(best < 0.05 || abs_diff < 1e-8);
        } else {
            BOOST_CHECK(std::abs(gweights[i]) < 1.);
        }
    }

    // then the biases
    ex.set_weights(orig_w);
    ex.set_biases(orig_b);
    for (decltype(ex.get_biases().size()) i = 0u; i < ex.get_biases().size(); ++i) {
        ex.set_biases(orig_b);
        auto tmp = ex.get_bias(i);
        auto h = std::max(1., std::abs(tmp)) * eps;
        ex.set_bias(i, tmp + h);
        auto val = ex.loss(in, out, loss_e);
        ex.set_bias(i, tmp - h);
        auto val2 = ex.loss(in, out, loss_e);

        auto abs_diff = std::abs(((val - val2) / 2. / h - gbiases[i]));
        auto rel_diff = abs_diff / std::abs(gbiases[i]);

        // Since numerical differentiation sucks we look (brute force) for a better step for numerical
        // differentiation
        auto bval = val;
        auto bval2 = val2;
        auto best = rel_diff;
        if (!(rel_diff < 0.05 || abs_diff < 1e-8)) {
            h = 10.;
            for (auto j = 0u; j < 6; ++j) {
                ex.set_biases(orig_b);
                tmp = ex.get_bias(i);
                h = h * 0.01; // will generate 0.1, 0.001, ...., 0.000000001
                ex.set_bias(i, tmp + h);
                val = ex.loss(in, out, loss_e);
                ex.set_bias(i, tmp - h);
                val2 = ex.loss(in, out, loss_e);
                abs_diff = std::abs(((val - val2) / 2. / h - gbiases[i]));
                rel_diff = abs_diff / std::abs(gbiases[i]);

                if (rel_diff < best) {
                    best = rel_diff;
                    bval = val;
                    bval2 = val2;
                }
                if (rel_diff < 0.05 || abs_diff == 0) {
                    break;
                }
            }
        }
        // We test the results.
        // If there is a numerical difference when computing ex(x) and ex(x+h)
        if (bval != bval2) {
            BOOST_CHECK(best < 0.05 || abs_diff < 1e-8);
        } else {
            // Numercially there is no difference, the analytical results must be something small
            BOOST_CHECK(std::abs(gbiases[i]) < 1.);
        }
    }
}

BOOST_AUTO_TEST_CASE(construction)
{
    // Random seed
    std::random_device rd;
    // Kernel functions
    kernel_set<double> ann_set({"tanh"});
    expression_ann ex(1, 1, 1, 2, 1, 1, ann_set(), rd());
    // We test that all weights are set to 1 and biases to 0
    auto ws = ex.get_weights();
    auto bs = ex.get_biases();
    BOOST_CHECK(std::all_of(ws.begin(), ws.end(), [](unsigned el) { return el == 1u; }));
    BOOST_CHECK(std::all_of(bs.begin(), bs.end(), [](unsigned el) { return el == 0u; }));

    kernel_set<double> ann_set_malformed1({"tanh", "sin"});
    kernel_set<double> ann_set_malformed2({"cos", "sig"});
    kernel_set<double> ann_set_malformed3({"ReLu", "diff"});

    BOOST_CHECK_THROW((expression_ann{1, 1, 1, 2, 1, 1, ann_set_malformed1(), rd()}), std::invalid_argument);
    BOOST_CHECK_THROW((expression_ann{1, 1, 1, 2, 1, 1, ann_set_malformed2(), rd()}), std::invalid_argument);
    BOOST_CHECK_THROW((expression_ann{1, 1, 1, 2, 1, 1, ann_set_malformed3(), rd()}), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(parenthesis)
{
    {
        // We test a simple arity 1 row 1 dCGP-ANN
        // Random seed
        std::random_device rd;
        // Kernel functions
        kernel_set<double> ann_set({"tanh"});
        expression_ann ex(1, 1, 1, 2, 1, 1, ann_set(), rd());
        ex.set_weights({0.1, 0.2});
        ex.set_biases({0.3, 0.4});
        auto res = ex({0.23})[0];
        auto ground_truth = std::tanh(0.4 + 0.2 * std::tanh(0.23 * 0.1 + 0.3));
        BOOST_CHECK_CLOSE(res, ground_truth, 1e-13);
    }
    {
        // We test a simple arity 2 row 1 dCGP-ANN
        // Random seed
        std::random_device rd;
        // Kernel functions
        kernel_set<double> ann_set({"tanh"});
        expression_ann ex(1, 1, 1, 2, 1, 2, ann_set(), rd());
        ex.set_weights({0.1, 0.2, 0.3, 0.4});
        ex.set_biases({0.5, 0.6});
        auto res = ex({0.23})[0];
        auto n1 = std::tanh(0.23 * 0.1 + 0.23 * 0.2 + 0.5);
        auto ground_truth = std::tanh(0.3 * n1 + 0.4 * n1 + 0.6);
        BOOST_CHECK_CLOSE(res, ground_truth, 1e-13);
    }
    {
        // We test a arity 2 row 2 column 2 dCGP-ANN
        // Random seed
        std::random_device rd;
        // Kernel functions
        kernel_set<double> ann_set({"tanh"});
        expression_ann ex(1, 1, 2, 2, 1, 2, ann_set(), rd());
        ex.set_weights({0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8});
        ex.set_biases({0.9, 1.1, 1.2, 1.3});
        ex.set({0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 1, 2, 3});
        auto res = ex({0.23})[0];
        auto n0 = 0.23;
        auto n1 = std::tanh(0.1 * n0 + 0.2 * n0 + 0.9);
        auto n2 = std::tanh(0.3 * n0 + 0.4 * n0 + 1.1);
        auto ground_truth = std::tanh(0.5 * n1 + 0.6 * n2 + 1.2);
        BOOST_CHECK_CLOSE(res, ground_truth, 1e-13);
    }
}

BOOST_AUTO_TEST_CASE(sgd)
{
    audi::print("Calling Stochastic Gradient Descent\n");

    // Random numbers stuff
    std::random_device rd;
    std::mt19937 gen{rd()};
    std::uniform_real_distribution<> uniform(-1., 1.);

    // Kernel functions
    kernel_set<double> ann_set({"sig", "tanh", "ReLu"});
    expression_ann ex(3, 2, 100, 3, 1, 10, ann_set(), rd());
    ex.randomise_weights();
    ex.randomise_biases();
    std::vector<std::vector<double>> data(200, {0., 0., 0.});
    std::vector<std::vector<double>> label(200, {0., 0.});
    for (auto &item : data) {
        std::generate(item.begin(), item.end(), [&uniform, &gen]() { return uniform(gen); });
    }
    for (auto i = 0u; i < label.size(); ++i) {
        label[i][0] = 1. / 5. * std::cos(data[i][0] + data[i][1] + data[i][2]) - data[i][0] * data[i][1];
        label[i][1] = data[i][0] * data[i][1] * data[i][2];
    }
    double tmp_start = ex.loss(data, label, "MSE");
    double tmp_end = 0.;
    audi::print("Start: ", tmp_start, "\n");
    for (auto j = 0u; j < 20; ++j) {
        auto loss = ex.sgd(data, label, 0.001, 32, "MSE");
        tmp_end = ex.loss(data, label, "MSE");
        audi::print("Loss (", j, ") real: ", tmp_end, " proxy: ", loss, "\n");
    }
    BOOST_CHECK(tmp_end <= tmp_start);
}

BOOST_AUTO_TEST_CASE(d_loss)
{
    audi::print("Testing against numerical derivatives\n");
    using loss_t = expression_ann::loss_type;

    // Random distributions
    std::mt19937 gen(std::random_device{}());
    std::normal_distribution<> norm{0., 1.};
    std::uniform_int_distribution<unsigned> random_seed(2, 165360u);

    // corner cases
    test_against_numerical_derivatives(1, 1, 1, 1, 1, {2}, random_seed(gen), loss_t::MSE);
    test_against_numerical_derivatives(2, 1, 1, 1, 1, {2}, random_seed(gen), loss_t::MSE);
    test_against_numerical_derivatives(1, 2, 1, 1, 1, {2}, random_seed(gen), loss_t::MSE);
    test_against_numerical_derivatives(2, 2, 1, 1, 1, {2}, random_seed(gen), loss_t::MSE);
    test_against_numerical_derivatives(2, 2, 2, 2, 2, {2, 2}, random_seed(gen), loss_t::MSE);

    // medium
    test_against_numerical_derivatives(5, 1, 5, 5, 1, {2, 2, 2, 2, 2}, random_seed(gen), loss_t::MSE);
    test_against_numerical_derivatives(1, 5, 1, 1, 1, {2}, random_seed(gen), loss_t::MSE);
    test_against_numerical_derivatives(3, 4, 6, 6, 1, {6, 6, 6, 6, 6, 6}, random_seed(gen), loss_t::MSE);

    // higher dimensions
    test_against_numerical_derivatives(10, 13, 100, 1, 1, {45}, random_seed(gen), loss_t::MSE);
    test_against_numerical_derivatives(3, 2, 100, 1, 1, {23}, random_seed(gen), loss_t::MSE);

    // Checks on Cross - entropy
    test_against_numerical_derivatives(5, 1, 5, 5, 1, {2, 2, 2, 2, 2}, random_seed(gen), loss_t::CE);
    test_against_numerical_derivatives(1, 5, 1, 1, 1, {2}, random_seed(gen), loss_t::CE);
    test_against_numerical_derivatives(3, 4, 6, 6, 1, {6, 6, 6, 6, 6, 6}, random_seed(gen), loss_t::CE);

    // Checks on non-uniform arity
    test_against_numerical_derivatives(5, 1, 5, 5, 2, {2, 4, 3, 5, 7}, random_seed(gen), loss_t::MSE);
    test_against_numerical_derivatives(3, 4, 6, 6, 2, {10, 10, 30, 2, 4, 5}, random_seed(gen), loss_t::CE);

    // Checks on corner case arity (1)
    test_against_numerical_derivatives(5, 1, 5, 5, 2, {2, 1, 3, 1, 7}, random_seed(gen), loss_t::MSE);
    test_against_numerical_derivatives(5, 1, 6, 6, 2, {1, 1, 1, 1, 1, 1}, random_seed(gen), loss_t::CE);
}

BOOST_AUTO_TEST_CASE(n_active_weights)
{
    // Random numbers stuff
    std::random_device rd;
    std::mt19937 gen{rd()};
    std::normal_distribution<> norm(0., 1.);

    // Kernel functions
    kernel_set<double> ann_set({"sig", "tanh", "ReLu"});
    {
        expression_ann ex(2, 2, 2, 2, 5, 2, ann_set(), rd());
        ex.set({0, 0, 1, 0, 0, 1, 0, 2, 3, 0, 2, 3, 4, 5});
        BOOST_CHECK(ex.n_active_weights() == 8u);
        BOOST_CHECK(ex.n_active_weights(false) == 8u);
        BOOST_CHECK(ex.n_active_weights(true) == 8u);
        ex.set({0, 1, 1, 0, 0, 1, 0, 2, 3, 0, 2, 3, 4, 5});
        BOOST_CHECK(ex.n_active_weights() == 8u);
        BOOST_CHECK(ex.n_active_weights(false) == 8u);
        BOOST_CHECK(ex.n_active_weights(true) == 7u);
    }
}