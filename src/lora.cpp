#include "lora/lora.h"

/*
********************************************************************************
* Main lora functions
********************************************************************************
*/

SemaphoreHandle_t semaphore_isTransmitting;
bool isTransmitting = false;

void initLmic(bit_t adrEnabled)
{
    // init semaphore
    semaphore_isTransmitting = xSemaphoreCreateBinary();
    xSemaphoreGive(semaphore_isTransmitting);

    // Initialize LMIC runtime environment
    os_init();

    // Reset MAC state
    LMIC_reset();

    // Enable or disable ADR (data rate adaptation).
    // Should be turned off if the device is not stationary (mobile).
    // 1 is on, 0 is off.
    LMIC_setAdrMode(adrEnabled);

    #if defined(CFG_us915) || defined(CFG_au915)
        // NA-US and AU channels 0-71 are configured automatically
        // but only one group of 8 should (a subband) should be active
        // TTN recommends the second sub band, 1 in a zero based count.
        // https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json
        LMIC_selectSubBand(1);
    #endif

    // Register a custom eventhandler and don't use default onEvent() to enable
    // additional features (e.g. make EV_RXSTART available). User data pointer is omitted.
    LMIC_registerEventCb(&lmicEventCallback, nullptr);

    // start joining procedure
    LMIC_startJoining();
}

void lmicEventCallback(void *pUserData, ev_t ev)
{
    // LMIC event handler
    ostime_t timestamp = os_getTime();

    switch (ev)
    {
        case EV_RXSTART:
            // Do not print anything for this event or it will mess up timing.
            break;
        case EV_RXCOMPLETE:
            // -> processed in myRxCallback()
            break;

        case EV_TXSTART:
            xSemaphoreTake(semaphore_isTransmitting, portMAX_DELAY);
            isTransmitting = true;
            xSemaphoreGive(semaphore_isTransmitting);

            setTxIndicator();

            printEvent(timestamp, ev);
            break;
        case EV_TXCOMPLETE:
            // Transmit completed, includes waiting for RX windows.
            xSemaphoreTake(semaphore_isTransmitting, portMAX_DELAY);
            isTransmitting = false;
            xSemaphoreGive(semaphore_isTransmitting);
            
            setTxIndicator();

            printEvent(timestamp, ev);
            printFrameCounters();
            break;

        case EV_JOINING:
            setTxIndicator();

            printEvent(timestamp, ev);
            break;
        case EV_JOINED:
            xSemaphoreTake(semaphore_isTransmitting, portMAX_DELAY);
            isTransmitting = false;
            xSemaphoreGive(semaphore_isTransmitting);

            setTxIndicator();

            printEvent(timestamp, ev);
            printSessionKeys();
            break;
        case EV_JOIN_TXCOMPLETE:
        case EV_TXCANCELED:
            xSemaphoreTake(semaphore_isTransmitting, portMAX_DELAY);
            isTransmitting = false;
            xSemaphoreGive(semaphore_isTransmitting);

            setTxIndicator();

            printEvent(timestamp, ev);
            break;
          
        // Below events are printed only.
        case EV_SCAN_TIMEOUT:
        case EV_BEACON_FOUND:
        case EV_BEACON_MISSED:
        case EV_BEACON_TRACKED:
        case EV_RFU1:                    // This event is defined but not used in code
        case EV_JOIN_FAILED:
        case EV_REJOIN_FAILED:
        case EV_LOST_TSYNC:
        case EV_RESET:
        case EV_LINK_DEAD:
        case EV_LINK_ALIVE:
        case EV_SCAN_FOUND:              // This event is defined but not used in code 
            printEvent(timestamp, ev);
            break;

        default:
            printEvent(timestamp, "Unknown Event");
            break;
    }
}

lmic_tx_error_t scheduleUplink(uint8_t fPort, uint8_t* data, uint8_t dataLength, bool confirmed)
{
    // Transmission will be performed at the next possible time
    ostime_t timestamp = os_getTime();
    printEvent(timestamp, "Packet queued");

    lmic_tx_error_t retval = LMIC_setTxData2(fPort, data, dataLength, confirmed ? 1 : 0);

    if (retval != LMIC_ERROR_SUCCESS)
    {
        timestamp = os_getTime();
        String errmsg = "LMIC Error: ";
        errmsg.concat(lmicErrorNames[abs(retval)]);
        printEvent(timestamp, errmsg.c_str());
    }

    return retval;
}


/*
********************************************************************************
* LMIC overrides
********************************************************************************
*/

uint8_t deviceEUI[8] = { 0x00, 0x00, 0x00, 0x36, 0x21, 0x00, 0x00, 0x00 };
static const u1_t PROGMEM APPEUI[8]  = { OTAA_APPEUI };
static const u1_t PROGMEM APPKEY[16] = { OTAA_APPKEY };

void os_getDevEui (u1_t* buf) {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);

    // global EUI object
    deviceEUI[0] = mac[0];
    deviceEUI[1] = mac[1];
    deviceEUI[2] = mac[2];
    deviceEUI[5] = mac[3];
    deviceEUI[6] = mac[4];
    deviceEUI[7] = mac[5];

    if (buf == nullptr)
        return;

    // End-device Identifier (u1_t[8]) in lsb format
    u1_t eui[8] = { mac[5], mac[4], mac[3], 0x21, 0x36, mac[2], mac[1], mac[0] };
    memcpy_P(buf, eui, 8);
}
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16); }

/*
********************************************************************************
* Helpers
********************************************************************************
*/

float getSnr()
{
    // Returns ten times the SNR (dB) value of the last received packet.
    // Calculation per SX1276 datasheet rev.7 §6.4, SX1276 datasheet rev.4 §6.4.
    // LMIC.snr contains value of PacketSnr, which is 4 times the actual SNR value.
    return LMIC.snr / 4;
}

int16_t getRssi()
{
    // Returns correct RSSI (dBm) value of the last received packet.
    #define RSSI_OFFSET 64
    return LMIC.rssi - RSSI_OFFSET;
}

uint8_t getDataRate()
{
    // Returns current data rate for LMIC
    #ifdef CFG_eu868
        return 12 - LMIC.datarate;
    #elif
        return 0;
    #endif
}

bool isConnected()
{
    // Returns true if device is joined the network
    return LMIC.devaddr != 0;
}
