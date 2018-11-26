#include <iostream>
#include "mcint.hpp"

int main(int argc, char *argv[]) {

    auto f = [] (auto x) {return 4*std::sqrt(1 - x*x);};
    std::size_t calls_per_update = 1e6;
    double error_goal = 1e-5;
    auto pm = std::make_shared<mc::progress_manager<double>>(error_goal);
    std::size_t num_threads = std::thread::hardware_concurrency();
    mc::pool<mc::p_worker<decltype(f), double>> wk{num_threads, f, calls_per_update, pm};

    pm->wait_for_completion();

    wk.stop();

    auto result = wk.get_current_stat();

    double estimate = result.M1;
    double error = std::abs(estimate - std::acos(-1.0));

    std::cout.precision(10);
    std::cout << "final estimate: "  << estimate  << std::endl;
    std::cout << "variance :" << mc::variance(result) << std::endl;
    std::cout << "actual error: " << error << std::endl;
    std::cout << "error estimate: " << mc::error(result) << std::endl;
    std::cout << "ratio: " << error/mc::error(result) << std::endl;
    std::cout << "calls number: " << result.calls_num << std::endl;


    return 0;
}
