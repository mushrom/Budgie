#pragma once

#include <thread>
#include <iostream>
#include <atomic>
#include <optional>

// TODO: put in namespace?
template <typename T>
class ts_forward_list {
	struct listnode {
		listnode(const T& em) {
			element = em;
			next = nullptr;
		}

		listnode* next;
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

	static bool is_marked_reference(listnode *t) {
		return (uintptr_t)t & 1;
	}

	static listnode *mark_reference(listnode *t) {
		return (listnode*)((uintptr_t)t | 1);
	}

	static listnode *unmark_reference(listnode *t) {
		return (listnode*)((uintptr_t)t & (~(uintptr_t)1));
	}

	public:
		ts_forward_list() {
			start = nullptr;
			numelems = 0;
		}

		size_t size() {
			return numelems;
		}

		bool empty() {
			return numelems == 0;
		}

		void push_front(const T& em) {
			listnode *t = new listnode(em);

			do {
				t->next = start;
			} while (!start.compare_exchange_weak(t->next, t));

			numelems++;
		}

		// returning T from pop_front(), so the element can be
		// retrieved and removed atomically.  Using front() here would be perilous.
		std::optional<T> pop_front() {
			listnode *next;
			listnode *temp;

			// since nodes are only added to and removed from the front,
			// (ie, before any other nodes),
			// can get away with using a single CAS, no backtracking needed
			do {
				temp = start;

				if (!temp) {
					// list might be empty while here, return emptyhanded
					return {};
				}

				next = temp->next;
			} while (!start.compare_exchange_weak(temp, next));

			T ret = temp->element;
			delete temp;
			numelems--;
			return ret;
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
