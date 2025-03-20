/* ---------------------------------------------------------------- *\
 * freelist.cpp
 * Author: Gavin Ralston
 * Date Created: 2025-01-12
\* ---------------------------------------------------------------- */

#include <iostream>

#include "freelist.hpp"

namespace Anthrax
{

Freelist::Freelist()
{
	freelist_ = std::vector<uint64_t>(0);
	return;
}


void Freelist::copy(const Freelist &other)
{
	freelist_ = other.freelist_;
	return;
}


size_t Freelist::alloc()
{
	size_t next_index = findNextFreeIndex();
	size_t bigindex = next_index >> 6;
	size_t subindex = next_index & 0x3F;
	freelist_[bigindex] |= (0x8000000000000000ull >> subindex);
	return next_index;
}


void Freelist::free(size_t index)
{
	size_t bigindex = index >> 6;
	size_t subindex = index & 0x3F;
	freelist_[bigindex] &= ~(0x8000000000000000ull >> subindex);
	return;
}


size_t Freelist::findNextFreeIndex()
{
	for (size_t index = 0; index < freelist_.size(); index++)
	{
		if (freelist_[index] != 0xFFFFFFFFFFFFFFFFull)
		{
			for (int subindex = 0; subindex < 64; subindex++)
			{
				if (((0x8000000000000000ull >> subindex) & freelist_[index]) == 0ull)
				{
					return ((index<<6) + subindex);
				}
			}

		}
	}
	freelist_.push_back(static_cast<uint64_t>(0));
	return (static_cast<size_t>(freelist_.size()-1)) << 6;
}


void Freelist::setRange(size_t offset, size_t num_elements, bool val)
{
	// TODO
}

} // namespace Anthrax
