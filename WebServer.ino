String webdata = "";

//********************************************************************************
// Handle webserver requests
//********************************************************************************
void WebServerHandleClient() {
  EthernetClient client = WebServer.available();
  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    char request[40];
    byte count = 0;
    boolean getrequest = true;
    webdata = "&";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (getrequest)
          if (count < 40)
            request[count++] = c;

        if (c == '\n' && currentLineIsBlank) {

          while (client.available()) { // post data...
            char c = client.read();
            webdata += c;
          }

          byte pos = 5; // for GET, querystring starts at position 5
          if (request[0] == 'P') // for POST, position is 6
            pos = 6;

          if (request[pos] == '?')
          {
            String arg = request;
            arg = arg.substring(pos + 1);
            webdata += arg;
          }

          webdata = URLDecode(webdata.c_str());

          // to save code, we use a very simple approach where webpage names are single char long
          char cmd = request[pos];
          switch (cmd)
          {
            #if FEATURE_WEB_CONFIG_NETWORK 
            case 'n':
              addHeader(true, client);
              handle_network(client, webdata);
              break;
            #endif
            case 'c':
            #if FEATURE_WEB_CONFIG_MAIN 
              addHeader(true, client);
              handle_config(client, webdata);
              break;
            #endif
            case '?':
            #if FEATURE_WEB_CONTROL 
              addHeader(true, client);
              handle_control(client, webdata);
              break;
            #endif
            default:
              addHeader(true, client);
              client.print(F("Uptime:"));
              client.print(wdcounter / 2);
          }
          break;
        }

        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          getrequest = false;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
    webdata = "";
  }
}


//********************************************************************************
// Get specific querystring parameter
//********************************************************************************
String WebServerarg(String arg)
{
  arg = "&" + arg;
  String returnarg = "";
  returnarg.reserve(26);
  int pos = webdata.indexOf(arg);
  if (pos >= 0)
  {
    returnarg = webdata.substring(pos + 1, pos + 27); // max field len set to 26
    pos = returnarg.indexOf("&");
    if (pos > 0)
      returnarg = returnarg.substring(0, pos);
    pos = returnarg.indexOf("=");
    if (pos > 0)
      returnarg = returnarg.substring(pos + 1);
  }
  return returnarg;
}


//********************************************************************************
// Add top menu
//********************************************************************************
void addHeader(boolean showMenu, EthernetClient &client)
{
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));  // the connection will be closed after completion of the response
  client.println();

  client.print(F("<h1>Nano Easy: "));
  client.print(Settings.Name);
  client.print(F("</h1>"));

  if (showMenu)
  {
    client.print(F("<a href=\"n\">Network</a>"));
    client.print(F(" <a href=\"c\">Config</a>"));
  }
  client.print(F("<form method='post'><table>"));
}


//********************************************************************************
// Add footer
//********************************************************************************
void addFooter(EthernetClient &client)
{
  client.print(F("<TR><TD><TD><input type='submit' value='Submit'>"));
  client.print(F("</table></form>"));
}


//********************************************************************************
// Web Interface main config page
//********************************************************************************
void handle_config(EthernetClient &client, String &post) {

  update_config();
  webdata = "";

  client.print(F("<TR><TD>Name:<TD><input type='text' name='n' value='"));
  Settings.Name[25] = 0;
  client.print(Settings.Name);

  client.print(F("'><TR><TD>Unit:<TD><input type='text' name='u' value='"));
  client.print(Settings.Unit);

  char str[20];

  client.print(F("'><TR><TD>Controller<TR><TD>IP:<TD><input type='text' name='cip' value='"));
  ip2str(str, Settings.Controller_IP);
  client.print(str);

  client.print(F("'><TR><TD>Port:<TD><input type='text' name='cp' value='"));
  client.print(Settings.ControllerPort);
  client.print(F("'>"));

#if FEATURE_HTTP_AUTH
  client.print(F("'><TR><TD>User:<TD><input type='text' name='cu' value='"));
  client.print(Settings.ControllerUser);

  client.print(F("'><TR><TD>Password:<TD><input type='text' name='cpw' value='"));
  client.print(Settings.ControllerPassword);
  client.print(F("'>"));
#endif

  client.print(F("<TR><TD>Sensor<TR><TD>Delay:<TD><input type='text' name='d' value='"));
  client.print(Settings.Delay);
  client.print(F("'>"));

  client.print(F("<TR><TD>IDX/Var:<TD><input type='text' name='idx' value='"));
  client.print(Settings.IDX);
  client.print(F("'>"));

  client.print(F("<TR><TD>Pin:<TD><input type='text' name='pin' value='"));
  client.print(Settings.Pin);
  client.print(F("'>"));

  client.print(F("<TR><TD>Mode:<TD><input type='text' name='m' value='"));
  client.print(Settings.Mode);
  client.print(F("'>"));

  addFooter(client);
}


void update_config()
{
  char tmpString[64];

  String arg = "";
  arg = WebServerarg(F("n"));

  if (arg[0] != 0)
  {
    strncpy(Settings.Name, arg.c_str(), sizeof(Settings.Name));

    arg = WebServerarg(F("cip"));
    arg.toCharArray(tmpString, 26);
    str2ip(tmpString, Settings.Controller_IP);

    arg = WebServerarg(F("cp"));
    Settings.ControllerPort = arg.toInt();

#if FEATURE_HTTP_AUTH
    arg = WebServerarg(F("cu"));
    strncpy(Settings.ControllerUser, arg.c_str(), sizeof(Settings.ControllerUser));
    arg = WebServerarg(F("cpw"));
    strncpy(Settings.ControllerPassword, arg.c_str(), sizeof(Settings.ControllerPassword));
#endif

    arg = WebServerarg(F("d"));
    Settings.Delay = arg.toInt();
    arg = WebServerarg(F("u"));
    Settings.Unit = arg.toInt();
    arg = WebServerarg(F("idx"));
    Settings.IDX = arg.toInt();
    arg = WebServerarg(F("pin"));
    Settings.Pin = arg.toInt();
    arg = WebServerarg(F("m"));
    Settings.Mode = arg.toInt();

    SaveSettings();
  }
}


//********************************************************************************
// Web Interface network config page
//********************************************************************************
void handle_network(EthernetClient &client, String &post) {

  update_network();
  webdata = "";

  char str[20];

  client.print(F("<TR><TD>IP:<TD><input type='text' name='ip' value='"));
  ip2str(str, Settings.IP);
  client.print(str);

  client.print(F("'><TR><TD>GW:<TD><input type='text' name='gw' value='"));
  ip2str(str, Settings.Gateway);
  client.print(str);

  client.print(F("'><TR><TD>Subnet:<TD><input type='text' name='sn' value='"));
  ip2str(str, Settings.Subnet);
  client.print(str);

  client.print(F("'><TR><TD>DNS:<TD><input type='text' name='dns' value='"));
  ip2str(str, Settings.DNS);
  client.print(str);
  client.print(F("'>"));

  addFooter(client);
}


void update_network()
{
  char tmpString[64];

  String arg = "";
  arg = WebServerarg(F("ip"));

  if (arg[0] != 0)
  {
    arg = WebServerarg(F("ip"));
    arg.toCharArray(tmpString, 26);
    str2ip(tmpString, Settings.IP);
    arg = WebServerarg(F("gw"));
    arg.toCharArray(tmpString, 26);
    str2ip(tmpString, Settings.Gateway);
    arg = WebServerarg(F("sn"));
    arg.toCharArray(tmpString, 26);
    str2ip(tmpString, Settings.Subnet);
    arg = WebServerarg(F("dns"));
    arg.toCharArray(tmpString, 26);
    str2ip(tmpString, Settings.DNS);
    SaveSettings();
  }
}


//********************************************************************************
// Web Interface control page
//********************************************************************************
void handle_control(EthernetClient &client, String &post) {

  String webrequest = WebServerarg(F("cmd"));
  // gpio,1,1
  char cmd[10];
  webrequest.toCharArray(cmd, 10);
  byte pin;
  byte val;
  if (cmd[6] == ',')
  {
    pin = cmd[5] - 48;
    val = cmd[7] - 48;
  }
  else
  {
    pin = 10 + cmd[6] - 48;
    val = cmd[8] - 48;
  }

  if (pin > 1 && pin < 10)
  {
#if FEATURE_SERIAL_DEBUG
    Serial.println(pin);
    Serial.println(val);
#endif
    pinMode(pin, OUTPUT);
    digitalWrite(pin, val);
  }
}


//********************************************************************************
// Decode special characters in URL of get/post data
//********************************************************************************
String URLDecode(const char *src)
{
  String rString;
  const char* dst = src;
  char a, b;

  while (*src) {

    if (*src == '+')
    {
      rString += ' ';
      src++;
    }
    else
    {
      if ((*src == '%') &&
          ((a = src[1]) && (b = src[2])) &&
          (isxdigit(a) && isxdigit(b))) {
        if (a >= 'a')
          a -= 'a' - 'A';
        if (a >= 'A')
          a -= ('A' - 10);
        else
          a -= '0';
        if (b >= 'a')
          b -= 'a' - 'A';
        if (b >= 'A')
          b -= ('A' - 10);
        else
          b -= '0';
        rString += (char)(16 * a + b);
        src += 3;
      }
      else {
        rString += *src++;
      }
    }
  }
  return rString;
}


#if FEATURE_HTTP_AUTH
//********************************************************************************
// Base64 encoding for HTTP authentication
//********************************************************************************
int b64_encode(const char* aInput, int aInputLen, char* aOutput, int aOutputLen)
{
  // Work out if we've got enough space to encode the input
  // Every 6 bits of input becomes a byte of output
  if (aOutputLen < (aInputLen * 8) / 6)
  {
    // FIXME Should we return an error here, or just the length
    return (aInputLen * 8) / 6;
  }

  // If we get here we've got enough space to do the encoding

  const char* b64_dictionary = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  if (aInputLen == 3)
  {
    aOutput[0] = b64_dictionary[aInput[0] >> 2];
    aOutput[1] = b64_dictionary[(aInput[0] & 0x3) << 4 | (aInput[1] >> 4)];
    aOutput[2] = b64_dictionary[(aInput[1] & 0x0F) << 2 | (aInput[2] >> 6)];
    aOutput[3] = b64_dictionary[aInput[2] & 0x3F];
  }
  else if (aInputLen == 2)
  {
    aOutput[0] = b64_dictionary[aInput[0] >> 2];
    aOutput[1] = b64_dictionary[(aInput[0] & 0x3) << 4 | (aInput[1] >> 4)];
    aOutput[2] = b64_dictionary[(aInput[1] & 0x0F) << 2];
    aOutput[3] = '=';
  }
  else if (aInputLen == 1)
  {
    aOutput[0] = b64_dictionary[aInput[0] >> 2];
    aOutput[1] = b64_dictionary[(aInput[0] & 0x3) << 4];
    aOutput[2] = '=';
    aOutput[3] = '=';
  }
  else
  {
    // Break the input into 3-byte chunks and process each of them
    int i;
    for (i = 0; i < aInputLen / 3; i++)
    {
      b64_encode(&aInput[i * 3], 3, &aOutput[i * 4], 4);
    }
    if (aInputLen % 3 > 0)
    {
      // It doesn't fit neatly into a 3-byte chunk, so process what's left
      b64_encode(&aInput[i * 3], aInputLen % 3, &aOutput[i * 4], aOutputLen - (i * 4));
    }
  }
}
#endif

