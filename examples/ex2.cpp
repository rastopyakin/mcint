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
    mc::pool<mc::worker<decltype(f), double>> pool{f, calls_per_update};

    auto start = std::chrono::steady_clock::now();

    auto finish = start + std::chrono::seconds(10);

    auto t = finish - std::chrono::steady_clock::now();
    std::cout.precision(10);
    while (t.count() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        auto chunk = pool.get_current_stat();

        std::size_t total_calls  = chunk.calls_num;
        double estimate = chunk.M1;
        double variance_estimate =
            (chunk.M2 - estimate*estimate)*total_calls/(total_calls - 1);
        double error = std::abs(estimate - std::exp(1));
        double error_estimate = std::sqrt(variance_estimate/total_calls);
        std::cout << "current estimate: "  << estimate  << std::endl;
        std::cout << "variance estimate: " << variance_estimate << std::endl;
        std::cout << "error: " << error << std::endl;
        std::cout << "error estimate: " << error_estimate << std::endl;
        std::cout << "ratio: " << error/error_estimate << std::endl;
        std::cout << "calls number: " << total_calls << std::endl;
        std::cout << std::endl;

        t = finish - std::chrono::steady_clock::now();
    }

    pool.stop();

    return 0;
}
