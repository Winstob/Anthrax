/* ---------------------------------------------------------------- *\
 * character.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-02-26
\* ---------------------------------------------------------------- */
#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include <glm/glm.hpp>

namespace Anthrax
{

class Character
{
public:
  Character() : Character(0, glm::ivec2(0), glm::ivec2(0), 0) {}
  Character(unsigned int t, glm::ivec2 s, glm::ivec2 b, unsigned int a) : TextureID(t), Size(s), Bearing(b), Advance(a) {}
  unsigned int TextureID;  // ID handle of the glyph texture
  glm::ivec2   Size;       // Size of glyph
  glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
  unsigned int Advance;    // Offset to advance to next glyph
};

} // namespace Anthrax

#endif // CHARACTER_HPP
