#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <cassert>
#include <climits>
#include <chrono>

using namespace std::chrono_literals;
struct Backoff {

public:
    inline void operator()() {
#ifdef __GNUC__
        __asm volatile ("pause");
#endif
        std::this_thread::sleep_for(sleep_for_);
        sleep_for_ *= 2;
    }

private:
    std::chrono::duration<long, std::ratio<1, 1000>> sleep_for_ = 1ms;
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
            while (locked.load(std::memory_order_relaxed))
            {
                backoff();
            }
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
        while (!locked.compare_exchange_weak(flag, true, std::memory_order_acquire, std::memory_order_relaxed)) 
        {
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
        while (current_ticket.load(std::memory_order_relaxed) != my_ticket)
        {
            backoff();
        }
        current_ticket.load(std::memory_order_acquire);
    }
    void unlock() {
        const unsigned next = current_ticket.load(std::memory_order_relaxed) + 1;
        current_ticket.store(next, std::memory_order_release);
    }
};