#ifndef RDESTL_STRING_UTILS_H
#define RDESTL_STRING_UTILS_H

namespace rde
{
//-----------------------------------------------------------------------------
template<typename T> int strlen(const T* str)
{
	int len(0);
	while (*str++)
		++len;
	return len;
}

//-----------------------------------------------------------------------------
template<typename T> int strcompare(const T* s1, const T* s2, size_t len)
{
	for (/**/; len != 0; --len)
	{
		const T c1 = *s1;
		const T c2 = *s2;
		if (c1 != c2)
			return (c1 < c2 ? -1 : 1);

		++s1;
		++s2;
	}
	return 0;
}

//-----------------------------------------------------------------------------
template<typename T> int strcompare(const T* s1, const T* s2)
{
	while (*s1 && *s2)
	{
		const T c1 = *s1;
		const T c2 = *s2;
		if (c1 != c2)
			return (c1 < c2 ? -1 : 1);

		++s1;
		++s2;
	}
	if (*s1)
		return 1;
	if (*s2)
		return -1;
	return 0;
}

} // rde

#endif // RDESTL_STRING_UTILS_H
