#ifndef _SYS_CFG_H_
#define _SYS_CFG_H_


#include <Arduino.h>

#define _DEBUG_	1 // set to 0 for release version, 1 for basic output, 2 for detailed output

//#define Serial Serial3

//#define WDG_RST  asm("wdr")
#define WDG_RST 

#define USE_RS485 1

// Pin mapping:
// ---------------------------
// Ethernet SPI CS: PA3 - set in "w5100.h"
// Ethernet reset:  PA2
// SD-Card SPI CS:  PA4
// RS485 direction: PA8

#ifdef _DEBUG_
#define DBG_PIN	PB0
#define DEBUG_PIN_LOW	digitalWrite(DBG_PIN, LOW)
#define DEBUG_PIN_HIGH	digitalWrite(DBG_PIN, HIGH)
#endif

#define LED_PIN	PC13

#ifdef USE_RS485
	#define EC_ID	1
	#define VITO_ID	2
	#define RS484_DIR_PIN	PA8
	#define RS485_SET_DIR_TO_OUTPUT	pinMode(RS484_DIR_PIN, OUTPUT)
	#define RS485_ENABLE_TX			digitalWrite(RS484_DIR_PIN, HIGH)
	#define RS485_ENABLE_RX			digitalWrite(RS484_DIR_PIN, LOW)
#endif

#define CS_SD_CARD PA4  // PB4 on custom ATmega128

#define ETHERNET_SPI_CS_PIN	PA3
#define ETHERNET_RESET_PIN	PA2
#define ETHERNET_RESET_PIN_MODE_SET_TO_OUTPUT	pinMode(ETHERNET_RESET_PIN, OUTPUT)	// set PE6 to output
#define ETHERNET_RESET_PIN_SET_TO_LOW			digitalWrite(ETHERNET_RESET_PIN, LOW)	//asm("cbi 0x03, 6")
#define ETHERNET_RESET_PIN_SET_TO_HIGH			digitalWrite(ETHERNET_RESET_PIN, HIGH)	//asm("sbi 0x03, 6")

extern void showSocketStatus(void);

#endif
