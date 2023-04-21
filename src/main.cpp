/*
********************************************************************************
* Main include
********************************************************************************
*/

#include "main.h"


/*
********************************************************************************
* Task Handles
********************************************************************************
*/

TaskHandle_t oledTask = NULL;
TaskHandle_t sensorTask = NULL;
TaskHandle_t lmicTask = NULL;


/*
********************************************************************************
* Tasks
********************************************************************************
*/

void oledtask(void *pvParameters) {
    for (;;) {

        // --- send data to oled ---
        if (updateDisplay) {
            displayMainScreen();

            xSemaphoreTake(semaphore_updateDisplay, portMAX_DELAY);
            updateDisplay = false;
            xSemaphoreGive(semaphore_updateDisplay);
        }

        // yield to CPU
        delay(2);
    }
}

// Uplink interval
const unsigned long uplinkInterval = UPLINK_INTERVAL_S * 1000;
unsigned long uplinkPreviousMillis = 0;

// data array
uint8_t sendData[sizeof(sUplinkMessage)];

void sensortask(void *pvParameters) {
  for (;;) {

    // uplink
    if (millis() - uplinkPreviousMillis > uplinkInterval) {

        uplinkPreviousMillis = millis();

        // read sensor
        readSensor();

        // don't schedule uplink if lmic is busy
        if (LMIC.devaddr == 0 || !LMIC_queryTxReady())
        {
            // TxRx is currently pending, do not send.
            printEvent(os_getTime(), "Uplink not scheduled...");

            // retry in 1s
            delay(1000);
            continue;
        }

        // construct message
        int32_t val;
        sUplinkMessage msg;

        msg.temperature = temperature;
        msg.humidity = humidity;

        // Prepare uplink payload.
        memcpy(sendData, &msg, sizeof(sUplinkMessage));

        uint8_t fPort = 10;
        scheduleUplink(fPort, sendData, sizeof(sUplinkMessage), true);

    }

    // yield to CPU
    delay(2);
  }
}

void lmictask(void *pvParameters) {
  for (;;) {

    // --- proccess RF ---
    os_runloop_once();

    // yield to CPU
    delay(2);
    
  }
}


/*
********************************************************************************
* Setup
********************************************************************************
*/

void setup()
{

    initDisplay();

    initSerial(MONITOR_SPEED);

    // generate EUI
    os_getDevEui(nullptr);

    // show device info
    serial.println(F("------------------------"));
    serial.print(F("EUI: "));
    serial.printf("%02X%02X%02X%02X%02X%02X%02X%02X\n", deviceEUI[0], deviceEUI[1], deviceEUI[2], deviceEUI[3], deviceEUI[4], deviceEUI[5], deviceEUI[6], deviceEUI[7]);
    serial.println(F("------------------------"));

    // init sensor
    aht.begin();

    // read sensor
    readSensor();

    // init lmic
    initLmic();

    // start oled task
    xTaskCreatePinnedToCore(oledtask,       // task function
                            "oledtask",     // name of task
                            4096,           // stack size of task
                            (void *)1,      // parameter of the task
                            1,              // priority of the task
                            &oledTask,      // task handle
                            0);             // CPU core

    // start sensor task
    xTaskCreatePinnedToCore(sensortask,     // task function
                            "sensortask",   // name of task
                            4096,           // stack size of task
                            (void *)1,      // parameter of the task
                            2,              // priority of the task
                            &sensorTask,    // task handle
                            0);             // CPU core

    // start lmic task
    xTaskCreatePinnedToCore(lmictask,       // task function
                            "lmictask",     // name of task
                            4096,           // stack size of task
                            (void *)1,      // parameter of the task
                            2,              // priority of the task
                            &lmicTask,      // task handle
                            1);             // CPU core

    while (LMIC.devaddr == 0 || LMIC.opmode & OP_TXRXPEND) {
        // wait for join
        delay(10);
    }
}


/*
********************************************************************************
* Loop (unused)
********************************************************************************
*/

void loop() { vTaskDelete(NULL); }