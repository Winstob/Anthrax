/* ---------------------------------------------------------------- *\
 * tools.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-05-29
\* ---------------------------------------------------------------- */

#ifndef ANTHRAX_TOOLS_HPP
#define ANTHRAX_TOOLS_HPP


#define xstr(s) istr(s)
#define istr(s) #s

#define KB(x) ((size_t) (x) << 10)
#define MB(x) ((size_t) (x) << 20)
#define GB(x) ((size_t) (x) << 30)

#define PI 3.14159265358979

namespace Anthrax
{

unsigned int log2(unsigned int val);

}

#endif // ANTHRAX_TOOLS_HPP
