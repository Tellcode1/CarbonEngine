#ifndef __C_HASHMAP_HPP__
#define __C_HASHMAP_HPP__

#include "defines.h"
#include "types/coptional.hpp"

/* FNV-1a Hash function */
template <typename key>
struct cdefault_hash {
    static u32 hash(const key &k) {
        constexpr int FNV_PRIME = 16777619;
        constexpr int OFFSET_BASIS = 2166136261;

        u32 hash = OFFSET_BASIS;
        const uchar *data = (const uchar *)(&k);
        for (u8 byte = 0; byte < sizeof(k); byte++) {
            hash ^= data[byte]; // xor
            hash *= FNV_PRIME;
        }
        return hash;
    };
};

// Honestly, reddit is writing all the code for me. I'm not complaining :>
constexpr inline unsigned next_pott(unsigned n) {
    if (n == 0) return 1;
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

template<typename key, typename value, typename hash_fn = cdefault_hash<key>>
struct chashmap {
    public:
    struct cht_node {
        key k;
        value v;
        bool is_occupied;
    };

    chashmap(int capacity = 16) 
        : entries(next_pott(capacity)), count(0) {
        nodes = (cht_node **)malloc(entries * sizeof(cht_node));
        memset(nodes, 0, entries * sizeof(cht_node));
    }

    ~chashmap() {
        if (!nodes)
            return;
        for (int i = 0; i < entries; i++)
            if (nodes[i])
                free(nodes[i]);
        free(nodes);
    }

    void insert(const key& k, const value& v) {
        assert(nodes != nullptr);
        if (count >= entries * 0.7f)
            resize(entries * 2);

        int index = hash_fn::hash(k) & (entries - 1);
        while (nodes[index] != nullptr && nodes[index]->is_occupied) {
            index = (index + 1) & (entries - 1);
        }
        
        if (!nodes[index])
            nodes[index] = (cht_node *)malloc(sizeof(cht_node));

        // Respect the assignment operator gods
        nodes[index]->k = k;
        nodes[index]->v = v;
        nodes[index]->is_occupied = true;
        count++;
    }

    coptional<value> find(const key& k) const {
        coptional<value> retval = nullopt;
        int index = hash_fn::hash(k) & (entries - 1);
        while (nodes[index]) {
            if (nodes[index]->is_occupied && nodes[index]->k == k) // you have to use == instead of memcmp because cstring and other structs
                retval.set(nodes[index]->v);
            index = (index + 1) & (entries - 1);
        }
        return retval;
    }

    value &at(const key &k) const {
        int index = hash_fn::hash(k) & (entries - 1);
        while (true) {
        cht_node *node = nodes[index];
            if (node->is_occupied && node->k == k)
                return node->v;
            index = (index + 1) & (entries - 1);
        }
    }
    private:

    void resize(int new_capacity) {
        new_capacity = next_pott(new_capacity);
        cht_node** old_nodes = nodes;
        int old_entries = entries;

        entries = new_capacity;
        count = 0;
        nodes = (cht_node **)malloc(entries * sizeof(cht_node));
        memset(nodes, 0, entries * sizeof(cht_node));

        for (int i = 0; i < old_entries; i++) {
            if (old_nodes[i] != nullptr && old_nodes[i]->is_occupied) {
                insert(old_nodes[i]->k, old_nodes[i]->v);
                free(old_nodes[i]);
            }
        }
        free(old_nodes);
    }

    cht_node **nodes;
    int entries;
    int count;
};

#endif//__C_HASHMAP_HPP__