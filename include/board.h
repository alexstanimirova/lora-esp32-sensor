#pragma once

#ifndef _BOARD_H_
#define _BOARD_H_

#include <Arduino.h>
#include "lmic.h"
#include "hal/hal.h"
#include <Adafruit_AHTX0.h>


// Pin mappings for LoRa transceiver
#define SX1276_CS       18                  // SX1276's CS
#define SX1276_RST      23                  // SX1276's RESET
#define SX1276_DI0      26                  // SX1276's IRQ (Interrupt Request)
#define SX1276_DI1      33                  // SX1276's DIO1
#define SX1276_DI2      32                  // SX1276's DIO2
#define SX1276_SPI_FREQ 8000000             // SX1276's DIO2

extern const lmic_pinmap lmic_pins;


// Battery definitions
float getBatteryVolts();


// Serial definitions
extern HardwareSerial& serial;

extern const char * const lmicEventNames[];
extern const char * const lmicErrorNames[];

bool initSerial(unsigned long speed = 115200);
void printChars(Print& printer, char ch, uint8_t count, bool linefeed = false);
void printSpaces(Print& printer, uint8_t count, bool linefeed = false);
void printHex(Print& printer, uint8_t* bytes, size_t length = 1, bool linefeed = false, char separator = 0);
void printEvent(ostime_t timestamp, const char * const message, bool eventLabel = false);
void printEvent(ostime_t timestamp, ev_t ev);
void printFrameCounters();
void printSessionKeys();


// EasyLed definitions
#include "EasyLed.h"
extern EasyLed led;


// Display definitions
#include <Wire.h>
#include "U8g2lib.h"

#define DISPLAY_FONT        u8g2_font_victoriamedium8_8r
#define ROW_0               8
#define ROW_2               21
#define ROW_3               31
#define ROW_4               41
#define ROW_5               51
#define ROW_7               64
#define HEADER_ROW          ROW_0
#define SENSOR_1_ROW        ROW_2
#define SENSOR_2_ROW        ROW_3
#define SENSOR_3_ROW        ROW_4
#define SENSOR_4_ROW        ROW_5
#define EUI_ROW             ROW_7
#define COL_0               0
#define COL_8               64
#define TXSYMBOL_COL        15

extern U8G2_SSD1306_128X64_NONAME_1_HW_I2C display;

extern SemaphoreHandle_t semaphore_updateDisplay;
extern bool updateDisplay;

extern uint8_t txSymbol[8];
extern uint8_t noTxSymbol[8];

void setSSD1306VcomDeselect(uint8_t v);
void setSSD1306PreChargePeriod(uint8_t p1, uint8_t p2);
void initDisplay();
void displayEUI();
void displaySensors();
void displayDeviceInfo();
void displayTxSymbol();
void displayMainScreen();
void setTxIndicator();


// Sensor definitions
extern Adafruit_AHTX0 aht;
extern float temperature;
extern float humidity;

void readSensor();

#endif // _BOARD_H_