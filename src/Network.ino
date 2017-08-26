#if FEATURE_UDP_SEND
#if FEATURE_SYSLOG
void syslog(const char *message)
{
 if (Settings.Syslog_IP[0] != 0)
  {
    IPAddress broadcastIP(Settings.Syslog_IP[0], Settings.Syslog_IP[1], Settings.Syslog_IP[2], Settings.Syslog_IP[3]);
    portUDP.beginPacket(broadcastIP, 514);
    char str[80];
    str[0] = 0;
    snprintf_P(str, sizeof(str), PSTR("<7>Unit: %u : %s"), Settings.Unit, message);
    portUDP.write(str);
    portUDP.endPacket();
  }
}
#endif

void UDP()
{
  //uint8_t mac[] = {0, 0, 0, 0, 0, 0};
  //uint8_t* macread = WiFi.macAddress(mac);
  byte data[80];
  data[0] = 255;
  data[1] = 1;
  //for (byte x = 0; x < 6; x++)
  //  data[x + 2] = macread[x];
  IPAddress ip = Ethernet.localIP();
  for (byte x = 0; x < 4; x++)
    data[x + 8] = ip[x];
  data[12] = Settings.Unit;
  data[13] = Settings.Build & 0xff;
  data[14] = Settings.Build >> 8;
  memcpy((byte*)data + 15, Settings.Name, 25);
  data[40] = NODE_TYPE_ID;

  IPAddress broadcastIP(255, 255, 255, 255);
  portUDP.beginPacket(broadcastIP, Settings.UDPPort);
  portUDP.write(data, 80);
  portUDP.endPacket();

  #if FEATURE_NODE_LIST 
  // store my own info also in the list...
  if (Settings.Unit < UNIT_MAX)
  {
    IPAddress ip = Ethernet.localIP();
    #if FEATURE_NODE_LIST_FULLIP
      for (byte x = 0; x < 4; x++)
        Nodes[Settings.Unit].ip[x] = ip[x];
    #else
        Nodes[Settings.Unit].octet = ip[3];
    #endif    
    Nodes[Settings.Unit].age = 0;
    //Nodes[Settings.Unit].build = Settings.Build;
  }
  #endif    
}
#endif

#if FEATURE_UDP_RECV
void UDPCheck()
{
  int size = portUDP.parsePacket();
  #if FEATURE_SERIAL_DEBUG
  if (size)
  {
    //Serial.print("U:");
    //Serial.println(size);
  }
  #endif
  if (size == 80)
  {
    byte packetBuffer[80];
    int len = portUDP.read(packetBuffer, 80);
    if (packetBuffer[0]==255 && packetBuffer[1] == 1)
    {
      #if FEATURE_SERIAL_DEBUG 
        Serial.print(F("Unit:"));
        Serial.println(packetBuffer[12]);
      #endif

      #if FEATURE_NODE_LIST
      byte mac[6];
      byte ip[4];
      byte unit = packetBuffer[12];
      for (byte x = 0; x < 6; x++)
        mac[x] = packetBuffer[x + 2];
      for (byte x = 0; x < 4; x++)
        ip[x] = packetBuffer[x + 8];

      if (unit < UNIT_MAX)
      {
        #if FEATURE_NODE_LIST_FULLIP
        for (byte x = 0; x < 4; x++)
          Nodes[unit].ip[x] = packetBuffer[x + 8];
        #else
          Nodes[unit].octet = packetBuffer[11];
        #endif
        Nodes[unit].age = 0; // reset 'age counter'
        if (len >20) // extended packet size
        {
          //Nodes[unit].build = packetBuffer[13] + 256*packetBuffer[14];
        }
      }
      #endif  
    }
    #ifdef __ENC28J60__    
      portUDP.stop();
      portUDP.begin(Settings.UDPPort);
    #endif
  }
  #ifndef __ENC28J60__    
    portUDP.flush();
  #endif
}
#endif

