#ifndef MCINT_HPP
#define MCINT_HPP

#include <thread>
#include <random>
#include <atomic>
#include <mutex>
#include <cmath>
#include <list>
#include <memory>
#include <iostream>

#include "buffer.hpp"

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
        if (chunk.calls_num != 0)
            return std::sqrt(variance(chunk)/chunk.calls_num);
        else
            return std::numeric_limits<ValType>::max();
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
    protected:
        virtual void update(ValType M1, ValType M2, std::size_t chunks_num) {
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
        std::thread work_th;
        std::atomic<bool> stop_flag {false};
        Func integrand;
    protected:
        std::size_t calls_per_chunk;
    };

    template <class Func, class ValType, class ArgType, class Gen>
    typename Gen::result_type worker<Func, ValType, ArgType, Gen>::seed_counter = 0;

    template<class ValType>
    class progress_manager {
    public:
        progress_manager() {
            th = std::thread(&progress_manager<ValType>::process, this);
        }
        void send_chunk(mc_chunk<ValType> chunk) {
            chunks_buf.push(chunk);
        }
        ~progress_manager() {
            chunks_buf.disable_waiting();
            th.join();
        }
    private:
        void process() {
            while (true) {
                auto chunk = chunks_buf.pop_or_wait();
                if (chunk) {
                    global.M1 = (global.M1*global.calls_num + chunk->M1*chunk->calls_num)/(global.calls_num + chunk->calls_num);
                    global.M2 = (global.M2*global.calls_num + chunk->M2*chunk->calls_num)/(global.calls_num + chunk->calls_num);
                    global.calls_num += chunk->calls_num;
                } else break;
                std::cout << "current estimate: "  << global.M1  << std::endl;
                std::cout << "variance estimate: " << variance(global) << std::endl;
                std::cout << "error estimate: " << error(global) << std::endl;
                std::cout << "calls number: " << global.calls_num << std::endl;
                std::cout << std::endl;
            }
        }
        mc_chunk<ValType> global {};
        buffer<mc_chunk<ValType>> chunks_buf;
        std::thread th;
    };

    template <class Func, class ValType, class ArgType = ValType,
              class Gen = std::mt19937_64>
    class p_worker : public worker<Func, ValType, ArgType, Gen> {
    public:
        using worker_t = worker<Func, ValType, ArgType, Gen>;
        using pm_t = progress_manager<ValType>;
        p_worker(Func f, std::size_t calls,
                 const std::shared_ptr<pm_t> &p) :
            worker_t{f, calls}, pm{p} {
            }
    protected:
        void update(ValType M1, ValType M2, std::size_t chunks_num) override {
            worker_t::update(M1, M2, chunks_num);
            pm->send_chunk(make_chunk(M1, M2, this->calls_per_chunk));
        }
        std::shared_ptr<pm_t> pm;
    };

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

    class naive_integrator {};

}

#endif /* MCINT_HPP */
