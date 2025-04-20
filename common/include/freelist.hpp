/* ---------------------------------------------------------------- *\
 * freelist.hpp
 * Author: Gavin Ralston
 * Date Created: 2025-01-12
 *
 * I know it's backwards but 0 means free and 1 means taken.
\* ---------------------------------------------------------------- */
#ifndef ANTHRAX_FREELIST_HPP
#define ANTHRAX_FREELIST_HPP

#include <vector>
#include <cstdint>

namespace Anthrax
{

class Freelist
{
public:
	Freelist();
	Freelist &operator=(const Freelist &other) { copy(other); return *this; }
	Freelist(const Freelist &other) { copy(other); }
	void copy(const Freelist &other);
	size_t alloc();
	void free(size_t index);
	void setRange(size_t offset, size_t num_elements, bool val);
	void clear();

private:
	std::vector<uint64_t> freelist_;

	size_t findNextFreeIndex();
};

} // namespace Anthrax

#endif // ANTHRAX_FREELIST_HPP
