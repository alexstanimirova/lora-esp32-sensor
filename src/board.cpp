#include "main.h"

// Pin mappings for LoRa transceiver
const lmic_pinmap lmic_pins = {
    .nss = SX1276_CS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = SX1276_RST,
    .dio = { /*dio0*/ SX1276_DI0, /*dio1*/ SX1276_DI1, /*dio2*/ SX1276_DI2 },
    .rxtx_rx_active = 0,
    .rssi_cal = 10,
    .spi_freq = SX1276_SPI_FREQ /* 8 MHz */
};


// Battery
float getBatteryVolts()
{
    int tmp = 0;
    int totalValue = 0;
    int averageValue = 0;

    for (int i = 0; i < 20; i++)
    {
        tmp = analogRead(35);
        delay(1);
        tmp = analogRead(35);
        tmp = analogRead(35);
        totalValue += tmp;
    }

    averageValue = totalValue / 20;

    return averageValue * 1.8 / 1000;
}


// Serial
HardwareSerial& serial = Serial;

const char * const lmicEventNames[] = { LMIC_EVENT_NAME_TABLE__INIT };
const char * const lmicErrorNames[] = { LMIC_ERROR_NAME__INIT };

bool initSerial(unsigned long speed)
{
    serial.begin(speed);
    return serial;
}

void printChars(Print& printer, char ch, uint8_t count, bool linefeed)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        printer.print(ch);
    }
    if (linefeed)
    {
        printer.println();
    }
}

void printSpaces(Print& printer, uint8_t count, bool linefeed)
{
    printChars(printer, ' ', count, linefeed);
}

void printHex(Print& printer, uint8_t* bytes, size_t length, bool linefeed, char separator)
{
    for (size_t i = 0; i < length; ++i)
    {
        if (i > 0 && separator != 0)
        {
            printer.print(separator);
        }
        if (bytes[i] <= 0x0F)
        {
            printer.print('0');
        }
        printer.print(bytes[i], HEX);
    }
    if (linefeed)
    {
        printer.println();
    }
}

void printEvent(ostime_t timestamp, const char * const message, bool eventLabel)
{
    // Create padded/indented output without using printf().
    // printf() is not default supported/enabled in each Arduino core.
    // Not using printf() will save memory for memory constrainted devices.

    String timeString(timestamp);
    uint8_t len = timeString.length();
    uint8_t zerosCount = 12 > len ? 12 - len : 0;

    printChars(serial, '0', zerosCount);
    serial.print(timeString);
    serial.print(":  ");
    if (eventLabel)
    {
        serial.print(F("Event: "));
    }
    serial.println(message);
}

void printEvent(ostime_t timestamp, ev_t ev)
{
    printEvent(timestamp, lmicEventNames[ev], true);
}

void printFrameCounters()
{
    printSpaces(serial, 15);
    serial.print(F("Up: "));
    serial.print(LMIC.seqnoUp);
    serial.print(F(",  Down: "));
    serial.println(LMIC.seqnoDn);
}

void printSessionKeys()
{
    u4_t networkId = 0;
    devaddr_t deviceAddress = 0;
    u1_t networkSessionKey[16];
    u1_t applicationSessionKey[16];
    LMIC_getSessionKeys(&networkId, &deviceAddress, networkSessionKey, applicationSessionKey);

    printSpaces(serial, 15);
    serial.print(F("Network Id: "));
    serial.println(networkId, DEC);

    printSpaces(serial, 15);
    serial.print(F("Device Address: "));
    serial.println(deviceAddress, HEX);

    printSpaces(serial, 15);
    serial.print(F("Application Session Key: "));
    printHex(serial, applicationSessionKey, 16, true, '-');

    printSpaces(serial, 15);
    serial.print(F("Network Session Key:     "));
    printHex(serial, networkSessionKey, 16, true, '-');
}


// EasyLed
EasyLed led(LED_BUILTIN, EasyLed::ActiveLevel::High);


// Display
// Create U8x8 instance for SSD1306 OLED display (no reset) using hardware I2C.
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, /*rst*/ U8X8_PIN_NONE, /*scl*/ SCL, /*sda*/ SDA);

bool updateDisplay = false;
SemaphoreHandle_t semaphore_updateDisplay;

uint8_t txSymbol[8] = { 0x18, 0x18, 0x00, 0x24, 0x99, 0x42, 0x3c, 0x00 };
uint8_t noTxSymbol[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// value from 0 to 7, higher values more brighter
void setSSD1306VcomDeselect(uint8_t v)
{	
    u8x8_cad_StartTransfer(display.getU8x8());
    u8x8_cad_SendCmd(display.getU8x8(), 0x0db);
    u8x8_cad_SendArg(display.getU8x8(), v << 4);
    u8x8_cad_EndTransfer(display.getU8x8());
}

// p1: 1..15, higher values, more darker, however almost no difference with 3 or more
// p2: 1..15, higher values, more brighter
void setSSD1306PreChargePeriod(uint8_t p1, uint8_t p2)
{
    u8x8_cad_StartTransfer(display.getU8x8());
    u8x8_cad_SendCmd(display.getU8x8(), 0x0d9);
    u8x8_cad_SendArg(display.getU8x8(), (p2 << 4) | p1 );
    u8x8_cad_EndTransfer(display.getU8x8());
}

void initDisplay()
{
    // init semaphore
    semaphore_updateDisplay = xSemaphoreCreateBinary();
    xSemaphoreGive(semaphore_updateDisplay);

    display.begin();

    // set brightness to the lowest setting to prevent oled burn-in
    setSSD1306VcomDeselect(0);
    setSSD1306PreChargePeriod(0, 0);
}

void displayEUI()
{
    display.setFont(DISPLAY_FONT);
    display.setCursor(COL_0, EUI_ROW);
    display.printf("%02X%02X%02X%02X%02X%02X%02X%02X", deviceEUI[0], deviceEUI[1], deviceEUI[2], deviceEUI[3], deviceEUI[4], deviceEUI[5], deviceEUI[6], deviceEUI[7]);
}

void displaySensors()
{
    display.setFont(DISPLAY_FONT);

    // Temperature
    display.setCursor(COL_0, SENSOR_1_ROW);
    display.printf("T>%6.1f", temperature);

    // Humidity
    display.setCursor(COL_0, SENSOR_2_ROW);
    display.printf("H>%6.1f", humidity);
}

void displayDeviceInfo()
{
    display.setFont(DISPLAY_FONT);

    // fcnt
    display.setCursor(COL_8, SENSOR_1_ROW);
    display.printf("%8d", LMIC.seqnoUp);

    // spreading factor
    uint8_t dataRate = getDataRate();
    display.setCursor(COL_8, SENSOR_2_ROW);
    display.printf("    SF%2d", dataRate);

    // RSSI
    int16_t rssi = getRssi();
    if (rssi != -64) {
        display.setCursor(COL_8, SENSOR_3_ROW);
        display.printf("%8d", rssi);
    }

    // SNR
    float snr = getSnr();
    if (rssi != -64) {
        display.setCursor(COL_8, SENSOR_4_ROW);
        display.printf("%8.1f", snr);
    }
}

void displayTxSymbol()
{
    if (isTransmitting)
    {
        display.drawTile(TXSYMBOL_COL, ROW_0, 1, txSymbol);
    }
    else
    {
        display.drawTile(TXSYMBOL_COL, ROW_0, 1, noTxSymbol);
    }
}

void displayMainScreen()
{
    display.firstPage();
    do {
        if (!isConnected()) {
            display.setFont(DISPLAY_FONT);
            display.setCursor(COL_0, HEADER_ROW);
            display.print(F("Connecting..."));
        } else {
            // display battery voltage
            display.setFont(DISPLAY_FONT);
            display.setCursor(COL_0, HEADER_ROW);
            display.printf("B>%5.2fv", getBatteryVolts());
        }

        displayTxSymbol();

        // print filtered sensor values
        displaySensors();
        displayDeviceInfo();
        displayEUI();
    } while ( display.nextPage() );
}

void setTxIndicator()
{
    if (isTransmitting)
    {
        led.on();
    }
    else
    {
        led.off();
    }

    xSemaphoreTake(semaphore_updateDisplay, portMAX_DELAY);
    updateDisplay = true;
    xSemaphoreGive(semaphore_updateDisplay);
}


// Sensor
Adafruit_AHTX0 aht;
float temperature = -1;
float humidity = -1;

void readSensor()
{
    sensors_event_t ev_humidity, ev_temp;
    aht.getEvent(&ev_humidity, &ev_temp);

    temperature = ev_temp.temperature;
    humidity = ev_humidity.relative_humidity;
}