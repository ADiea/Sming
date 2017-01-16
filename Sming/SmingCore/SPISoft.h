/*
Author: ADiea
Project: Sming for ESP8266
License: MIT
Date: 15.07.2015
Descr: Implement software SPI for HW configs other than hardware SPI pins(GPIO 12,13,14)
*/
#ifndef _SPI_SOFT_
#define _SPI_SOFT_

#include "SPIBase.h"
#include "SPISettings.h"


class SPISoft: public SPIBase {

public:

	SPISoft(uint16_t miso, uint16_t mosi, uint16_t sck, uint8_t delay = 0)
	{
		mMISO = miso;
		mMOSI = mosi;
		mCLK = sck;
		m_delay = delay;
	}

	virtual ~SPISoft() {};

	/**brief begin(): Initializes the SPI bus by setting SCK, MOSI to outputs,
	 *  pulling SCK and MOSI low, and SS high.
	 */
	virtual void begin();	//setup pins

	/*brief end(): Disables the SPI bus (leaving pin modes unchanged).
	 */
	virtual void end() {};

	/*brief beginTransaction(): Initializes the SPI bus using the defined SPISettings.
	 */
	virtual void beginTransaction(SPISettings& mySettings);

	/*brief endTransaction(): Stop using the SPI bus. Normally this is called after de-asserting the chip select, to allow other libraries to use the SPI bus.
	 */
	virtual void endTransaction(){}

	/*brief transfer(), transfer16()
	 *
	 * SPI transfer is based on a simultaneous send and receive: the received data is returned in receivedVal (or receivedVal16). In case of buffer transfers the received data is stored in the buffer in-place (the old data is replaced with the data received).
	 *
	 * 		receivedVal = SPI.transfer(val)
	 * 		receivedVal16 = SPI.transfer16(val16)
	 * 		SPI.transfer(buffer, size)
	 */
	void transfer(uint8 * buffer, size_t size);
	unsigned char transfer(unsigned char val) {transfer(&val, 1); return val;};
	unsigned short transfer16(unsigned short val) {transfer((uint8 *)&val, 2); return val;};

private:
	uint16_t mMISO, mMOSI, mCLK;
	uint8_t m_delay;
};

#endif /*_SPI_SOFT_*/
