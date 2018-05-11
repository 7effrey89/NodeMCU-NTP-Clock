# NodeMCU NTP Clock

##### NodeMCU development board

<img src="https://cdn.shopify.com/s/files/1/0672/9409/products/NodeMCU_ESP8266_development_board_1024x1024.jpg" width="250">

##### TM1637 
![TM1637](extras/img/TM1637-4-digit-colon.jpg)

# Hardware setup

| TM1637 PIN | NodeMCU PIN      | Description         |
|------------|------------------|---------------------|
| CLK        |  D6              | Clock               |
| DIO        |  D5              | Digital output      |
| VCC        |  5V              | Supply voltage      |
| GND        |  GND             | Ground              |

| Button     | NodeMCU PIN      | Description         |
|------------|------------------|---------------------|
| VCC        |  D3              | For resetting wifi password |
| GND        |  GND             | Ground              |

##### Configuration
For adjusting the clock to use different time zone, see the wiki

##### Get Started
1. Wire the hardware as described above.
2. Upload the sketch using Arduino IDE (download needed libraries with the internal library manager)
3. Power up the unit
4. - If the NodeMCU doesn't have any WIFI credentials saved, it will start an Access Point (AP) with the name "NTP Clock". Connect to the AP using e.g. your phone, and it will take you directly to a web page where you can connect the AP to your WIFI router - if web page does not appear type in 192.168.4.1 in your browser.
4. - If your NodeMCU has a WIFI credentials saved, the AP will not be started, but will instead connect to the NTP server to adjust the clock. 

Status messages that appears on the TM1637 display:
- INIT = Device starting up
- SETUP = NodeMCU has no wifi credentials
- CONNECT = NodeMCU has been connected to the router
- NO SYNC = NodeMCU could not connect to the NCP server, and will restart the device to make the attempt to connect. (This might happen a few times before it connects..)
- The colon (:) dissapear in the middle of the display, it means it lost the connect to the NTP server; the clock will continue to work, and will sync later if the connection appears.

