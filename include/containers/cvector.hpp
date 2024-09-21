
#ifndef __CARRAY_HPP__
#define __CARRAY_HPP__

#include "../defines.h"
#include "../stdafx.h"
#include "../math/math.h"

#include <cstdlib>
#include <initializer_list>

template<typename T>
struct cvector
{
    protected:
    u32 m_count;
    u32 m_capacity;
    T *m_data;

    public:

    constexpr static u32 npos = -1;

    constexpr CARBON_FORCE_INLINE u32 size() const {
        return m_count;
    }

    constexpr CARBON_FORCE_INLINE u32 length() const {
        return m_count;
    }

    constexpr CARBON_FORCE_INLINE u32 max_capacity() const {
        return m_capacity;
    }

    constexpr CARBON_FORCE_INLINE T *data() const {
        return m_data;
    }

    constexpr CARBON_FORCE_INLINE T *begin() const {
        return m_data;
    }

    constexpr CARBON_FORCE_INLINE T *end() const {
        return m_data + m_count;
    }

    constexpr CARBON_FORCE_INLINE bool empty() const {
        return m_count == 0;
    }

    T &front() const noexcept {
        return m_data[0];
    }

    T &back() const noexcept {
        return m_data[m_count];
    }

    constexpr void reallocate(u32 new_capacity) {
        void *new_data = malloc(sizeof(T) * new_capacity);
        if (m_data) {
            memcpy(new_data, m_data, sizeof(T) * m_capacity);
            free(m_data);
        }
        m_capacity = new_capacity;
        m_data = reinterpret_cast<T*>(new_data);
    }

    constexpr CARBON_FORCE_INLINE void reserve(u32 amt) {
        reallocate(this->max_capacity() + amt);
    }

    constexpr cvector() : m_count(0), m_capacity(0), m_data(nullptr) {}
    constexpr cvector(u32 init_capacity) : m_count(0), m_capacity(0), m_data(nullptr) { reserve(init_capacity); }
    
    constexpr cvector(const std::initializer_list<T>& init_list) : m_count(0), m_capacity(0), m_data(nullptr) {
        reallocate(init_list.size());
        memcpy(m_data, init_list.begin(), init_list.size() * sizeof(T));
        m_count = init_list.size();
    }

    ~cvector() {
        if (m_data)
            free(m_data);
    }

    public:

    // copies an element into array
    constexpr void push_back(const T& element) {
        if ((m_count + 1) >= m_capacity)
            reallocate(cmmax(1, m_capacity * 2));
        memcpy(&m_data[m_count], &element, sizeof(T));
        m_count++;
    }

    // copies multiple elements into the array
    constexpr void push_set(const T *elements, u32 count) {
        u32 required_capacity = m_count + count;
        if (required_capacity >= max_capacity())
            reallocate(required_capacity);
        memcpy(&m_data[m_count], elements, count * sizeof(T));
        m_count += count;
    }

    constexpr void pop_back() {
        if (m_count > 0)
            m_count--;
    }

    // WARNING!!! Has to move all the elements in the array back so shouldn't be used!
    constexpr void pop_front() {
        if (m_count > 0) {
            m_count--;
            memcpy(m_data, m_data + 1, m_count * sizeof(T));
        }
    }

    constexpr cvector<T> & operator=(const cvector<T> &other) {
        if (&other == this)
            return *this;
        if (size() < other.size())
            reallocate(other.size());
        memcpy(m_data, other.data(), other.size() * sizeof(T));
        m_count = other.size();
        return *this;
    }

    constexpr void insert(u32 index, const T& element) {
        if (index >= m_capacity)
            reallocate(cmmax(1, index * 2)); // linear allocator. i could probably implement more but i don't have time rn
        if (index >= m_count)
            m_count = index + 1;
        m_data[index] = element;
    }

    constexpr void remove(u32 index) {
        if (index >= m_count)
            LOG_ERROR(
                "Attempt to delete element at index %u but array holds only %u elements?", index, m_count
            );
        
        if (m_count - index - 1)
            memcpy(m_data + index, m_data + (index + 1), (m_count - index - 1) * sizeof(T)); // please don't ask me what this is
        
        m_count--;
    }

    CARBON_NO_DISCARD constexpr T& at(u32 index) const {
        return m_data[index];
    }

    constexpr CARBON_FORCE_INLINE T& operator [](int index) const {
        return m_data[index];
    }

    constexpr CARBON_FORCE_INLINE void clear() {
        if (m_data && m_count > 0)
            free(m_data);
        m_data = nullptr;
        m_count = 0;
        m_capacity = 0;
    }

    constexpr CARBON_FORCE_INLINE void reset() {
        m_count = 0;
    }

    constexpr CARBON_FORCE_INLINE void resize(u32 new_size) {
        if (new_size == 0)
            LOG_ERROR("Request array resize to 0. Use clear() instead.");

        T *new_data = (T *)malloc(sizeof(T) * new_size);

        u32 move_size = cmmin(new_size, m_count) * sizeof(T);

        if (m_data) {
            memcpy(new_data, m_data, move_size);
            free(m_data);
        }

        m_data = new_data;
        m_count =  new_size;
        m_capacity = new_size;
    }

    // Finds and returns the first occurence of the element in the array, cvector::npos if doesn't exist
    CARBON_NO_DISCARD constexpr u32 find(const T &element) const {
        for (u32 i = 0; i < m_count; i++)
            if (m_data[i] == element)
                return i;
        return npos;
    }

    CARBON_NO_DISCARD CARBON_FORCE_INLINE constexpr u32 rfind(const T &element) const {
        u32 i = size();
        for (const T *it = this->end(); it != this->begin();) {
            it--;
            if (*it == element)
                return i - 1;
            i--;
        }
        return npos;
    }
};

#endif