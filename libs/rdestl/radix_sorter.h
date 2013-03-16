#ifndef RDESTL_RADIX_SORTER_H
#define RDESTL_RADIX_SORTER_H

#include "vector.h"

namespace rde
{
template<typename T>
class radix_sorter
{
public:
	static const size_t kHistogramSize	= 256;

	enum data_type
	{
		data_unsigned = false,
		data_signed,
	};

	// User-provided temporary buffer for intermediate operations.
	// It has to be at least 'num' elements big.
	template<data_type TDataType, typename TFunc>
	void sort(T* src, int num, const TFunc& func, T* help_buffer)
	{
		if (num == 0)
			return;

		uint32 histogram[kHistogramSize * 4];
		Sys::MemSet(histogram, 0, sizeof(histogram));

		uint32* h1 = &histogram[0] + kHistogramSize;
		uint32* h2 = h1 + kHistogramSize;
		uint32* h3 = h2 + kHistogramSize;

		bool alreadySorted(true);
		uint32 prevValue = func(src[0]);

		int k(0);
		for (int i = 0; i < num; ++i)
		{
			k = i;
			const uint32 x = func(src[i]);
			if (alreadySorted && x < prevValue)
			{
				alreadySorted = false;
				break;
			}
			const uint8* px = (const uint8*)&x;

			++histogram[*px];
			++h1[px[1]];
			++h2[px[2]];
			++h3[px[3]];

			prevValue = x;
		}
		if (alreadySorted)
			return;

		for (; k < num; ++k)
		{
			const uint32 x = func(src[k]);
			const uint8* px = (const uint8*)&x;

			++histogram[*px];
			++h1[px[1]];
			++h2[px[2]];
			++h3[px[3]];
		}

		// 3rd byte == 0
		const bool canBreakAfter16Bits = (h2[0] == (uint32)num && h3[0] == (uint32)num);
		(void)canBreakAfter16Bits;

		if (TDataType == data_signed)
			calculate_offsets_signed(histogram);
		else
			calculate_offsets(histogram);

		for (int i = 0; i < num; ++i)
		{
			const uint32 pos = func(src[i]) & 0xFF;
			help_buffer[histogram[pos]++] = src[i];
		}
		for (int i = 0; i < num; ++i)
		{
			const uint32 pos = (func(m_dst[i]) >> 8) & 0xFF;
			src[h1[pos]++] = help_buffer[i];
		}
		if (TDataType == data_unsigned && canBreakAfter16Bits)
			return;
		for (int i = 0; i < num; ++i)
		{
			const uint32 pos = (func(src[i]) >> 16) & 0xFF;
			help_buffer[h2[pos]++] = src[i];
		}
		for (int i = 0; i < num; ++i)
		{
			const uint32 pos = (func(m_dst[i]) >> 24) & 0xFF;
			src[h3[pos]++] = help_buffer[i];
		}
	}

	// Version that uses internal buffer.
	template<data_type TDataType, typename TFunc>
	void sort(T* src, int num, const TFunc& func)
	{
		if (num > m_dst.size())
			resize(num);
		sort<TDataType, TFunc>(src, num, func, m_dst.begin());
	}		

private:
	void resize(int num)
	{
		m_dst.resize(num);
	}
	void calculate_offsets(uint32* histogram)
	{
		uint32 offsets[4] = { 1, 1, 1, 1 };
		for (int i = 0; i < kHistogramSize; ++i)
		{
			uint32 temp = histogram[i] + offsets[0];
			histogram[i] = offsets[0] - 1;
			offsets[0] = temp;

			temp = histogram[i + kHistogramSize] + offsets[1];
			histogram[i + kHistogramSize] = offsets[1] - 1;
			offsets[1] = temp;

			temp = histogram[i + kHistogramSize*2] + offsets[2];
			histogram[i + kHistogramSize*2] = offsets[2] - 1;
			offsets[2] = temp;

			temp = histogram[i + kHistogramSize*3] + offsets[3];
			histogram[i + kHistogramSize*3] = offsets[3] - 1;
			offsets[3] = temp;
		}
	}
	void calculate_offsets_signed(uint32* histogram)
	{
		uint32 offsets[4] = { 1, 1, 1, 1 };
		int numNeg(0);
		for (int i = 0; i < kHistogramSize; ++i)
		{
			uint32 temp = histogram[i] + offsets[0];
			histogram[i] = offsets[0] - 1;
			offsets[0] = temp;

			temp = histogram[i + kHistogramSize] + offsets[1];
			histogram[i + kHistogramSize] = offsets[1] - 1;
			offsets[1] = temp;

			temp = histogram[i + kHistogramSize*2] + offsets[2];
			histogram[i + kHistogramSize*2] = offsets[2] - 1;
			offsets[2] = temp;

			if (i >= kHistogramSize/2)
				numNeg += histogram[i + kHistogramSize*3];
		}
		uint32* h3 = &histogram[kHistogramSize*3];
		offsets[3] = numNeg + 1;
		for (int i = 0; i < kHistogramSize / 2; ++i)
		{
			uint32 temp = h3[i] + offsets[3];
			h3[i] = offsets[3] - 1;
			offsets[3] = temp;
		}
		offsets[3] = 1;
		for (int i = kHistogramSize / 2; i < kHistogramSize; ++i)
		{
			uint32 temp = h3[i] + offsets[3];
			h3[i] = offsets[3] - 1;
			offsets[3] = temp;
		}
	}
	vector<T>	m_dst;
};
}

#endif
