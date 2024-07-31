#ifndef __CARRAY_HPP__
#define __CARRAY_HPP__

#include "defines.h"
#include <string.h>

template<typename T>
struct cvector
{
    private:
    u32 m_count;
    u32 m_capacity;
    T *m_data;

    public:
    CARBON_FORCE_INLINE u32 size() const {
        return m_count;
    }

    CARBON_FORCE_INLINE u32 length() const {
        return m_count;
    }

    CARBON_FORCE_INLINE T *data() const {
        return m_data;
    }

    CARBON_FORCE_INLINE T *begin() const {
        return m_data;
    }

    CARBON_FORCE_INLINE T *end() const {
        return m_data + m_count;
    }

    CARBON_FORCE_INLINE bool empty() const {
        return m_count == 0;
    }

    void reallocate(u32 new_capacity) {
        void *new_data = malloc(sizeof(T) * new_capacity);
        if (m_data)
            memmove(new_data, m_data, sizeof(T) * m_capacity);
        m_capacity = new_capacity;
        m_data = reinterpret_cast<T*>(new_data);
    }

    cvector() : m_count(0), m_data(nullptr), m_capacity(0) {}
    cvector(u32 element_count) : m_count(0), m_data(nullptr) { reallocate(element_count); }
    
    cvector(const std::initializer_list<T>& init_list) : m_count(init_list.size()) {
        reallocate(init_list.size());
        memcpy(m_data, init_list.begin(), m_count * sizeof(T));
    }

    ~cvector() {
        free(m_data);
    }

    u32 push_back(const T& element) {
        if ((m_count + 1) >= m_capacity)
            reallocate(std::max(1U, m_capacity * 2U));
        // m_data[m_count] = element;
        memcpy(&m_data[m_count], &element, sizeof(T));
        m_count++;
        return m_count - 1;
    }

    constexpr cvector<T> & operator=(const T &other) {
        clear();
        push_back(other);
        return *this;
    }

    constexpr cvector<T> & operator=(const cvector<T> &other) {
        clear();
        reallocate(other.size());
        memcpy(m_data, other.data(), other.size() * sizeof(T));
        return *this;
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

    CARBON_NO_DISCARD T& at(u32 index) const {
        return m_data[index];
    }

    CARBON_FORCE_INLINE T& operator [](u64 index) const {
        return m_data[index];
    }

    // Finds and returns the first occurence of the element in the array, UINT32_MAX if doesn't exist
    CARBON_NO_DISCARD u32 find(const T& element) const {
        for (u32 i = 0; i < m_count; i++)
            if (m_data[i] == element)
                return i;
        return UINT32_MAX;
    }

    CARBON_FORCE_INLINE void clear() {
        free(m_data);
        m_data = nullptr;
        m_count = 0;
        m_capacity = 0;
    }

    CARBON_FORCE_INLINE void reset() {
        m_count = 0;
    }

    CARBON_FORCE_INLINE void resize(u32 new_size) {
        if (new_size == 0)
            LOG_ERROR("Request array resize to 0");

        T *new_data = (T *)malloc(sizeof(T) * new_size);

        u32 move_size = std::min(new_size, m_count) * sizeof(T);

        if (m_data) {
            memmove(new_data, m_data, move_size);
            free(m_data);
        }

        m_data = new_data;
        m_count =  new_size;
        m_capacity = new_size;
    }
};

#endif