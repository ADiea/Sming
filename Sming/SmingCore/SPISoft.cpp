/*
Author: (github.com/)ADiea
Project: Sming for ESP8266 - https://github.com/anakod/Sming
License: MIT
Date: 15.07.2015
Descr: Implement software SPI. To improve speed, GPIO16 is not supported(see Digital.cpp)
*/
#include "SPISoft.h"

#define GP_IN(pin)	((GPIO_REG_READ(GPIO_IN_ADDRESS)>>(pin)) & 1)
#define GP_OUT(pin, val) GPIO_REG_WRITE(((((val) != LOW) ? \
							GPIO_OUT_W1TS_ADDRESS : \
							GPIO_OUT_W1TC_ADDRESS)), ((uint16_t)1<<(pin)))
#define SCK_PULSE	GP_OUT(mCLK, HIGH); \
					fastDelay(m_delay); \
					GP_OUT(mCLK, LOW); \
					fastDelay(m_delay);

#define SCK_SPEED_REFERENCE 40000000

static inline void IRAM_ATTR fastDelay(unsigned d)
{
	volatile unsigned a=d;
	while(a)
	{
		--a;
	}
}

void SPISoft::begin()
{
	if(16 == mMISO || 16 == mMOSI || 16 == mCLK)
	{
		/*To be able to use fast/simple GPIO read/write GPIO16 is not supported*/
		debugf("SPISoft: GPIO 16 not supported");
		return;
	}

	pinMode(mCLK, OUTPUT);
	digitalWrite(mCLK, LOW);

	pinMode(mMISO, INPUT);
	digitalWrite(mMISO, HIGH);

	pinMode(mMOSI, OUTPUT);
}

void SPISoft::beginTransaction(SPISettings& mySettings)
{
	//Get the desired bus frequency of the peripheral and
	// convert it to soft delay cycles
	if(mySettings.getSpeed() > 0)
		m_delay = SCK_SPEED_REFERENCE / mySettings.getSpeed();

}

void SPISoft::transfer(uint8_t* buffer, uint32_t size)
{
	uint8_t d;
	uint8_t r;

	if(!buffer)
	{
		debugf("SPIS:xfer NULL ptr");
		return;
	}

	do {
		d = *buffer; r=0;

		GP_OUT(mMOSI, d & 0x80);	/* bit7 */
		r = 		 GP_IN(mMISO); //bit 7
		SCK_PULSE
		GP_OUT(mMOSI, d & 0x40);	/* bit6 */
		r = r << 1 | GP_IN(mMISO); //bit 6
		SCK_PULSE
		GP_OUT(mMOSI, d & 0x20);	/* bit5 */
		r = r << 1 | GP_IN(mMISO); //bit 5
		SCK_PULSE
		GP_OUT(mMOSI, d & 0x10);	/* bit4 */
		r = r << 1 | GP_IN(mMISO); //bit 4
		SCK_PULSE
		GP_OUT(mMOSI, d & 0x08);	/* bit3 */
		r = r << 1 | GP_IN(mMISO); //bit 3
		SCK_PULSE
		GP_OUT(mMOSI, d & 0x04);	/* bit2 */
		r = r << 1 | GP_IN(mMISO); //bit 2
		SCK_PULSE
		GP_OUT(mMOSI, d & 0x02);	/* bit1 */
		r = r << 1 | GP_IN(mMISO); //bit 1
		SCK_PULSE
		GP_OUT(mMOSI, d & 0x01);	/* bit0 */
		r = r << 1 | GP_IN(mMISO); //bit 0
		SCK_PULSE

		*buffer++ = r;
	} while (--size);
}

