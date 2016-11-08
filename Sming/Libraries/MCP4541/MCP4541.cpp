/*
 * See header file for details
 *
 *  This program is free software: you can redistribute it and/or modify\n
 *  it under the terms of the GNU General Public License as published by\n
 *  the Free Software Foundation, either version 3 of the License, or\n
 *  (at your option) any later version.\n
 * 
 *  This program is distributed in the hope that it will be useful,\n
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of\n
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n
 *  GNU General Public License for more details.\n
 * 
 *  You should have received a copy of the GNU General Public License\n
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.\n
 */

/* Dependencies */
#include <Wire.h>
#include "MCP4541.h"

MCP4541::MCP4541():_address(0)
{

}

void MCP4541::begin(uint8_t address/* = 0x5C*/)
{
	_address = address;
}

void MCP4541::setCursorPos(uint8_t pos)
{
	
	
	
	cursorPos = pos;
}

void MCP4541::disconnectCursor()
{

}
	