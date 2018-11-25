#ifndef MCINT_HPP
#define MCINT_HPP

#include <thread>
#include <random>
#include <atomic>
#include <mutex>
#include <cmath>
#include <list>
#include <iostream>

namespace mc {

    template <class ValType>
    struct mc_chunk {
        mc_chunk() = default;
        ValType M1 {};         // mean value
        ValType M2 {};         // squared mean value
        std::size_t  calls_num = 0;
    };

    template<class ValType>
    auto make_chunk(ValType M1, ValType M2, std::size_t calls_num) {
        return mc_chunk<ValType>{M1, M2, calls_num};
    }

    template<class ValType>
    auto variance(const mc_chunk<ValType> &chunk) {
        return (chunk.M2 - chunk.M1*chunk.M1)*chunk.calls_num/(chunk.calls_num - 1);
    }

    template<class ValType>
    auto error(const mc_chunk<ValType> &chunk) {
        return std::sqrt(variance(chunk)/chunk.calls_num);
    }

    template <class Func, class ValType, class ArgType = ValType,
              class Gen = std::mt19937_64>
    class worker {
    public:
        using value_t = ValType;
        using func_t = Func;

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

    template <class Func, class ValType, class ArgType, class Gen>
    typename Gen::result_type worker<Func, ValType, ArgType, Gen>::seed_counter = 0;

    template<class WorkerT>
    class pool {
    public:
        using worker_t = WorkerT;
        using value_t = typename WorkerT::value_t;
        using func_t = typename WorkerT::func_t;

        template<class ... Args>
        pool(std::size_t workers_num, Args ... args) {
            for (int i = 0; i < workers_num; i++)
                workers.emplace_back(args...);
        }
        void stop() {
            for (auto &wk : workers)
                wk.stop();
        }
        auto get_current_stat() const {
            mc_chunk<value_t> chunk;
            for (const auto &wk : workers) {
                auto temp = wk.get_current_stat();
                chunk.M1 += temp.M1*temp.calls_num;
                chunk.M2 += temp.M2*temp.calls_num;
                chunk.calls_num += temp.calls_num;
            }
            chunk.M1 /= chunk.calls_num;
            chunk.M2 /= chunk.calls_num;
            return chunk;
        }
    private:
        std::list<worker_t> workers;
    };

    class governor {};

    class naive_integrator {};

}

#endif /* MCINT_HPP */
