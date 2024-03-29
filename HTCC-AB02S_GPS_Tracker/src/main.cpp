/* The example is for CubeCell_GPS,
 * GPS works only before lorawan uplink, the board current is about 45uA when in lowpower mode.
 */
#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "GPS_Air530.h"
#include "GPS_Air530Z.h"
#include "HT_SSD1306Wire.h"

Air530Class GPS;
extern SSD1306Wire display;

// when gps waked, if in GPS_UPDATE_TIMEOUT, gps not fixed then into low power mode
#define GPS_UPDATE_TIMEOUT 500000

// once fixed, GPS_CONTINUE_TIME later into low power mode
#define GPS_CONTINUE_TIME 10000 // 10000
/*
   set LoraWan_RGB to Active,the RGB active in loraWan
   RGB red means sending;
   RGB purple means joined done;
   RGB blue means RxWindow1;
   RGB yellow means RxWindow2;
   RGB green means received done;
*/

/* OTAA para*/
uint8_t devEui[] = {0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x05, 0xB5, 0x6C};
uint8_t appEui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t appKey[] = {0x35, 0x21, 0x8B, 0x8B, 0xD9, 0x03, 0x49, 0xBE, 0xBE, 0x09, 0xA5, 0xB9, 0x77, 0xC9, 0x72, 0x14};

/* ABP para*/
uint8_t nwkSKey[] = {0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda, 0x85};
uint8_t appSKey[] = {0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef, 0x67};
uint32_t devAddr = (uint32_t)0x007e6ae1;

/*LoraWan channelsmask, default channels 0-7*/
uint16_t userChannelsMask[6] = {0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t loraWanClass = LORAWAN_CLASS;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

/*OTAA or ABP*/
bool overTheAirActivation = LORAWAN_NETMODE;

/*ADR enable*/
bool loraWanAdr = LORAWAN_ADR;

/* set LORAWAN_Net_Reserve ON, the node could save the network info to flash, when node reset not need to join again */
bool keepNet = LORAWAN_NET_RESERVE;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = LORAWAN_UPLINKMODE;

/* Application port */
uint8_t appPort = 1;
/*!
  Number of trials to transmit the frame, if the LoRaMAC layer did not
  receive an acknowledgment. The MAC performs a datarate adaptation,
  according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
  to the following table:

  Transmission nb | Data Rate
  ----------------|-----------
  1 (first)       | DR
  2               | DR
  3               | max(DR-1,0)
  4               | max(DR-1,0)
  5               | max(DR-2,0)
  6               | max(DR-2,0)
  7               | max(DR-3,0)
  8               | max(DR-3,0)

  Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
  the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

int32_t fracPart(double val, int n)
{
    return (int32_t)((val - (int32_t)(val)) * pow(10, n));
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
void displayGPSInof()
{
    char str[30];
    display.clear();
    display.setFont(ArialMT_Plain_10);
    int index = sprintf(str, "%02d-%02d-%02d", GPS.date.year(), GPS.date.day(), GPS.date.month());
    str[index] = 0;
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, str);

    index = sprintf(str, "%02d:%02d:%02d", GPS.time.hour(), GPS.time.minute(), GPS.time.second(), GPS.time.centisecond());
    str[index] = 0;
    display.drawString(60, 0, str);

    if (GPS.location.age() < 1000)
    {
        display.drawString(120, 0, "A");
    }
    else
    {
        display.drawString(120, 0, "V");
    }

    index = sprintf(str, "alt: %d.%d", (int)GPS.altitude.meters(), fracPart(GPS.altitude.meters(), 2));
    str[index] = 0;
    display.drawString(0, 16, str);

    index = sprintf(str, "hdop: %d.%d", (int)GPS.hdop.hdop(), fracPart(GPS.hdop.hdop(), 2));
    str[index] = 0;
    display.drawString(0, 32, str);

    index = sprintf(str, "lat :  %d.%d", (int)GPS.location.lat(), fracPart(GPS.location.lat(), 4));
    str[index] = 0;
    display.drawString(60, 16, str);

    index = sprintf(str, "lon:%d.%d", (int)GPS.location.lng(), fracPart(GPS.location.lng(), 4));
    str[index] = 0;
    display.drawString(60, 32, str);

    index = sprintf(str, "speed: %d.%d km/h", (int)GPS.speed.kmph(), fracPart(GPS.speed.kmph(), 3));
    str[index] = 0;
    display.drawString(0, 48, str);
    display.display();
}

void printGPSInfo()
{
    Serial.print("Date/Time: ");
    if (GPS.date.isValid())
    {
        Serial.printf("%d/%02d/%02d", GPS.date.year(), GPS.date.day(), GPS.date.month());
    }
    else
    {
        Serial.print("INVALID");
    }

    if (GPS.time.isValid())
    {
        Serial.printf(" %02d:%02d:%02d.%02d", GPS.time.hour(), GPS.time.minute(), GPS.time.second(), GPS.time.centisecond());
    }
    else
    {
        Serial.print(" INVALID");
    }
    Serial.println();

    Serial.print("LAT: ");
    Serial.print(GPS.location.lat(), 6);
    Serial.print(", LON: ");
    Serial.print(GPS.location.lng(), 6);
    Serial.print(", ALT: ");
    Serial.print(GPS.altitude.meters());

    Serial.println();

    Serial.print("SATS: ");
    Serial.print(GPS.satellites.value());
    Serial.print(", HDOP: ");
    Serial.print(GPS.hdop.hdop());
    Serial.print(", AGE: ");
    Serial.print(GPS.location.age());
    Serial.print(", COURSE: ");
    Serial.print(GPS.course.deg());
    Serial.print(", SPEED: ");
    Serial.println(GPS.speed.kmph());
    Serial.println();
}

static void prepareTxFrame(uint8_t port)
{
    /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
      appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
      if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
      if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
      for example, if use REGION_CN470,
      the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
    */

    float lat, lon, alt, course, speed, hdop, sats;

    Serial.println("Waiting for GPS FIX ...");

    VextON(); // oled power on;
    delay(10);
    display.init();
    display.clear();

    uint32_t start = millis();

    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 32 - 16 / 2, "GPS Searching...");
    display.drawString(64, 32 + 8, "remaining: " + String((GPS_UPDATE_TIMEOUT - (millis() - start)) / 1000));
    Serial.println("GPS Searching...");
    display.display();

    GPS.begin();

    while ((millis() - start) < GPS_UPDATE_TIMEOUT)
    {
        while (GPS.available() > 0)
        {
            GPS.encode(GPS.read());

            // Show message GPS searching and timeout
            display.clear();
            display.drawString(64, 32 - 16, "GPS Searching...");
            display.drawString(64, 32 + 16, "Timeout in: " + String((GPS_UPDATE_TIMEOUT - (millis() - start)) / 1000));
            display.display();

            // Print gps info to serial output every 10 seconds
            if ((millis() - start) % 10000 == 0)
            {
                Serial.print("GPS INFO:");
                printGPSInfo();
            }
        }
        // gps fixed in a second
        if (GPS.location.age() < 1000)
        {
            break;
        }
    }

    // if gps fixed,  GPS_CONTINUE_TIME later stop GPS into low power mode, and every 1 second update gps, print and display gps info
    if (GPS.location.age() < 1000)
    {
        start = millis();
        uint32_t printinfo = 0;
        while ((millis() - start) < GPS_CONTINUE_TIME)
        {
            while (GPS.available() > 0)
            {
                GPS.encode(GPS.read());
            }

            if ((millis() - start) > printinfo)
            {
                printinfo += 1000;
                printGPSInfo();
                displayGPSInof();
            }
        }
    }
    else
    {
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_16);
        display.drawString(64, 32 - 16 / 2, "No GPS signal");
        Serial.println("No GPS signal");
        display.display();
        delay(2000);
    }
    // Air530.end();
    display.clear();
    display.display();
    display.stop();
    VextOFF(); // oled power off

    lat = GPS.location.lat();
    lon = GPS.location.lng();
    alt = GPS.altitude.meters();
    course = GPS.course.deg();
    speed = GPS.speed.kmph();
    sats = GPS.satellites.value();
    hdop = GPS.hdop.hdop();

    uint16_t batteryVoltage = getBatteryVoltage();

    unsigned char *puc;

    appDataSize = 0;
    puc = (unsigned char *)(&lat);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];
    appData[appDataSize++] = puc[2];
    appData[appDataSize++] = puc[3];
    puc = (unsigned char *)(&lon);

    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];
    appData[appDataSize++] = puc[2];
    appData[appDataSize++] = puc[3];
    puc = (unsigned char *)(&alt);

    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];
    appData[appDataSize++] = puc[2];
    appData[appDataSize++] = puc[3];
    puc = (unsigned char *)(&course);

    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];
    appData[appDataSize++] = puc[2];
    appData[appDataSize++] = puc[3];
    puc = (unsigned char *)(&speed);

    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];
    appData[appDataSize++] = puc[2];
    appData[appDataSize++] = puc[3];

    puc = (unsigned char *)(&hdop);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];
    appData[appDataSize++] = puc[2];
    appData[appDataSize++] = puc[3];

    // Added this to also send amount of sats
    puc = (unsigned char *)(&sats);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];
    appData[appDataSize++] = puc[2];
    appData[appDataSize++] = puc[3];

    appData[appDataSize++] = (uint8_t)(batteryVoltage >> 8);
    appData[appDataSize++] = (uint8_t)batteryVoltage;

    Serial.print("SATS: ");
    Serial.print(GPS.satellites.value());
    Serial.print(", HDOP: ");
    Serial.print(GPS.hdop.hdop());
    Serial.print(", LAT: ");
    Serial.print(GPS.location.lat());
    Serial.print(", LON: ");
    Serial.print(GPS.location.lng());
    Serial.print(", AGE: ");
    Serial.print(GPS.location.age());
    Serial.print(", ALT: ");
    Serial.print(GPS.altitude.meters());
    Serial.print(", COURSE: ");
    Serial.print(GPS.course.deg());
    Serial.print(", SPEED: ");
    Serial.println(GPS.speed.kmph());
    Serial.print(" BatteryVoltage:");
    Serial.println(batteryVoltage);
}

void setup()
{
    boardInitMcu();
    Serial.begin(115200);

    // GPS Mode

    /*three modes supported:
     * https://github.com/sivaelid/Heltec_AB02S_Mods/blob/master/LoRaWan_OnBoardGPS_Air530_Vaelid.ino
     * GPS        :    MODE_GPS - this works
     * GPS+BEIDOU :    MODE_GPS_BEIDOU this works
     * GPS+GLONASS:    MODE_GPS_GLONASS this works
     * GPS+GALILEO:    MODE_GPS_GALILEO this does not work
     * GPS+BEIDOU+GLONASS+GALILEO:   MODE_GPS_MULTI   this does not work
     * default mode is GPS+BEIDOU.
     */

    GPS.setmode(MODE_GPS_GLONASS); // GPS+GLONASS

#if (AT_SUPPORT)
    enableAt();
#endif
    LoRaWAN.displayMcuInit();
    deviceState = DEVICE_STATE_INIT;
    LoRaWAN.ifskipjoin();

    GPS.begin();
}

void loop()
{
    switch (deviceState)
    {
    case DEVICE_STATE_INIT:
    {
#if (AT_SUPPORT)
        getDevParam();
#endif
        printDevParam();
        LoRaWAN.init(loraWanClass, loraWanRegion);
        deviceState = DEVICE_STATE_JOIN;
        break;
    }
    case DEVICE_STATE_JOIN:
    {
        LoRaWAN.displayJoining();
        LoRaWAN.join();
        break;
    }
    case DEVICE_STATE_SEND:
    {
        prepareTxFrame(appPort);
        LoRaWAN.displaySending();
        LoRaWAN.send();
        deviceState = DEVICE_STATE_CYCLE;
        break;
    }
    case DEVICE_STATE_CYCLE:
    {
        // Schedule next packet transmission
        txDutyCycleTime = appTxDutyCycle; //  + randr( 0, APP_TX_DUTYCYCLE_RND );
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        break;
    }
    case DEVICE_STATE_SLEEP:
    {
        LoRaWAN.displayAck();
        LoRaWAN.sleep();
        break;
    }
    default:
    {
        deviceState = DEVICE_STATE_INIT;
        break;
    }
    }
}
