#pragma once

#ifndef _LORA_H_
#define _LORA_H_

#include "board.h"
#include "lora/message.h"
#include "lora/secrets.h"

#if !defined(OTAA_APPEUI) || !defined(OTAA_APPKEY)
    #error One or more LoRaWAN keys (OTAA_APPEUI, OTAA_APPKEY) are not defined.
#endif

// Determine if a valid region is defined.
// This actually has little effect because
// MCCI LMIC: if no region is defined it
// sets CFG_eu868 as default.
#if ( ( !( defined(CFG_as923) \
            || defined(CFG_as923jp) \
            || defined(CFG_au915) \
            || defined(CFG_eu868) \
            || defined(CFG_in866) \
            || defined(CFG_kr920) \
            || defined(CFG_us915) ) ) \
)
    #error No valid LoRaWAN region defined
#endif

/*
********************************************************************************
* Main lora functions
********************************************************************************
*/

extern SemaphoreHandle_t semaphore_isTransmitting;
extern bool isTransmitting;

void initLmic(bit_t adrEnabled = 1);
void lmicEventCallback(void *pUserData, ev_t ev);
lmic_tx_error_t scheduleUplink(uint8_t fPort, uint8_t* data, uint8_t dataLength, bool confirmed = false);


/*
********************************************************************************
* LMIC overrides
********************************************************************************
*/

extern uint8_t deviceEUI[8];
void os_getDevEui (u1_t* buf);
void os_getArtEui (u1_t* buf);
void os_getDevKey (u1_t* buf);


/*
********************************************************************************
* Helpers
********************************************************************************
*/

float getSnr();
int16_t getRssi();
uint8_t getDataRate();
bool isConnected();

#endif // _LORA_H_