#include <atomic>
#include <cassert>
#include <thread>
#include <array>
#include <iostream>

class SpscQueue {
    std::array<int, 10> arr{};
    std::atomic<std::size_t> head{};
    std::atomic<std::size_t> tail{};

public:
    bool push(int val) {
        std::size_t h = head.load(std::memory_order_relaxed);
        std::size_t t = tail.load(std::memory_order_acquire);
        std::size_t next = (h + 1) % arr.size();
        if (next == t) return false; 

        arr[h] = val;

        head.store(next, std::memory_order_release);
        return true;
    }

    bool pop(int& out) {
        std::size_t t = tail.load(std::memory_order_relaxed);
        std::size_t h = head.load(std::memory_order_acquire);
        if (h == t) return false;

        out = arr[t];

        tail.store(++t % arr.size(), std::memory_order_release);
        return true;
    }
};

int main() {
    SpscQueue q{};
    q.push(1);
    q.push(2);
    q.push(3);
    int out;
    while (q.pop(out)) {
        std::cout << out << '\n';
    }

    while (q.push(1));
    
    int count {};
    while (q.pop(out)) {
        ++count;
    }
    std::cout << count << '\n';

    constexpr int N = 10'000'000;
    int mismatches {};

    std::thread producer ([&] {
        for (int i = 0; i < N; ++i) {
           while (!q.push(i));
        }
    });

    std::thread consumer([&] {
        for (int i = 0; i < N; ++i) {
            while (!q.pop(out));
            if (out != i) ++mismatches;
        }
    });

    producer.join();
    consumer.join();

    if (mismatches != 0) std::cout << "Fail" << '\n';
    else std::cout << "Pass" << '\n';
    return 0;
}
