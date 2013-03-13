#ifndef RDESTL_FIXED_SUBSTRING_H
#define RDESTL_FIXED_SUBSTRING_H

#include "fixed_array.h"
#include "string_utils.h"

namespace rde
{
// "Substring" of fixed size.
// Cannot grow.
// Never allocates memory.
template<typename E, size_t N>
class fixed_substring : private fixed_array<E, N + 1>
{
	typedef fixed_array<E, N + 1>	Base;
public:
	typedef E	value_type;
	typedef int	size_type;

	fixed_substring()
	{
		data()[0] = value_type(0);
	}
	explicit fixed_substring(const value_type* str)
	{
		assign(str);
	}
	template<size_t M>
	fixed_substring(const fixed_substring<E, M>& rhs)
	{
		assign(rhs.data());
	}

	template<size_t M>
	fixed_substring& operator=(const fixed_substring<E, M>& rhs)
	{
		assign(rhs.data());
		return *this;
	}
	fixed_substring& operator=(const value_type* str)
	{
		assign(str);
		return *this;
	}

	using Base::operator[];
	using Base::data;

	// For rde::string compatibility.
	const char* c_str() const
	{
		return data();
	}

	void assign(const value_type* str)
	{
		RDE_ASSERT(str != data());
		const size_type len = (size_type)rde::strlen(str);
		const size_type toCopy = len < N ? len : N;
		Sys::MemCpy(data(), str, toCopy * sizeof(value_type));
		data()[toCopy] = value_type(0);
	}
	void assign(const fixed_substring& rhs)
	{
		assign(rhs.data());
	}

	void append(const value_type* str)
	{
		const size_type strLen = (size_type)rde::strlen(str);
		const size_type ourLen = (size_type)rde::strlen(data());
		size_type toAppend = strLen;
		if (ourLen + toAppend > N)
			toAppend = N - ourLen;
		Sys::MemCpy(data() + ourLen, str, toAppend * sizeof(value_type));
		data()[ourLen + toAppend] = value_type(0);
	}
	void append(const fixed_substring& rhs)
	{
		append(rhs.data());
	}

	size_type find_index_of(value_type ch) const
	{
		size_type retIndex(-1);
		const E* ptr = data();
		size_type currentIndex(0);
		while (*ptr != value_type(0))
		{
			if (*ptr == ch)
			{
				retIndex = currentIndex;
				break;
			}
			++ptr;
			++currentIndex;
		}
		return retIndex;
	}
	size_type find_index_of_last(value_type ch) const
	{
		size_type retIndex(-1);
		const value_type* ptr = data();
		size_type currentIndex(0);
		while (*ptr != value_type(0))
		{
			if (*ptr == ch)
				retIndex = currentIndex;
			++ptr;
			++currentIndex;
		}
		return retIndex;
	}

	// Removes all characters from [index, end) range.
	void trim_end(size_type index)
	{
		if (index >= 0)
		{
			RDE_ASSERT(index < (size_type)rde::strlen(data()));
			data()[index] = value_type(0);
		}
	}

	bool empty() const
	{
		// @todo: consider caching string len
		return length() == 0;
	}
	size_type length() const
	{
		return rde::strlen(data());
	}

	bool operator==(const fixed_substring& rhs) const
	{
		return rde::strcompare(data(), rhs.data()) == 0;
	}
	bool operator!=(const fixed_substring& rhs) const
	{
		return !(*this == rhs);
	}
};

} // rde

#endif // #ifndef RDESTL_FIXED_SUBSTRING_H
