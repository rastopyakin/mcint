#include <iostream>
#include <complex>
#include <vector>
#include <list>
#include <functional>
#include "mcint.hpp"

template<class Func> struct arg_type {};

template<class RetType, class ArgType>
struct arg_type<RetType(ArgType)> {
    using type = ArgType;
};

struct Foo {
    double operator()(double x) {
        return 0;
    }
};

template<class T>
void print_type(T t) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
}

int main(int argc, char *argv[]) {

    auto f = [] (auto x) {return exp(x)+1;};
    std::size_t calls_per_update = 1e6;
    std::size_t workers_num = std::thread::hardware_concurrency();
    mc::pool<mc::worker<decltype(f), double>> pool{workers_num, f, calls_per_update};
    double error_goal = 3.0e-5;
    std::cout.precision(10);
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        auto chunk = pool.get_current_stat();

        double estimate = chunk.M1;
        double error = std::abs(estimate - std::exp(1));
        std::cout << "current estimate: "  << estimate  << std::endl;
        std::cout << "variance estimate: " << mc::variance(chunk) << std::endl;
        std::cout << "error: " << error << std::endl;
        std::cout << "error estimate: " << mc::error(chunk) << std::endl;
        std::cout << "ratio: " << error/mc::error(chunk) << std::endl;
        std::cout << "calls number: " << chunk.calls_num << std::endl;
        std::cout << std::endl;
        if (mc::error(chunk) < error_goal) break;
    }

    std::cout << "the goal has been reached\n";
    pool.stop();

    return 0;
}
