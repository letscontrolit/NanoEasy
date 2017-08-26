/****************************************************************************************************************************\
   Arduino project "Nano Easy" © Copyright www.letscontrolit.com

   This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
   You received a copy of the GNU General Public License along with this program in file 'License.txt'.

   IDE download    : https://www.arduino.cc/en/Main/Software

   Source Code     : https://github.com/ESP8266nu/ESPEasy
   Support         : http://www.letscontrolit.com
   Discussion      : http://www.letscontrolit.com/forum/

   Additional information about licensing can be found at : http://www.gnu.org/licenses
  \*************************************************************************************************************************/

// This file incorporates work covered by the following copyright and permission notice:

/****************************************************************************************************************************\
  Arduino project "Nodo" © Copyright 2010..2015 Paul Tonkes

  This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
  You received a copy of the GNU General Public License along with this program in file 'License.txt'.

  Voor toelichting op de licentievoorwaarden zie    : http://www.gnu.org/licenses
  Uitgebreide documentatie is te vinden op          : http://www.nodo-domotica.nl
  Compiler voor deze programmacode te downloaden op : http://arduino.cc
  \*************************************************************************************************************************/

// A very limited Easy project as the Nano has only 30k flash for program code and only 2k of RAM.
// We skipped 99% of the Arduino Easy code to get it compiled.
// Due to very limited code size, these things should be avoided or they are just plain impossible...:
//   - DHCP
//   - using floats
//   - using web style sheet elements
//   - using dropdown lists

// That actually leaves us with not much more than a Domoticz HTTP sensor/actuator with a single io pin.
#define __ENC28J60__                            // UNCOMMENT this when compiling for UNO with W5100 ethernet shield
#define DEFAULT_IP          {192,168,0,254}     // default IP if no IP setting
#define DEFAULT_NAME        "new"               // Enter your device friendly name
#define DEFAULT_SERVER      "192.168.0.8"       // Enter your Domoticz Server IP address
#define DEFAULT_PORT        8080                // Enter your Domoticz Server port value
#define DEFAULT_DELAY       60                  // Enter your Send delay in seconds
#define DEFAULT_UNIT        0                   // Enter your default unit number, must be UNIQUE for each unit!
#define UDP_PORT            65500

#ifdef __ENC28J60__
  #define FEATURE_CONTROLLER_DOMOTICZ true
  #define FEATURE_CONTROLLER_MQTT     false // does not work with webconfig, sketch to big...
  #define FEATURE_UDP_SEND            true
  #define FEATURE_UDP_RECV            true
  #define FEATURE_HTTP_AUTH           false
  #define FEATURE_WEB_CONFIG_NETWORK  true
  #define FEATURE_WEB_CONFIG_MAIN     true
  #define FEATURE_WEB_CONTROL         true
  #define FEATURE_NODE_LIST           true
  #define FEATURE_NODE_LIST_FULLIP    false // false is suitable for common C-class home networks and saves 100B RAM!
  #define FEATURE_SERIAL_DEBUG        false
  #define FEATURE_SYSLOG              false //mainly for debugging during development
#else
  #define FEATURE_CONTROLLER_DOMOTICZ true
  #define FEATURE_CONTROLLER_MQTT     false // does not work with webconfig, sketch to big...
  #define FEATURE_UDP_SEND            true
  #define FEATURE_UDP_RECV            true
  #define FEATURE_HTTP_AUTH           false
  #define FEATURE_WEB_CONFIG_NETWORK  true
  #define FEATURE_WEB_CONFIG_MAIN     true
  #define FEATURE_WEB_CONTROL         true
  #define FEATURE_NODE_LIST           true
  #define FEATURE_NODE_LIST_FULLIP    false // false is suitable for common C-class home networks and saves 100B RAM!
  #define FEATURE_SERIAL_DEBUG        true
  #define FEATURE_SERIAL_CMD          true
  #define FEATURE_SYSLOG              true  //mainly for debugging during development
  #define FEATURE_WEB_ROOT            true
#endif

// do not change anything below this line !

#define NANO_PROJECT_PID       2016120701L
#define VERSION                          1
#define BUILD                          150

#define NODE_TYPE_ID_NANO_EASY_STD         81
#define NODE_TYPE_ID                       NODE_TYPE_ID_NANO_EASY_STD

#define UNIT_MAX                           32 // Only relevant for UDP unicast message 'sweeps' and the nodelist.

#include <EEPROM.h>

#ifdef __ENC28J60__
  #include <UIPEthernet.h>
  #include <UIPServer.h>
  #include <UIPClient.h>
#else
  #include <Ethernet.h>
  #include <DNS.h>
#endif

#if FEATURE_CONTROLLER_MQTT
  #include <PubSubClient.h>
  EthernetClient mqtt;
  PubSubClient MQTTclient(mqtt);
#endif

EthernetServer WebServer = EthernetServer(80);
EthernetClient client;

#if FEATURE_UDP_SEND or FEATURE_UDP_RECV
  EthernetUDP portUDP;
#endif

void(*Reboot)(void) = 0;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

struct SettingsStruct
{
  unsigned long PID;
  int           Version;
  byte          Unit;
  int16_t       Build;
  byte          IP[4];
  byte          Gateway[4];
  byte          Subnet[4];
  byte          DNS[4];
  byte          Controller_IP[4];
  unsigned int  ControllerPort;
  unsigned long Delay;
  char          Name[26];
  int16_t       IDX;
  byte          Pin;
  byte          Mode;
  byte          Syslog_IP[4];
  unsigned int  UDPPort;
  #if FEATURE_HTTP_AUTH
    char          ControllerUser[26];
    char          ControllerPassword[64];
  #endif
  #if FEATURE_CONTROLLER_MQTT
    char          MQTTsubscribe[80];
    boolean       MQTTRetainFlag;
  #endif
} Settings;

#if FEATURE_NODE_LIST
struct NodeStruct
{
#if FEATURE_NODE_LIST_FULLIP
  byte ip[4];
#else
  byte octet;
#endif
  byte age;
  //uint16_t build;
} Nodes[UNIT_MAX];
#endif

unsigned long timer;
unsigned long timerwd;
unsigned long wdcounter;

void setup()
{
#if FEATURE_SERIAL_DEBUG
  Serial.begin(115200);
#endif
  LoadSettings();
  if (Settings.Version == VERSION && Settings.PID == NANO_PROJECT_PID)
  {
#if FEATURE_SERIAL_DEBUG
    Serial.println("OK");
#endif
  }
  else
  {
#if FEATURE_SERIAL_DEBUG
    Serial.print("R!");
#endif
    delay(1000);
    ResetFactory();
  }

  if (Settings.Build != BUILD)
  {
    Settings.Build = BUILD;
    SaveSettings();
  }

  mac[5] = Settings.Unit; // make sure every unit has a unique mac address
  IPAddress myIP;
  if (Settings.IP[0] != 0)
    myIP = Settings.IP;
  else
    myIP = DEFAULT_IP;

  Ethernet.begin(mac, myIP);
  //Ethernet.begin(mac, myIP, Settings.DNS, Settings.Gateway, Settings.Subnet);

#if FEATURE_UDP_RECV
    if (Settings.UDPPort != 0)
      portUDP.begin(Settings.UDPPort);
    else
      portUDP.begin(123); // setup for NTP and other stuff if no user port is selected
#endif

  WebServer.begin();

#if FEATURE_CONTROLLER_MQTT
  MQTTConnect();
#endif

  timer = millis() + Settings.Delay * 1000;
  timerwd = millis() + 30000;
#if FEATURE_SYSLOG
  syslog("Boot");
#endif
}

void loop()
{
  #if FEATURE_SERIAL_CMD
    serial();
  #endif
  
  if (millis() > timerwd)
  {
    timerwd = millis() + 30000;
    wdcounter++;
#if FEATURE_SERIAL_DEBUG
    Serial.print(wdcounter / 2);
    Serial.print("-");
    Serial.println(FreeMem());
#endif
#if FEATURE_UDP_SEND
    UDP();
#endif

#if FEATURE_NODE_LIST
    // refresh node list
    for (byte counter = 0; counter < UNIT_MAX; counter++)
    {
#if FEATURE_NODE_LIST_FULLIP
      if (Nodes[counter].ip[0] != 0)
#else
      if (Nodes[counter].octet != 0)
#endif
      {
        Nodes[counter].age++;  // increment age counter
        if (Nodes[counter].age > 10) // if entry to old, clear this node ip from the list.
          for (byte x = 0; x < 4; x++)
#if FEATURE_NODE_LIST_FULLIP
            Nodes[counter].ip[x] = 0;
#else
            Nodes[counter].octet = 0;
#endif
      }
    }
#endif // NODE LIST
  }

  if (Settings.Delay != 0)
  {
    if (millis() > timer)
    {
#if FEATURE_SERIAL_DEBUG
      Serial.println("S");
#endif
      timer = millis() + Settings.Delay * 1000;
      int value = 0;
      switch (Settings.Mode)
      {
        case 1:
          value = analogRead(Settings.Pin);
          break;
        case 2:
          value = digitalRead(Settings.Pin);
          break;
      }
#if FEATURE_CONTROLLER_DOMOTICZ
      Domoticz_sendData(Settings.IDX, value);
#endif
#if FEATURE_CONTROLLER_MQTT
      String svalue = String(value);
      MQTTclient.publish(Settings.Name, svalue.c_str());
#endif
    }
  }
  WebServerHandleClient();
#if FEATURE_UDP_RECV
  UDPCheck();
#endif
#if FEATURE_CONTROLLER_MQTT
  MQTTclient.loop();
#endif
}

