#ifndef __C_LINKED_LIST_HPP__
#define __C_LINKED_LIST_HPP__

#include "defines.h"

/*
    Just a whole lot of shifting first and last
*/

template<typename T>
struct cdoubly_linked_list {

    /* doubly linked list node */
    struct cdll_node {
        cdll_node *prev;
        cdll_node *next;
        T data;
    };

    constexpr cdoubly_linked_list() : first(nullptr), last(nullptr) {}

    ~cdoubly_linked_list() {
        cdll_node *node = first;
        while (node) {
            cdll_node *next = node->next;
            free(node);
            node = next;
        }
    }

    constexpr cdll_node *get_first() const {
        return first;
    }

    constexpr cdll_node *get_last() const {
        return last;
    }

    constexpr T &front() const {
        return first->data;
    }

    constexpr T &back() const {
        return last->data;
    }

    constexpr void push_front(const T &data) {
        cdll_node *new_node = (cdll_node *)malloc(sizeof(cdll_node));
        new_node->prev = nullptr;
        new_node->next = first;
        new_node->data = data;

        if (first == nullptr) {
            first = new_node;
            last = new_node;
        } else {
            first->prev = new_node;
            first = new_node;
        }
    }

    constexpr void push_back(const T &data) {
        cdll_node *new_node = (cdll_node *)malloc(sizeof(cdll_node));
        new_node->prev = last;
        new_node->next = nullptr;
        new_node->data = data;

        if (last == nullptr) {
            first = new_node;
            last = new_node;
        } else {
            last->next = new_node;
            last = new_node;
        }
    }

    constexpr void pop_front() {
        if (first == nullptr) return;
        cdll_node *next = first->next;
        if (next != nullptr) {
            next->prev = nullptr;
        } else {
            last = nullptr;
        }
        free(first);
        first = next;
    }

    constexpr void pop_back() {
        if (last == nullptr) return;
        cdll_node *prev = last->prev;
        if (prev != nullptr) {
            prev->next = nullptr;
        } else {
            first = nullptr;
        }
        free(last);
        last = prev;
    }

    constexpr void insert_before(cdll_node *node, cdll_node *new_node) {
        new_node->next = node;
        new_node->prev = node->prev;

        if (node->prev == nullptr) {
            first = new_node;
        } else {
            node->prev->next = new_node;
        }
        node->prev = new_node;
    }

    constexpr void remove(cdll_node *node) {
        if (node->prev == nullptr) {
            first = node->next;
        } else {
            node->prev->next = node->next;
        }

        if (node->next == nullptr) {
            last = node->prev;
        } else {
            node->next->prev = node->prev;
        }

        free(node);
    }

    private:
    cdll_node *first;
    cdll_node *last;
};

template <typename T>
using cdeque = cdoubly_linked_list<T>;

#endif // __C_LINKED_LIST_HPP__