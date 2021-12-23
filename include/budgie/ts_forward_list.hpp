#pragma once

#include <thread>
#include <iostream>
#include <atomic>

// TODO: put in namespace?
template <typename T>
class ts_forward_list {
	struct listnode {
		listnode(const T& em) {
			element = em;
		}

		listnode *next;
		T element;
	};

	struct iterator {
		listnode *cur;

		iterator(listnode *start)
			: cur(start) {};

		const iterator& operator++(void) {
			if (cur) {
				cur = cur->next;
			}

			return *this;
		}

		const T& operator*(void) {
			return cur->element;
		}

		bool operator==(const iterator& other) {
			return cur == other.cur;
		}

		bool operator!=(const iterator& other) {
			return !(*this == other);
		}
	};

	std::atomic<listnode*> start;
	std::atomic<size_t> numelems;

	public:
		ts_forward_list() {
			start = nullptr;
			numelems = 0;
		}

		size_t size() {
			return numelems;
		}

		void push_front(const T& em) {
			listnode *t = new listnode(em);

			do {
				t->next = start;
			} while (!start.compare_exchange_weak(t->next, t));

			numelems++;
		}

		void pop_front() {
			// TODO: pop_front()
		}

		T& front() {
			return start->element;
		}

		void clear() {
			listnode *temp;
			do {
				temp = start;
				numelems = 0;
			} while (!start.compare_exchange_weak(temp, nullptr));

			while (temp) {
				listnode *next = temp->next;
				delete temp;
				temp = next;
			}
		}

		iterator begin(void) {
			return iterator(start);
		}

		iterator end(void) {
			return iterator(nullptr);
		}
};
