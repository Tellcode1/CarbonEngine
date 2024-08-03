// utf32 string, 4 bytes per char

#ifndef __C_STRING_HPP__
#define __C_STRING_HPP__

#include "containers/cvector.hpp"

typedef char32_t unicode;
struct cstring;
struct cstring_view;

constexpr inline static u32 ustrlen(const unicode* str) {
    const unicode* s = str;
    for (; *s != U'\0'; s++) {}
    return s - str;
}

// if you're wondering why everything is const. It's because string_view is just supposed to do that.
struct cstring_view {
    public:
    constexpr static u32 npos = UINT32_MAX;

    constexpr inline cstring_view() noexcept
        : m_data(nullptr), m_count(0) {}

    constexpr inline cstring_view(const unicode* str) noexcept
        : m_data(str), m_count(ustrlen(str)) {}

    constexpr inline cstring_view(const unicode* str, u32 size) noexcept
        : m_data(str), m_count(size) {}

    constexpr inline u32 size() const noexcept {
        return m_count;
    }

    constexpr inline u32 length() const noexcept {
        return m_count;
    }

    constexpr inline const unicode* data() const noexcept {
        return m_data;
    }

    constexpr inline const unicode* begin() const noexcept {
        return m_data;
    }

    constexpr inline const unicode* end() const noexcept {
        return m_data + m_count;
    }

    constexpr inline const unicode* c_str() const noexcept {
        return m_data;
    }

    constexpr inline bool empty() const noexcept {
        return m_count == 0;
    }

    operator cstring();

    constexpr unicode operator[](u32 pos) const {
        if (pos >= m_count)
            LOG_ERROR("attempt to access at index %u in cstring_view of size %u", pos, m_count);
        return m_data[pos];
    }

    constexpr cstring_view substr(u32 pos, u32 len) const {
        if (pos > m_count)
            LOG_ERROR("Trying to substr at pos %u >= size %u", pos, m_count);
        return cstring_view(m_data + pos, std::min(len, m_count - pos));
    }

    constexpr inline u32 find(const unicode &c, u32 begin = 0) const {
        if (begin >= m_count)
            return npos;

        for (u32 i = begin; i < m_count; i++) {
            if (m_data[i] == c)
                return i;
        }
        return npos;
    }

    constexpr inline u32 rfind(const unicode &c) const {
        u32 i = size();
        for (auto it = this->end(); it != this->begin();) {
            it--;
            if (*it == c)
                return i - 1;
            i--;
        }
        return npos;
    }

    private:
    const unicode* m_data;
    u32 m_count;
};

struct cstring : public cvector<unicode> {

    constexpr inline u32 size() const noexcept {
        return m_count;
    }

    constexpr inline u32 length() const noexcept {
        return m_count;
    }

    constexpr inline const unicode* data() const noexcept {
        return m_data;
    }

    constexpr inline const unicode* begin() const noexcept {
        return m_data;
    }

    constexpr inline const unicode* end() const noexcept {
        return m_data + m_count;
    }

    constexpr inline const unicode* c_str() const noexcept {
        return m_data;
    }

    constexpr inline bool empty() const noexcept {
        return m_count == 0;
    }

    constexpr inline cstring &operator +=(const unicode &c) {
        push_back(c);
        return *this;
    }

    constexpr inline cstring &operator +=(const unicode *str) {
        u32 other_len = ustrlen(str);
        if ((size() + other_len) >= (this->max_capacity()))
            reallocate(size() + other_len);
        memcpy(m_data + m_count, str, other_len * sizeof(unicode));
        m_count += other_len;
        m_data[m_count] = U'\0';
        return *this;
    }

    constexpr inline cstring &operator +=(const cstring &other) {
        if ((size() + other.size()) >= (this->max_capacity()))
            reallocate(size() + other.size());
        memcpy(m_data + m_count, other.data(), other.size() * sizeof(unicode));
        m_count += other.size();
        return *this;
    }

    void push_back(const unicode &c) {
        if ((m_count + 1) >= m_capacity)
            reallocate(std::max(1U, m_capacity * 2U));

        m_data[m_count] = c;
        m_count++;
        m_data[m_count] = U'\0';
    }

    void pop_back() {
        m_count--;
        m_data[m_count] = U'\0';
    }

    constexpr inline cstring &operator =(const unicode *str) {
        const u32 other_len = ustrlen(str);
        if ((other_len - 1) > this->max_capacity())
            reallocate(other_len - 1);
        memcpy(m_data + m_count, str, (other_len) * sizeof(unicode));
        m_count += (other_len - 1);
        return *this;
    }

    cstring(const cstring& other) {
        reallocate(other.m_capacity);
        m_count = other.m_count;
        memcpy(m_data, other.m_data, m_count * sizeof(unicode));
        m_data[m_count] = U'\0';
    }

    cstring& operator=(const cstring& other) {
        if (this != &other) {
            if (m_data)
                free(m_data);
            m_count = other.m_count;
            reallocate(other.m_capacity);
            memcpy(m_data, other.m_data, m_count * sizeof(unicode));
        }
        return *this;
    }

    cstring& operator=(cstring&& other) noexcept {
        if (this != &other && other.size() > 0) {
            if (m_data)
                free(m_data);
            m_data = other.m_data;
            m_count = other.m_count;
            m_capacity = other.m_capacity;

            other.m_data = nullptr;
            other.m_count = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    constexpr inline cstring() {
        m_data = nullptr;
        m_count = 0;
        m_capacity = 0;
    }

    constexpr inline cstring(const unicode *begin, const unicode *end) {
        m_data = nullptr;
        m_count = 0;
        m_capacity = 0;

        const u32 len = end - begin;
        reallocate(len);
        memcpy(m_data, begin, len * sizeof(unicode));
        m_data[m_count] = U'\0';
    }

    constexpr inline cstring(const unicode *str) {
        m_data = nullptr;
        m_count = 0;
        m_capacity = 0;

        const u32 len = ustrlen(str);
        reallocate(len + 1);
        m_count = len;
        m_data[m_count] = U'\0';
        memcpy(m_data, str, len * sizeof(unicode));
    }

    constexpr inline cstring(const unicode *str, u32 len) {
        m_data = nullptr;
        m_count = 0;
        m_capacity = 0;
        m_count = len;
        reallocate(len);
        memcpy(m_data, str, len * sizeof(unicode));
        m_data[m_count] = U'\0';
    }

    constexpr inline operator cstring_view() {
        return cstring_view(this->data(), this->size());
    }

    constexpr inline u32 find(const unicode &c, u32 begin = 0) const {
        if (begin >= m_count)
            return npos;
        for (u32 i = begin; i < m_count; i++) {
            if (m_data[i] == c)
                return i;
        }
        return npos;
    }

    // constexpr inline u32 rfind(const unicode &c, u32 begin = npos) const {
    //     if (begin != npos && (this->begin() + begin) >= this->end())
    //         return npos;

    //     u32 i = (begin == npos) ? this->size() - 1 : begin;
    //     for (auto iter = this->begin() + i; iter >= this->begin(); iter--) {
    //         if (*iter == c)
    //             return iter - this->begin();
    //         if (iter == this->begin()) break;
    //     }
    //     return npos;
    // }

    constexpr inline u32 rfind(const unicode &c) const {
        if (m_count == 0)
            return npos;

        u32 i = m_count - 1;
        for (auto iter = end()-1; iter != begin()-1; iter--) {
            if ((*iter) == c)
                return i;
            i--;
        }
        return npos;
    }

    inline cstring substr(u32 pos, u32 len) const {
        if (pos > m_count)
            return cstring();

        if (pos + len > m_count)
            len = m_count - pos;

        cstring result;
        result.reallocate(len + 1);
        memcpy(result.m_data, m_data + pos, len * sizeof(unicode));
        result.m_count = len;
        result.m_data[result.m_count] = U'\0';
        return result;
    }
};

inline cstring_view::operator cstring() {
    return cstring(m_data, m_count);
}

#endif//__C_STRING_HPP__