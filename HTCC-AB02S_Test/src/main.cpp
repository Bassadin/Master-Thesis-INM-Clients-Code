/**
 * This is an example of joining, sending and receiving data via LoRaWAN using a more minimal interface.
 *
 * The example is configured for OTAA, set your keys into the variables below.
 *
 * The example will upload a counter value periodically, and will print any downlink messages.
 *
 * please disable AT_SUPPORT in tools menu
 *
 * David Brodrick.
 */
#include "LoRaWanMinimal_APP.h"
#include "Arduino.h"

#include "HT_SSD1306Wire.h"

SSD1306Wire display(0x3c, 500000, SDA, SCL, GEOMETRY_128_64, GPIO10);

/*
 * set LoraWan_RGB to Active,the RGB active in loraWan
 * RGB red means sending;
 * RGB purple means joined done;
 * RGB blue means RxWindow1;
 * RGB yellow means RxWindow2;
 * RGB green means received done;
 */

// Set these OTAA parameters to match your app/node in TTN
static uint8_t devEui[] = {0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x05, 0xB2, 0xBF};
static uint8_t appEui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t appKey[] = {0x6D, 0x2C, 0x4B, 0x70, 0x61, 0xC7, 0xED, 0xD1, 0x6A, 0xB0, 0xA9, 0x49, 0xCE, 0x98, 0x7A, 0x27};

uint16_t userChannelsMask[6] = {0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};

static uint8_t counter = 0;

///////////////////////////////////////////////////
// Some utilities for going into low power mode
TimerEvent_t sleepTimer;
// Records whether our sleep/low power timer expired
bool sleepTimerExpired;

static void wakeUp()
{
    sleepTimerExpired = true;
}

static void serialAndDisplayPrint(const char *str)
{
    Serial.println(str);

    display.clear();
    display.println(str);
    display.drawLogBuffer(0, 0);
    display.display();
}

static void lowPowerSleep(uint32_t sleeptime)
{
    sleepTimerExpired = false;
    TimerInit(&sleepTimer, &wakeUp);
    TimerSetValue(&sleepTimer, sleeptime);
    TimerStart(&sleepTimer);
    // Low power handler also gets interrupted by other timers
    // So wait until our timer had expired
    while (!sleepTimerExpired)
        lowPowerHandler();
    TimerStop(&sleepTimer);
}

void VextON(void)
{
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
}

void VextOFF(void) // Vext default OFF
{
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, HIGH);
}

///////////////////////////////////////////////////
void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println();

    VextON();
    delay(100);

    display.init();
    display.setFont(ArialMT_Plain_10);

    display.setLogBuffer(5, 30);

    LoRaWAN.begin(LORAWAN_CLASS, ACTIVE_REGION);

    // Enable ADR
    LoRaWAN.setAdaptiveDR(true);

    while (1)
    {
        serialAndDisplayPrint("Joining... ");
        LoRaWAN.joinOTAA(appEui, appKey, devEui);
        if (!LoRaWAN.isJoined())
        {
            // In this example we just loop until we're joined, but you could
            // also go and start doing other things and try again later
            serialAndDisplayPrint("JOIN FAILED! Sleeping for 30 seconds");
            lowPowerSleep(30000);
        }
        else
        {
            serialAndDisplayPrint("JOINED");
            break;
        }
    }
}

///////////////////////////////////////////////////
void loop()
{
    // Counter is just some dummy data we send for the example
    counter++;

    // In this demo we use a timer to go into low power mode to kill some time.
    // You might be collecting data or doing something more interesting instead.
    lowPowerSleep(15000);

    // Now send the data. The parameters are "data size, data pointer, port, request ack"

    char logString[50];
    memset(logString, 0, sizeof logString);

    // Convert the counter to a string
    char counterString[16];
    itoa(counter, counterString, 10);

    // Build a string to print out
    strcat(logString, "SEND with #=");
    strcat(logString, counterString);

    serialAndDisplayPrint(logString);

    // Here we send confirmed packed (ACK requested) only for the first five (remember there is a fair use policy)
    bool requestack = counter < 5 ? true : false;
    if (LoRaWAN.send(1, &counter, 1, requestack))
    {
        serialAndDisplayPrint("Send OK");
    }
    else
    {
        serialAndDisplayPrint("Send FAILED");
    }
}

///////////////////////////////////////////////////
// Example of handling downlink data
void downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
    Serial.printf("Received downlink: %s, RXSIZE %d, PORT %d, DATA: ", mcpsIndication->RxSlot ? "RXWIN2" : "RXWIN1", mcpsIndication->BufferSize, mcpsIndication->Port);
    for (uint8_t i = 0; i < mcpsIndication->BufferSize; i++)
    {
        Serial.printf("%02X", mcpsIndication->Buffer[i]);
    }
    Serial.println();
}