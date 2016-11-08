/**
 * @brief MCP4541 arduino library
 * @author (github.com/) adiea
 * @version 1.0
 *
 * @section intro_sec Introduction
 * This class is designed to allow user to use MCP4541 digital potentiometer.
 *
 * @section license_sec License
 *  This program is free software: you can redistribute it and/or modify\n
 *  it under the terms of the GNU General Public License as published by\n
 *  the Free Software Foundation, either version 3 of the License, or\n
 *  (at your option) any later version.\n
 * \n
 *  This program is distributed in the hope that it will be useful,\n
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of\n
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n
 *  GNU General Public License for more details.\n
 * \n
 *  You should have received a copy of the GNU General Public License\n
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.\n
 *
 * @section other_sec Others notes and compatibility warning
 * Tested together with SMING framework on ESP8266. Not tested with an actual arduino
 */
 
#ifndef MCP4541_H
#define MCP4541_H

class MCP4541
{
public:
	MCP4541();
	
	/**
	 * Start the I2C device and store chip address
	 */
	void begin(uint8_t address = 0x5C);
	
	/* set W position */
	void setCursorPos(uint8_t pos);
	
	/*disconnect W */
	void disconnectCursor();
	
	
protected:
	/** MCP4541 I2C address */
	uint8_t _address;
	
	uint8_t cursorPos;
}

#endif /*MCP4541*/