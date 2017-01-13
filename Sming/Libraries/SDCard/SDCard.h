/*
Author: (github.com/)ADiea
Project: Sming for ESP8266 - https://github.com/anakod/Sming
License: MIT
Date: 15.07.2015
Descr: Low-level SDCard functions
*/
#ifndef _SD_CARD_
#define _SD_CARD_

#include <SmingCore.h>
#include "SPIBase.h"

#define SDCARD_DEBUG_VERBOSE 0

//Provide either the chip select pin for simple setups
//or a custom delegate that controls the chip select from application code
void SDCard_begin(uint8 PIN_CARD_SS, SPIDelegateCS customCSDelegate = nullptr);

/** @brief SDChipSelect: Offers flexibility to control
 *                       chip select/release from user code.
 */

extern SPIBase	*SDCardSPI;

#endif /*_SD_CARD_*/
