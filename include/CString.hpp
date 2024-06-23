// #ifndef __C_STRING_HPP__
// #define __C_STRING_HPP__

// #include "stdafx.hpp"

// struct CString
// {
// 	public:
// 	CString() = default;
// 	~CString() {
// 		if (m_str)
// 			delete[] m_str;
// 	}

// 	CString(const char* str) {
// 		m_length = strlen(str);
// 		m_str = new u32[m_length + 1];
// 		for (u32 i = 0; i < m_length; i++)
// 			m_str[i] = str[i];
// 		m_str[m_length] = 0;
// 	}

// 	constexpr inline  operator +=() {

// 	}

// 	constexpr inline u32 length() const { return m_length; }
// 	constexpr inline u32 size() const { return m_length; }

// 	constexpr inline u32* data() const { return m_str; }

// 	private:
// 	u32 m_length;
// 	u32* m_str;
// };

// #endif