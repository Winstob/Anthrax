/* ---------------------------------------------------------------- *\
 * tools.cpp
 * Author: Gavin Ralston
 * Date Created: 2025-01-13
\* ---------------------------------------------------------------- */

#include "tools.hpp"

namespace Anthrax
{

unsigned int log2(unsigned int val)
{
	unsigned int ret = 0;
	while (val >> ret > 1)
	{
		ret++;
	}
	return ret;
}

} // namespace Anthrax
