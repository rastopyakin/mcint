#include <iostream>
#include "mcint.hpp"

int main(int argc, char *argv[]) {

    auto f = [] (auto x) {return exp(x)+1;};
    std::size_t calls_per_update = 1e6;
    auto pm = std::make_shared<mc::progress_manager<double>>();
    std::size_t num_threads = std::thread::hardware_concurrency();
    mc::pool<mc::p_worker<decltype(f), double>> wk{num_threads, f, calls_per_update, pm};

    // mc::p_worker<decltype(f), double> wk{f, calls_per_update, pm};
    std::this_thread::sleep_for(std::chrono::seconds(5));

    wk.stop();

    return 0;
}
