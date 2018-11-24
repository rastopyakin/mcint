#ifndef MCINT_HPP
#define MCINT_HPP

#include <thread>
#include <random>
#include <atomic>
#include <mutex>
#include <iostream>

namespace mc {

    template <class ValType>
    struct mc_chunk {
        ValType M1 {};         // mean value
        ValType M2 {};         // squared mean value
        std::size_t  calls_num = 0;
    };

    template <class Func, class ValType, class ArgType = ValType,
              class Gen = std::mt19937_64>
    class worker {
    public:
        worker(const Func& f, std::size_t calls_num) :
            integrand{f}, calls_per_chunk(calls_num) {
                seed = seed_counter++;
                work_th = std::thread{&worker::run, this, seed_counter};
            }
        ~worker() {
            work_th.join();
        }
        mc_chunk<ValType> get_current_stat() const {
            std::lock_guard<std::mutex> lk{lock};
            return current;
        }
        void stop() {
            stop_flag.store(true);
        }
    private:
        void run(typename Gen::result_type seed) {
            Gen gen(seed);
            std::size_t chunks_num = 0;
            std::uniform_real_distribution<ArgType> ur{0.0, 1.0};
            while (!stop_flag.load()) {
                ValType M1 = 0;
                ValType M2 = 0;
                for (int i = 0; i < calls_per_chunk; i++) {
                    ValType val = integrand(ur(gen));
                    M1 += val;
                    M2 += val*val;
                }
                chunks_num++;
                M1 /= calls_per_chunk;
                M2 /= calls_per_chunk;
                update(M1, M2, chunks_num - 1); // pass num of current chunks
            }
        }

        void update(ValType M1, ValType M2, std::size_t chunks_num) {
            std::lock_guard<std::mutex> lk{lock};
            current.M1 = (chunks_num*current.M1 + M1)/(chunks_num + 1);
            current.M2 = (chunks_num*current.M2 + M2)/(chunks_num + 1);
            current.calls_num += calls_per_chunk;
        }
    private:
        static typename Gen::result_type seed_counter;
        typename Gen::result_type seed;
        mutable std::mutex lock;
        mc_chunk<ValType> current;
        Func integrand;
        std::size_t calls_per_chunk;
        std::thread work_th;
        std::atomic<bool> stop_flag {false};
    };

    class collector {};

    class naive_integrator {};

    template <class Func, class ValType, class ArgType, class Gen>
    typename Gen::result_type worker<Func, ValType, ArgType, Gen>::seed_counter = 0;

}

#endif /* MCINT_HPP */
