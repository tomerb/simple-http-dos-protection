#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class SafeMsgQueue
{
 public:
    SafeMsgQueue()
    {
    }

    void enqueue(T item)
    {
        {
            std::lock_guard lock(mutex);
            queue.push(item);
        }

        cond_var.notify_one();
    }

    T dequeue()
    {
        std::unique_lock lock(mutex);
        cond_var.wait(lock, [&]{ return !queue.empty(); });
        auto item = queue.front();
        queue.pop();
        return item;
    }

 private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable cond_var;
};
