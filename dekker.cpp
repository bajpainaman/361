/**
 * Dekker's Algorithm Implementation in C++
 * CS 361 - Final Assessment, Question 3, Option 7
 *
 * Dekker's algorithm is a concurrent programming algorithm for mutual exclusion
 * that allows two threads to share a single-use resource without conflict,
 * using only shared memory for communication.
 *
 * Key Components:
 * - flag[2]: Each thread's intent to enter critical section
 * - turn: Tie-breaker when both threads want to enter
 *
 * Properties demonstrated:
 * - Mutual Exclusion: Only one thread in critical section at a time
 * - No Deadlock: Progress is always made
 * - No Starvation: Both threads get fair access (via turn variable)
 */

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>

// Shared variables for Dekker's Algorithm
std::atomic<bool> flag[2] = {false, false};  // Intent flags
std::atomic<int> turn{0};                     // Tie-breaker

// Shared resource to demonstrate mutual exclusion
std::atomic<int> shared_counter{0};
std::atomic<int> in_critical_section{0};  // Should never exceed 1

// Number of times each thread enters critical section
const int NUM_ITERATIONS = 5;

// Thread-safe printing with timestamp
void print(int thread_id, const std::string& message) {
    static std::atomic<int> line_num{0};
    int ln = ++line_num;
    std::cout << "[" << ln << "] Thread " << thread_id << ": " << message << std::endl;
}

/**
 * Dekker's Algorithm for thread 'id' (0 or 1)
 * 'other' is the other thread's id
 */
void dekker_lock(int id, int other) {
    print(id, "Wants to enter critical section (setting flag[" + std::to_string(id) + "] = true)");
    flag[id].store(true, std::memory_order_seq_cst);

    // Check if other thread wants in
    bool contention = flag[other].load(std::memory_order_seq_cst);

    if (contention) {
        print(id, "Contention detected! Thread " + std::to_string(other) + " also wants in.");
    }

    while (flag[other].load(std::memory_order_seq_cst)) {
        if (turn.load(std::memory_order_seq_cst) != id) {
            print(id, "Not my turn (turn=" + std::to_string(turn.load()) + "). Backing off and waiting...");
            flag[id].store(false, std::memory_order_seq_cst);

            // Wait for turn silently
            while (turn.load(std::memory_order_seq_cst) != id) {
                std::this_thread::yield();
            }

            print(id, "My turn now! Re-raising flag.");
            flag[id].store(true, std::memory_order_seq_cst);
        } else {
            // Our turn but other thread still has flag up, wait silently
            std::this_thread::yield();
        }
    }

    print(id, ">>> ENTERING CRITICAL SECTION <<<");
}

void dekker_unlock(int id, int other) {
    print(id, "<<< LEAVING CRITICAL SECTION >>>");
    print(id, "Passing turn to Thread " + std::to_string(other));

    turn.store(other, std::memory_order_seq_cst);
    flag[id].store(false, std::memory_order_seq_cst);

    print(id, "Released lock (flag[" + std::to_string(id) + "] = false)");
}

/**
 * Thread function that repeatedly enters critical section
 */
void thread_function(int id) {
    int other = 1 - id;  // The other thread (0->1, 1->0)

    print(id, "=== STARTED ===");

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        print(id, "--- Iteration " + std::to_string(i + 1) + "/" + std::to_string(NUM_ITERATIONS) + " ---");

        // Entry section (Dekker's algorithm)
        dekker_lock(id, other);

        // ============ CRITICAL SECTION ============
        int cs_count = ++in_critical_section;
        if (cs_count > 1) {
            std::cerr << "!!! MUTUAL EXCLUSION VIOLATED !!! Count: " << cs_count << std::endl;
        }

        // Simulate work in critical section
        int old_val = shared_counter.load();
        print(id, "Reading shared_counter = " + std::to_string(old_val));

        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Simulate work

        int new_val = old_val + 1;
        shared_counter.store(new_val);
        print(id, "Writing shared_counter = " + std::to_string(new_val));

        --in_critical_section;
        // ========== END CRITICAL SECTION ==========

        // Exit section
        dekker_unlock(id, other);

        // Remainder section (non-critical work)
        print(id, "Doing non-critical work...");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    print(id, "=== FINISHED ===");
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║           DEKKER'S ALGORITHM DEMONSTRATION                   ║" << std::endl;
    std::cout << "║                                                              ║" << std::endl;
    std::cout << "║  Two threads competing for a shared critical section.        ║" << std::endl;
    std::cout << "║  Each thread will enter " << NUM_ITERATIONS << " times.                            ║" << std::endl;
    std::cout << "║  Final counter should be " << NUM_ITERATIONS * 2 << " if mutual exclusion holds.     ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    std::cout << "Initial state:" << std::endl;
    std::cout << "  flag[0] = " << flag[0] << ", flag[1] = " << flag[1] << std::endl;
    std::cout << "  turn = " << turn << std::endl;
    std::cout << "  shared_counter = " << shared_counter << std::endl;
    std::cout << std::endl;
    std::cout << "Starting threads..." << std::endl;
    std::cout << "════════════════════════════════════════════════════════════════" << std::endl;

    // Create two threads
    std::thread t0(thread_function, 0);
    std::thread t1(thread_function, 1);

    // Wait for both to complete
    t0.join();
    t1.join();

    std::cout << "════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                        RESULTS                               ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  Final shared_counter = " << shared_counter << "                                   ║" << std::endl;
    std::cout << "║  Expected value       = " << NUM_ITERATIONS * 2 << "                                   ║" << std::endl;
    std::cout << "║                                                              ║" << std::endl;

    if (shared_counter == NUM_ITERATIONS * 2) {
        std::cout << "║  ✓ MUTUAL EXCLUSION VERIFIED - No race conditions!          ║" << std::endl;
    } else {
        std::cout << "║  ✗ ERROR - Race condition detected!                         ║" << std::endl;
    }

    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;

    return 0;
}
