/* ---------------------------------------------------------------- *\
 * voxfile_parser.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-09-21
\* ---------------------------------------------------------------- */

#ifndef VOXFILE_PARSER
#define VOXFILE_PARSER

#include <fstream>
#include <string>
#include <cstring>

#include "tools.hpp"

#include "world.hpp"

namespace Anthrax
{

class World;

class VoxfileParser
{
public:
	VoxfileParser() {};
	VoxfileParser(World *world);
	~VoxfileParser();
	void parseFile(std::string filename);
private:
	void parseSizeXyziPair(std::ifstream &voxfile);

	World *world_;
};

} // namespace Anthrax

#endif // VOXFILE_PARSER
