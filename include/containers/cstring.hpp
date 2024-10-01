// utf32 string, 4 bytes per char

#ifndef __C_STRING_HPP__
#define __C_STRING_HPP__

#include "cvector.hpp"

typedef char32_t unicode;
struct cstring_view;

template <typename char_type>
constexpr inline static u32 ustrlen(const char_type* str) {
    const char_type* s = str;
    for (; *s; ++s) {}
    return s - str;
}

template <typename char_type>
struct cstring_base;

struct cstring_view;

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

    // operator cstring_base<unicode>();

    constexpr unicode operator[](u32 pos) const {
        if (pos >= m_count)
            LOG_ERROR("attempt to access at index %u in cstring_view of size %u", pos, m_count);
        return m_data[pos];
    }

    constexpr cstring_view substr(u32 pos, u32 len) const {
        if (pos > m_count)
            LOG_ERROR("Trying to substr at pos %u >= size %u", pos, m_count);
        return cstring_view(m_data + pos, cmmin(len, m_count - pos));
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

    constexpr cstring_view &operator =(const cstring_base<unicode> &other);

    private:
    const unicode* m_data;
    u32 m_count;
};

template <typename char_type>
struct cstring_base : public cvector<char_type> {
    using cvector<char_type>::m_data;
    using cvector<char_type>::begin;
    using cvector<char_type>::end;
    using cvector<char_type>::npos;
    using cvector<char_type>::m_count;
    using cvector<char_type>::push_back;
    using cvector<char_type>::push_set;
    using cvector<char_type>::reallocate;
    using cvector<char_type>::size;
    using cvector<char_type>::max_capacity;
    using cvector<char_type>::m_capacity;
    using base = cvector<char_type>;

    constexpr inline const char_type* c_str() const noexcept {
        return this->m_data;
    }

    constexpr inline cstring_base &operator +=(const char_type &c) {
        this->push_back(c);
        return *this;
    }

    constexpr inline void push_set(const char_type *str, int len) {
        if ((m_count + len + 1) >= max_capacity())
            reallocate(m_count + len + 1);
        memcpy(&m_data[m_count], str, len * sizeof(char_type));
        m_count += len;
        m_data[m_count] = U'\0';
    }

    constexpr inline void push_set(const char_type *str) {
        this->push_set(str, ustrlen(str));
    }

    constexpr inline cstring_base &operator +=(const char_type *str) {
        this->push_set(str);
        return *this;
    }

    constexpr inline cstring_base &operator +=(const cstring_base &other) {
        if (this != &other && other.size() > 0) {
            base::push_set(other.data(), other.size());
            m_data[m_count] = U'\0';
        }
        return *this;
    }

    void push_back(const char_type &c) {
        base::push_back(c);
        m_data[m_count] = U'\0';
    }

    void pop_back() {
        m_count--;
        m_data[m_count] = U'\0';
    }

    constexpr inline cstring_base &operator =(const char_type *str) {
        const u32 other_len = ustrlen(str);
        if ((other_len + 1) > this->max_capacity())
            reallocate(other_len + 1);
        memcpy(m_data + m_count, str, (other_len) * sizeof(char_type));
        m_count += other_len;
        m_data[m_count] = U'\0';
        return *this;
    }

    cstring_base(const cstring_base& other) {
        push_set(other.data(), other.size());
        m_data[m_count] = U'\0';
    }

    cstring_base& operator=(const cstring_base& other) {
        if (this != &other) {
            if (m_data) {
                free(m_data);
                m_data = nullptr;
            }
            m_count = other.m_count;
            reallocate(other.m_capacity);
            memcpy(m_data, other.m_data, m_count * sizeof(char_type));
        }
        return *this;
    }

    cstring_base& operator=(cstring_base&& other) noexcept {
        if (this != &other && other.size() > 0) {
            if (m_data)
                free(m_data);
            m_data = other.m_data;
            m_count = other.m_count;
            m_capacity = other.m_capacity;

            other.clear();
        }
        return *this;
    }

    bool operator ==(const cstring_base &other) const {
        if (this->size() != other.size())
            return false;
        return memcmp(this->data(), other.data(), this->size() * sizeof(char_type)) == 0;
    }

    constexpr inline cstring_base() {
        m_data = nullptr;
        m_count = 0;
        m_capacity = 0;
    }

    constexpr inline cstring_base(const char_type *begin, const char_type *end) {
        m_data = nullptr;
        m_count = 0;
        m_capacity = 0;

        const u32 len = end - begin;
        reallocate(len);
        memcpy(m_data, begin, len * sizeof(char_type));
        m_data[m_count] = U'\0';
    }

    constexpr inline cstring_base(const char_type *str) {
        m_data = nullptr;
        m_count = 0;
        m_capacity = 0;

        const u32 len = ustrlen(str);
        reallocate(len + 1);
        m_count = len;
        m_data[m_count] = U'\0';
        memcpy(m_data, str, len * sizeof(char_type));
    }

    constexpr inline cstring_base(const char_type *str, u32 len) {
        m_data = nullptr;
        m_count = 0;
        m_capacity = 0;
        reallocate(len);
        memcpy(m_data, str, len * sizeof(char_type));
        m_count = len;
        m_data[m_count] = U'\0';
    }

    constexpr inline operator cstring_view() {
        return cstring_view(this->data(), this->size());
    }

    constexpr inline u32 find(const char_type &c, u32 begin = 0) const {
        if (begin >= m_count)
            return npos;
        for (u32 i = begin; i < m_count; i++) {
            if (m_data[i] == c)
                return i;
        }
        return npos;
    }

    constexpr inline u32 rfind(const char_type &c) const {
        if (m_count == 0)
            return npos;

        const char_type * iter = end()-1;
        for (u32 i = m_count - 1; iter != begin()-1; iter--, i--) {
            if ((*iter) == c)
                return i;
        }

        return npos;
    }

    inline cstring_base substr(u32 pos, u32 len) const {
        if ((len == 0) || (pos > m_count)) {
            LOG_ERROR("out of range substr");
            return cstring_base();
        }

        if (pos + len > m_count)
            len = m_count - pos;

        cstring_base result;
        result.reallocate(len + 1);
        memcpy(result.m_data, m_data + pos, len * sizeof(char_type));
        result.m_count = len;
        result.m_data[len] = U'\0';
        return result;
    }
};

typedef cstring_base<unicode> cstring;
typedef cstring_base<char> crawstring;

constexpr cstring_view &cstring_view::operator=(const cstring_base<unicode> &other)
{
    m_data = other.data();
    m_count = other.length();
    return *this;
}

#endif//__C_STRING_HPP__