#ifndef __C_OPTIONAL_HPP__
#define __C_OPTIONAL_HPP__

#include "../../include/defines.h"

template <typename Tp>
struct coptional;

struct nullopt_t { constexpr inline nullopt_t(int _) {} };
static const nullopt_t nullopt = nullopt_t(0);

template <typename Tp>
struct coptional {
    public:
    constexpr coptional() : m_has_value(false) {  }
    constexpr coptional(const Tp &val) : m_val(val), m_has_value(true) {  }
    constexpr coptional(const nullopt_t) : m_has_value(false) {  }

    constexpr bool has_value() const {
        return m_has_value;
    }

    constexpr const Tp &get() const {
        cassert(m_has_value);
        return m_val;
    }

    constexpr void set(const Tp &val) {
        m_val = val;
        m_has_value = true;
    }

    constexpr void reset() {
        m_has_value = false;
    }

    constexpr coptional& operator=(nullopt_t) {
        reset();
        return *this;
    }
    private:
    Tp m_val;
    bool m_has_value;
};

#endif//__C_OPTIONAL_HPP__