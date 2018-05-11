
/*NTP ESP-8266 Clock By Sebouh
 * Tailored for NodeMCU and TM1637 by Jeffrey Lai
For Compiling USE Arduino IDE 1.6.5
*/
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <DNSServer.h>            //https://github.com/esp8266/Arduino/tree/master/libraries/DNSServer
#include <ESP8266WebServer.h>     //https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Time.h>                 //https://github.com/PaulStoffregen/Time
#include <Timezone.h>             //https://github.com/JChristensen/Timezone
#include "SevenSegmentTM1637.h"   //https://github.com/bremme/arduino-tm1637
#include "SevenSegmentExtended.h" //https://github.com/bremme/arduino-tm1637

//NodeMCU Module connection pins (Digital Pins)
const byte PIN_CLK = D6;   // define CLK pin (any digital pin)
const byte PIN_DIO = D5;   // define DIO pin (any digital pin)
SevenSegmentExtended    display(PIN_CLK, PIN_DIO);
int resetBtnPin = 3;      //Reset button to reset wifi

//Configuration
#define HOSTNAME "NTP Clock"
//#define AP_PASSWORD "Password123" //Uncomment to enable wifi password to access the Access Point, when WIFI credentials are unset.

//Edit These Lines According To Your Timezone and Daylight Saving Time
//TimeZone Settings Details https://github.com/JChristensen/Timezone

//Uncomment to use predefined timezone

//#define ausET // Australia Eastern Time Zone (Sydney, Melbourne)
#define CE    // Central European Time (Frankfurt, Brussels, Copenhagen, Madrid, Paris)
//#define UK    // United Kingdom (London, Belfast)
//#define UTC   // UTC
//#define usET  // US Eastern Time Zone (New York, Detroit)
//#define usCT  // US Central Time Zone (Chicago, Houston)
//#define usMT  // US Mountain Time Zone (Denver, Salt Lake City)
//#define usAZ  // Arizona is US Mountain Time Zone but does not use DST
//#define usPT  // // US Pacific Time Zone (Las Vegas, Los Angeles)

//Predefined timezones
#ifdef ausET
// Australia Eastern Time Zone (Sydney, Melbourne)
TimeChangeRule aEDT = {"AEDT", First, Sun, Oct, 2, 660};    // UTC + 11 hours
TimeChangeRule aEST = {"AEST", First, Sun, Apr, 3, 600};    // UTC + 10 hours
Timezone myLocalTime(aEDT, aEST);
#endif

#ifdef CE
// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone myLocalTime(CEST, CET);
#endif

#ifdef UK
// United Kingdom (London, Belfast)
TimeChangeRule BST = {"BST", Last, Sun, Mar, 1, 60};        // British Summer Time
TimeChangeRule GMT = {"GMT", Last, Sun, Oct, 2, 0};         // Standard Time
Timezone myLocalTime(BST, GMT);
#endif

#ifdef UTC
// UTC
TimeChangeRule utcRule = {"UTC", Last, Sun, Mar, 1, 0};     // UTC
Timezone myLocalTime(utcRule, utcRule);
#endif

#ifdef usET
// US Eastern Time Zone (New York, Detroit)
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  // Eastern Daylight Time = UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   // Eastern Standard Time = UTC - 5 hours
Timezone myLocalTime(usEDT, usEST);
#endif

#ifdef usCT
// US Central Time Zone (Chicago, Houston)
TimeChangeRule usCDT = {"CDT", Second, dowSunday, Mar, 2, -300};
TimeChangeRule usCST = {"CST", First, dowSunday, Nov, 2, -360};
Timezone myLocalTime(usCDT, usCST);
#endif

#ifdef usMT
// US Mountain Time Zone (Denver, Salt Lake City)
TimeChangeRule usMDT = {"MDT", Second, dowSunday, Mar, 2, -360};
TimeChangeRule usMST = {"MST", First, dowSunday, Nov, 2, -420};
Timezone myLocalTime(usMDT, usMST);
#endif

#ifdef usAZ
// Arizona is US Mountain Time Zone but does not use DST
Timezone myLocalTime(usMST, usMST);
#endif

#ifdef usPT
// US Pacific Time Zone (Las Vegas, Los Angeles)
TimeChangeRule usPDT = {"PDT", Second, dowSunday, Mar, 2, -420};
TimeChangeRule usPST = {"PST", First, dowSunday, Nov, 2, -480};
Timezone myLocalTime(usPDT, usPST);
#endif


//Pointer To The Time Change Rule, Use to Get The TZ Abbrev
TimeChangeRule *tcr;
time_t utc, local;

//NTP Server http://tf.nist.gov/tf-cgi/servers.cgi
static const char ntpServerName[] = "time.nist.gov";
WiFiUDP Udp;
// Local Port to Listen For UDP Packets
uint16_t localPort;

// Setup Details
void setup() {
  Serial.begin(115200);
  // Initialize LCD
  display.begin();            // initializes the display
  display.clear(); 
  display.setBacklight(100);  // set the brightness to 100 %
  display.print("INIT");      // display INIT on the display

  delay(500);
  
  // Reset Button Switch Type
  pinMode(resetBtnPin, INPUT_PULLUP);
  
  //WiFiManager
  //Local intialization.
  WiFiManager wifiManager;
  //AP Configuration
  wifiManager.setAPCallback(configModeCallback);
  //Exit After Config Instead of connecting
  wifiManager.setBreakAfterConfig(true);

  //Reset Settings - If Button Pressed/hold during power on
  if (digitalRead(resetBtnPin) == LOW) {
    //Display <Reset>
    display.clear();
    display.print("RESET");      // display RESET on the display
    delay(5000);
    wifiManager.resetSettings();
    //ESP.reset();
    ESP.restart();
  }
  /*
  Tries to connect to last known settings
  if it does not connect it starts an access point with the specified name
  here  "AutoConnectAP" without password (uncomment below line and comment
  next line if password required)
  and goes into a blocking loop awaiting configuration
  */

  #ifdef AP_PASSWORD
  if (!wifiManager.autoConnect(HOSTNAME, AP_PASSWORD)){   //If You Require Password For Your NTP Clock
  #else
  if (!wifiManager.autoConnect(HOSTNAME)) {               //If You Dont Require Password For Your NTP Clock
  #endif
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  //Display <Connect> Once Connected to AP
  display.clear(); 
  display.print("CONNECT");      // display CONNECT on the display
  delay(3000);
  {
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }

    // Seed Random With vVlues Unique To This Device
    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    uint32_t seed1 =
      (macAddr[5] << 24) | (macAddr[4] << 16) |
      (macAddr[3] << 8)  | macAddr[2];
    randomSeed(WiFi.localIP() + seed1 + micros());
    localPort = random(1024, 65535);
    Udp.begin(localPort);
    setSyncProvider(getNtpTime);
    //Set Sync Intervals
    setSyncInterval(5 * 60);
  }
}
void loop() {
  // when the digital clock was displayed
  static time_t prevDisplay = 0;
  timeStatus_t ts = timeStatus();
  utc = now();
  local = myLocalTime.toLocal(utc, &tcr);
  
  switch (ts) {
    case timeNeedsSync:
    case timeSet:
      //update the display only if time has changed
      if (now() != prevDisplay) {
        prevDisplay = now();
        digitalClockDisplay();
        tmElements_t tm;
        breakTime(now(), tm);
        //If Time Needs Sync turn off the colon (:) on the display
        if (ts == timeNeedsSync) {
          display.setColonOn(false);
        }
        //else {
        //lc.setDigit(0, 2, (second() % 10), false);
        //  }
      }
      break;
    case timeNotSet:
      //Display <No Sync> If Time Not Displayed
      display.clear();
      display.print("NO SYNC");      // display NO SYNC on the display
      now();
      delay(3000);
      ESP.restart();
  }
}
//#####################################################
void digitalClockDisplay() {
  tmElements_t tm;
  char *dayOfWeek;
  breakTime(now(), tm);
  display.clear();
  // digital clock display of the time
  serialPrintTime();
  display.printTime(hour(local),minute(local), true);
}

void serialPrintTime() {
  //serial printing the time
  Serial.print(hour(local));
  printDigits(minute(local));
  printDigits(second(local));
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year());
  Serial.println();
}

void printDigits(int digits) {
 // utility function for digital clock display: prints preceding colon and leading 0
 Serial.print(":");
 if (digits < 10)
 Serial.print('0');
 Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress timeServerIP; // time.nist.gov NTP server address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.print(F("Transmit NTP Request "));
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);
  Serial.println(timeServerIP);

  sendNTPpacket(timeServerIP);
  uint32_t beginWait = millis();
  while ((millis() - beginWait) < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL;
    }
  }
  return 0; // return 0 if unable to get the time
}
// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{

  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
//To Display <Setup> if not connected to AP
void configModeCallback (WiFiManager *myWiFiManager) {
  display.clear();
  display.print("SETUP");      // display SETUP on the display
  delay(2000);
}

