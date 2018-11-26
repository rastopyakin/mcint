// simple usage example of two workers
// calculating integral of e^x - 1 integrand.
#include <iostream>
#include <cmath>
#include <chrono>

#include "mcint.hpp"

namespace chr=std::chrono;

int main(int argc, char *argv[]) {

    auto f = [] (auto x) {return std::exp(x)+1;};
    std::size_t calls_per_update_num = 1e6;
    mc::worker<decltype(f), double>
        wk{f, calls_per_update_num}, wk1{f, calls_per_update_num};

    auto start = std::chrono::steady_clock::now();

    auto finish = start + std::chrono::seconds(10);

    auto t = finish - std::chrono::steady_clock::now();
    std::cout.precision(10);
    while (t.count() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        auto s1 = wk.get_current_stat();
        auto s2 = wk1.get_current_stat();

        std::size_t total_calls  = s1.calls_num + s2.calls_num;
        double estimate = (s1.M1*s1.calls_num + s2.M1*s2.calls_num)/total_calls;
        double variance_estimate = ((s1.M2*s1.calls_num + s2.M2*s2.calls_num)/total_calls -
                                    estimate*estimate)*total_calls/(total_calls - 1);
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
    wk.stop(); wk1.stop();
    return 0;
}
