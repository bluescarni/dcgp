#define BOOST_TEST_MODULE dcgp_evaluation_perf
#include <boost/test/unit_test.hpp>
#include <boost/timer/timer.hpp>
#include <iostream>

#include <dcgp/expression.hpp>
#include <dcgp/kernel_set.hpp>

void evaluate_loss(unsigned int in, unsigned int out, unsigned int rows, unsigned int columns, unsigned int levels_back,
                   unsigned int arity, unsigned int N, std::vector<dcgp::kernel<double>> kernel_set, bool parallel)
{
    // Random numbers engine
    std::default_random_engine re(123);
    // Instatiate the expression
    dcgp::expression<double> ex(in, out, rows, columns, levels_back, arity, kernel_set, 0u, 123u);
    // We create the input data upfront and we do not time it.
    std::vector<double> dumb(in);
    std::vector<double> dumb2(out);
    std::vector<std::vector<double>> points(N, dumb);
    std::vector<std::vector<double>> labels(N, dumb2);

    for (auto j = 0u; j < N; ++j) {
        for (auto i = 0u; i < in; ++i) {
            points[j][i] = std::uniform_real_distribution<double>(-1, 1)(re);
            labels[j][i] = std::uniform_real_distribution<double>(-1, 1)(re);
        }
    }

    std::cout << "Performing " << N << " evaluations, in:" << in << " out:" << out << " rows:" << rows
              << " columns:" << columns << std::endl;
    {
        boost::timer::auto_cpu_timer t;
        ex.loss(points, labels, "MSE", parallel);
    }
}

BOOST_AUTO_TEST_CASE(evaluation_speed)
{
    unsigned int N = 100000;

    // Sequential
    dcgp::kernel_set<double> kernel_set1({"sum", "diff", "mul", "div", "sin", "exp", "sig"});
    audi::print("Sequential: function set ", kernel_set1(), "\n");
    evaluate_loss(2, 4, 2, 3, 4, 4, N, kernel_set1(), false);
    evaluate_loss(2, 4, 10, 10, 11, 5, N, kernel_set1(), false);
    evaluate_loss(2, 4, 20, 20, 21, 6, N, kernel_set1(), false);
    evaluate_loss(2, 2, 1, 100, 101, 7, N, kernel_set1(), false);
    evaluate_loss(2, 2, 2, 100, 101, 8, N, kernel_set1(), false);
    evaluate_loss(2, 2, 3, 100, 101, 9, N, kernel_set1(), false);
    // parallel
    audi::print("Parallel: function set ", kernel_set1(), "\n");
    evaluate_loss(2, 4, 2, 3, 4, 4, N, kernel_set1(), true);
    evaluate_loss(2, 4, 10, 10, 11, 5, N, kernel_set1(), true);
    evaluate_loss(2, 4, 20, 20, 21, 6, N, kernel_set1(), true);
    evaluate_loss(2, 2, 1, 100, 101, 7, N, kernel_set1(), true);
    evaluate_loss(2, 2, 2, 100, 101, 8, N, kernel_set1(), true);
    evaluate_loss(2, 2, 3, 100, 101, 9, N, kernel_set1(), true);
}
