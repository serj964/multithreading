#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <cassert>
#include <climits>
#include <chrono>


struct Backoff {
public:
    inline void operator()() {
#ifdef __GNUC__
        __asm volatile ("pause");
#endif
        if (++iteration_num == limit) {
#ifdef __GNUC__
            __asm volatile ("pause");
#endif
            iteration_num = 0;
            limit = (limit >= min) ? limit * 100 / 85 : min;
            std::this_thread::yield();
        }

#ifdef __GNUC__
        __asm volatile ("pause");
#endif
    }

private:
    int min = 45000;
    int limit = 100'000;
    int iteration_num = 0;
};


class TTAS {
private:
    std::atomic<bool> locked;

public:
    TTAS() : locked(false) {}

    void lock() {

        bool flag = false;
        Backoff backoff;

        do {

//            flag = false;

            while (locked.load(std::memory_order_relaxed)) {
                backoff();
            }// wait


        } while (!locked.compare_exchange_weak(flag, true, std::memory_order_acquire, std::memory_order_relaxed));

    }

    void unlock() {
        locked.store(false, std::memory_order_release);
    }
};


class TAS {
private:
    std::atomic<bool> locked;

public:
    TAS() : locked(false) {}

    void lock() {

        bool flag = false;
        Backoff backoff;

        while (!locked.compare_exchange_weak(flag, true, std::memory_order_acquire, std::memory_order_relaxed)) {
            backoff();
        }

    }

    void unlock() {
        locked.store(false, std::memory_order_release);
    }
};


class TicketLock {
private:
    std::atomic<unsigned int> current_ticket;
    std::atomic<unsigned int> next_ticket;

public:
    void lock() {
        const unsigned my_ticket = next_ticket.fetch_add(1, std::memory_order_relaxed);
        Backoff backoff;
        while (current_ticket.load(std::memory_order_relaxed) != my_ticket) {
            backoff();
        }
        current_ticket.load(std::memory_order_acquire);
    }

    void unlock() {

        const unsigned next = current_ticket.load(std::memory_order_relaxed) + 1;
        current_ticket.store(next, std::memory_order_release);

        //current_ticket.fetch_add(1, std::memory_order_release);
    }
};