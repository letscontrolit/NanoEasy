#if FEATURE_CONTROLLER_DOMOTICZ
boolean Domoticz_sendData(int idx, int value)
{
  boolean success = false;

#if FEATURE_HTTP_AUTH
  String authHeader = "";
  if ((Settings.ControllerUser[0] != 0) && (Settings.ControllerPassword[0] != 0))
  {
    char out[80];
    String auth = Settings.ControllerUser;
    auth += ":";
    auth += Settings.ControllerPassword;
    b64_encode(auth.c_str(), auth.length(), out, 80);
    authHeader = F("Authorization: Basic ");
    authHeader += out;
    authHeader += F(" \r\n");
  }
#endif

  // Use WiFiClient class to create TCP connections
  EthernetClient client;
  if (!client.connect(Settings.Controller_IP, Settings.ControllerPort)) {
    return false;
  }

  // We now create a URI for the request
  String url = F("GET /json.htm?type=command&param=udevice&idx=");
  url += idx;
  url += F("&svalue=");
  url += value;
  url += F(" HTTP/1.1\r\n");
#if FEATURE_HTTP_AUTH
  url += authHeader;
#endif
  url += F("\r\nConnection: close\r\n\r\n");

  client.print(url);

  unsigned long timer = millis() + 200;
  while (!client.available() && millis() < timer) {}

  // we can skip this check to save code...
  /*

    // Read all the lines of the reply from server and print them to Serial
    while (client.available()) {
      String line = client.readStringUntil('\n');
      //Serial.println(line);
      if (line.substring(0, 15) == F("HTTP/1.1 200 OK"))
      {
        Serial.println(F("S_OK"));
        success = true;
      }
    }
  */
  client.stop();
  return success;
}
#endif

#if FEATURE_CONTROLLER_MQTT
/*********************************************************************************************\
   Handle incoming MQTT messages
  \*********************************************************************************************/
/*
  void callback(char* c_topic, byte* b_payload, unsigned int length) {
  char log[256];
  char c_payload[256];
  strncpy(c_payload,(char*)b_payload,length);
  c_payload[length] = 0;
  }
*/

/*********************************************************************************************\
   Connect to MQTT message broker
  \*********************************************************************************************/
void MQTTConnect()
{
  IPAddress MQTTBrokerIP(Settings.Controller_IP);
  MQTTclient.setServer(MQTTBrokerIP, Settings.ControllerPort);
  //MQTTclient.setCallback(callback);

  // MQTT needs a unique clientname to subscribe to broker
  String clientid = "C" + Settings.Unit;
  //String subscribeTo = "";

  for (byte x = 1; x < 3; x++)
  {
    boolean MQTTresult = false;

    MQTTresult = MQTTclient.connect(clientid.c_str());

    if (MQTTresult)
    {
#if FEATURE_SERIAL_DEBUG
      Serial.println("c!");
#endif
      //subscribeTo = Settings.MQTTsubscribe;
      //MQTTclient.subscribe(subscribeTo.c_str());
      break; // end loop if succesfull
    }
    delay(500);
  }
}
#endif

