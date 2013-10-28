/***************************************************
  WiFi Candy Bowl Monitor With Multicast DNS Support
    
  Monitor the status of your Halloween candy bowl remotely
  with the CC3000 and an Arduino.  This sketch uses and IR
  sensor and LED to detect if a candy bowl is full or empty,
  and exposes that sensor state to a WiFi network through a
  simple telnet server.  A multicast DNS query responder also
  runs to make the telnet server available over the
  'candybowl.local' address on your network.
  
  See the Adafruit learning system guide for more details
  and usage information:
    http://learn.adafruit.com/wifi-candy-bowl/overview

  License:
 
  This example is copyright (c) 2013 Tony DiCola (tony@tonydicola.com)
  and is released under an open source MIT license.  See details at:
    http://opensource.org/licenses/MIT
  
  This code was adapted from Adafruit CC3000 library example 
  code which has the following license:
  
  Designed specifically to work with the Adafruit WiFi products:
  ----> https://www.adafruit.com/products/1469

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution      
 ****************************************************/
#include <Adafruit_CC3000.h>
#include <SPI.h>
#include <CC3000_MDNS.h>

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIV2); // you can change this clock speed

#define WLAN_SSID       "myNetwork"           // cannot be longer than 32 characters!
#define WLAN_PASS       "myPassword"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define LISTEN_PORT           23    // What TCP port to listen on for connections.
#define IR_LED                8     // Digital pin that is hooked up to the IR LED.
#define IR_SENSOR             7     // Digital pin that is hooked up to the IR sensor.

// Create an instance of the CC3000 server listening on the specified port.
Adafruit_CC3000_Server candyServer(LISTEN_PORT);

// Create an instance of the multicast DNS query responder.
MDNSResponder mdns;

void setup(void)
{
  // Set up the input and output pins.
  pinMode(IR_LED, OUTPUT);
  pinMode(IR_SENSOR, INPUT);
  
  // Set up the serial port connection.
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n")); 
  
  // Set up the CC3000, connect to the access point, and get an IP address.
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
  Serial.println(F("Connected!"));
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100);
  }  
  while (!displayConnectionDetails()) {
    delay(1000);
  }
  
  // Set up the multicast DNS query responder to resolve 'candybowl.local' address queries.
  if (!mdns.begin("candybowl", cc3000, 3600)) {
    Serial.println("Error setting up MDNS responder!");
    while(1); 
  }
   
  // Start the candy bowl server.
  candyServer.begin();
  
  Serial.println(F("Listening for connections..."));
}

void loop(void)
{
  // Handle any multicast DNS requests
  mdns.update();
  
  // Handle a connected client.
  Adafruit_CC3000_ClientRef client = candyServer.available();
  if (client) {
     // Check if there is data available to read.
     if (client.available() > 0) {
       uint8_t ch = client.read();
       // Respond to candy a bowl status query.
       if (ch == '?') {
         client.fastrprint("Candy bowl status: ");
         if (isBowlFull()) {
           client.fastrprintln("FULL");
         }
         else {
           client.fastrprintln("LOW");
         }
       }
     }
  }
}

// Return true if the bowl is detected to be full.
boolean isBowlFull() {
  // Pulse the IR LED at 38khz for 1 millisecond
  pulseIR(1000);
  // Check if the IR sensor picked up the pulse (i.e. output wire went to ground).
  if (digitalRead(IR_SENSOR) == LOW) {
    return false; // Sensor can see LED, return not full.
  }
  return true; // Sensor can't see LED, return full.
}

// 38khz IR pulse function from Adafruit tutorial: http://learn.adafruit.com/ir-sensor/overview
void pulseIR(long microsecs) {
  // we'll count down from the number of microseconds we are told to wait
 
  cli();  // this turns off any background interrupts
 
  while (microsecs > 0) {
    // 38 kHz is about 13 microseconds high and 13 microseconds low
   digitalWrite(IR_LED, HIGH);  // this takes about 3 microseconds to happen
   delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working
   digitalWrite(IR_LED, LOW);   // this also takes about 3 microseconds
   delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working
 
   // so 26 microseconds altogether
   microsecs -= 26;
  }
 
  sei();  // this turns them back on
}

// Display connection details like the IP address.
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

