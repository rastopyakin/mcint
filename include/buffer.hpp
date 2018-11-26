#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace mc {

    template<class T>
    class buffer {
    public:
        template<class U>
        void push(U &&elem) {
            std::lock_guard<std::mutex> lk{buffer_lock};
            buffer.push(std::forward<U>(elem));
            push_cond.notify_one();
        }

        std::shared_ptr<T> pop_or_wait() {
            std::unique_lock<std::mutex> lk{buffer_lock};
            push_cond.wait(lk, [this] {return !buffer.empty() or stop_flag.load();});
            if (buffer.empty()) {
                return std::shared_ptr<T>{};
            }
            auto elem = std::make_shared<T>(std::move(buffer.front()));
            buffer.pop();
            return elem;
        }
        void disable_waiting() {
            stop_flag.store(true);
            std::lock_guard<std::mutex> lk{buffer_lock};
            push_cond.notify_all();
        }
    private:
        std::queue<T> buffer;
        std::mutex buffer_lock;
        std::condition_variable push_cond;
        std::atomic_bool stop_flag {false};
    };

}

#endif /* BUFFER_HPP */
