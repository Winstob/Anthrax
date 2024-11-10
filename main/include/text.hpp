/* ---------------------------------------------------------------- *\
 * text.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-02-27
\* ---------------------------------------------------------------- */
#ifndef TEXT_HPP
#define TEXT_HPP

#include <string>
#include <glm/glm.hpp>

namespace Anthrax
{

class Text
{
public:
	Text() : Text("", 0.0, 0.0, 0.0, glm::vec3(0.0)) {}
	Text(std::string t, float xi, float yi, float sc, glm::vec3 c) : text(t), x(xi), y(yi), scale(sc), color(c) {}
	Text& operator=(const Text& other)
	{
		text = other.text;
		x = other.x;
		y = other.y;
		scale = other.scale;
		color = other.color;
		return *this;
	}

	std::string text;
	float x, y, scale;
	glm::vec3 color;
};

} // namespace Anthrax

#endif // TEXT_HPP
