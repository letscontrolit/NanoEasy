#if FEATURE_UDP_SEND
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
  portUDP.beginPacket(broadcastIP, UDP_PORT);
  portUDP.write(data, 80);
  portUDP.endPacket();
}
#endif

#if FEATURE_UDP_RECV
void UDPCheck()
{
  int size = portUDP.parsePacket();
  if (size == 80)
  {
    byte id1 = portUDP.read();
    byte id2 = portUDP.read();
    Serial.println(id1);
    Serial.println(id2);
  }
  portUDP.flush();
}
#endif

