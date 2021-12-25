#pragma once

#include <budgie/ts_forward_list.hpp>

#include <thread>
#include <iostream>
#include <atomic>
#include <set>
#include <condition_variable>
#include <functional>
#include <vector>
#include <optional>

// XXX usleep()
#include <unistd.h>

class thread_pool {
	std::vector<std::thread> workers;
	// mutex not needed for syncronization, only used for condition_variable
	std::mutex mtx;
	std::condition_variable cv;
	std::condition_variable waiter;
	std::atomic<bool> finished;
	std::atomic<unsigned> available;
	unsigned numthreads;
	
	using Task = std::function<void()>;
	ts_forward_list<Task> tasks;

	public:
		thread_pool(unsigned threads = std::thread::hardware_concurrency()) {
			numthreads = threads;
			available = 0;
			start();
		}

		~thread_pool() {
			stop();
		}

		void add_task(Task t) {
			tasks.push_front(t);
			// TODO: this might wake up a thread which will only start
			//       after another thread has taken the task, causing the
			//       thread to immediately go back to sleep...
			//       must be a more efficient way to do this
			cv.notify_one();
		}

		// map a task to each thread in the pool
		void run_all(Task t) {
			for (unsigned i = 0; i < numthreads; i++) {
				add_task(t);
			}
		}

		void start() {
			finished = false;

			for (unsigned i = 0; i < numthreads; i++) {
				workers.push_back(std::thread(worker_loop, this));
				available++;
			}
		}

		void wait(void) {
			//std::unique_lock<std::mutex> lock(mtx);
			// wait for all pending tasks to finish
			while (!tasks.empty() || available != workers.size()) {
				// XXX
				// TODO: need to do some testing to determine if it's
				//       better to sleep here or use a condition variable,
				//       intuitively it would make sense for less syncroniztion
				//       to be better, but idk really
				usleep(25000);
				//std::this_thread::yield();
				//waiter.wait(lock);
			}
		}

		void stop(void) {
			finished = true;
			available = 0;
			cv.notify_all();

			for (auto& t : workers) {
				t.join();
			}

			workers.clear();
		}

		static void worker_loop(thread_pool *pool) {
			while (true) {
				// check finished before and after waiting for the cv
				if (pool->finished) return;
				if (pool->tasks.empty()) {
					std::unique_lock<std::mutex> lock(pool->mtx);
					//pool->waiter.notify_one();
					pool->cv.wait(lock);
				}
				if (pool->finished) return;

				pool->available--;

				// task list might be empty anyway
				if (auto res = pool->tasks.pop_front()) {
					(*res)();
				}

				pool->available++;
			}
		}
};
