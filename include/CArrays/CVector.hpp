#ifndef __CARRAY_HPP__
#define __CARRAY_HPP__

#include "defines.h"
#include <string.h>

template<typename T>
struct CVector
{
    private:
    u32 m_count;
    u32 m_capacity;
    T *m_data;

    public:
    CARBON_FORCE_INLINE u32 size() {
        return m_count;
    }

    CARBON_FORCE_INLINE u32 length() {
        return m_count;
    }

    CARBON_FORCE_INLINE T *data() {
        return m_data;
    }

    void reallocate(u32 new_capacity) {
        void *new_data = malloc(sizeof(T) * new_capacity);
        memmove(new_data, m_data, sizeof(T) * m_capacity);
        m_capacity = new_capacity;
        m_data = reinterpret_cast<T*>(new_data);
    }

    CVector() : m_count(0), m_data(nullptr) {}
    CVector(u32 element_count) : m_count(element_count) { reallocate(element_count); }
    
    CVector(const std::initializer_list<T>& init_list) : m_count(init_list.size()) {
        reallocate(init_list.size());
        memcpy(m_data, init_list.begin(), m_count * sizeof(T));
    }

    void push_back(const T& element) {
        if ((m_count + 1) >= m_capacity)
            reallocate(std::max(1U, m_capacity * 2U));
        m_data[m_count] = element;
        m_count++;
    }

    void insert(u32 index, const T& element) {
        if (index >= m_capacity)
            reallocate(std::max(1U, index * 2U)); // linear allocator. i could probably implement more but i don't have time rn
        if (index >= m_count)
            m_count = index + 1;
        m_data[index] = element;
    }

    void remove(u32 index) {
        if (index >= m_count)
            LOG_ERROR(
                "Attempt to delete element at index %u but array holds only %u elements?", index, m_count
            );
        
        if (m_count - index - 1)
            memmove(m_data + index, m_data + (index + 1), (m_count - index - 1) * sizeof(T)); // please don't ask me what this is
        
        m_count--;
    }

    CARBON_NO_DISCARD T at(u32 index) {
        if (index >= m_count)
            LOG_ERROR(
                "Attempt to access element at index %u but array holds only %u elements?", index, m_count
            );
        return m_data[index];
    }

    // Finds and returns the first occurence of the element in the array, UINT32_MAX if doesn't exist
    CARBON_NO_DISCARD u32 find(const T& element) {
        for (u32 i = 0; i < m_count; i++)
            if (m_data[i] == element)
                return i;
        return UINT32_MAX;
    }

    CARBON_FORCE_INLINE void clear() {
        free(m_data);
        m_count = 0;
        m_capacity = 0;
    }
};

#endif