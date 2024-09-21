#ifndef __C_ARRAY_HPP__
#define __C_ARRAY_HPP__

#include "../defines.h"
#include <cstring>

template <typename T, u32 m_size>
struct carray
{
    private:
    T *m_data;

    public:
    CARBON_FORCE_INLINE u32 size() const {
        return m_size;
    }

    constexpr CARBON_FORCE_INLINE u32 length() const {
        return m_size;
    }

    CARBON_FORCE_INLINE T *data() const {
        return m_data;
    }

    CARBON_FORCE_INLINE T &at(u32 index) const {
        if (index >= m_size)
            LOG_ERROR("out of range at() in carray");
        return m_data[index];
    }

    CARBON_FORCE_INLINE T *begin() const {
        return m_data;
    }

    CARBON_FORCE_INLINE T *end() const {
        return m_data + m_size;
    }

    carray(void) : m_data((T *)malloc(sizeof(T) * m_size)){}

    void clear() {
        memset(m_data, 0, m_size);
    }
};

#endif