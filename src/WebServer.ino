String webdata = "";

//********************************************************************************
// Handle webserver requests
//********************************************************************************
void WebServerHandleClient() {
  int freeMem = FreeMem();
  client = WebServer.available();
  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    char request[40];
    byte count = 0;
    boolean getrequest = true;
    webdata = "";
    unsigned long timer = millis() + 2000;
    boolean timeOut = false;
    while (client.connected()  && millis() < timer) {
      if(millis() >= timer)
        timeOut = true;
      if (client.available()) {
        char c = client.read();
        #if FEATURE_SERIAL_DEBUG
        Serial.write(c);
        #endif
        if (getrequest)
          if (count < 40)
            request[count++] = c;

        if (c == '\n' && currentLineIsBlank) {

          if(client.available())
            webdata = "&";
          while (client.available()) { // post data...
            char c = client.read();
            webdata += c;
          }

          byte pos = 5; // for GET, querystring starts at position 5
          if (request[0] == 'P') // for POST, position is 6
            pos = 6;

          // add get querystring to post data
          int posQS = -1;
          for (byte x=0; x<40;x++)
            if (request[x] == '?'){
               posQS = x;
               break;
            }
          if (posQS >= 0)
          {
            for (byte x=posQS; x<count; x++)
              if (request[x] == ' ') request[x]=0; // strip remainder of "GET /?cmd=gpio,9,1 HTTP/1.1
            String arg = request;
            arg = arg.substring(posQS + 1);
            webdata += "&" + arg;
          }

          webdata = URLDecode(webdata.c_str());
          // to save code, we use a very simple approach where webpage names are single char long
          char cmd = request[pos];
          switch (cmd)
          {
            #if FEATURE_WEB_CONFIG_NETWORK 
            case 'n':
              addHeader(true,true);
              handle_network(webdata);
              break;
            #endif
            case 'c':
            #if FEATURE_WEB_CONFIG_MAIN 
              addHeader(true,true);
              handle_config(webdata);
              break;
            #endif
            case '?':
            #if FEATURE_WEB_CONTROL 
              addHeader(true,true);
              handle_control(webdata);
              break;
            #endif
            case 'd':
            {
              #if FEATURE_NODE_LIST
              addHeader(false,false);
              client.print(F("<meta name=\"viewport\" content=\"width=width=device-width, initial-scale=1\"><STYLE>* {font-family:sans-serif; font-size:16pt;}.button {margin:4px; padding:4px 14px; background-color:#07D; color:#FFF; text-decoration:none; border-radius:4px}</STYLE>"));
              client.print(Settings.Unit);
              client.print(F(" - "));
              client.print(Settings.Name);
              // create <> navigation buttons
              byte prev=Settings.Unit;
              byte next=Settings.Unit;
              for (byte x = Settings.Unit-1; x > 0; x--)
                if (Nodes[x].octet != 0) {prev = x; break;}
              for (byte x = Settings.Unit+1; x < UNIT_MAX; x++)
                if (Nodes[x].octet != 0) {next = x; break;}
             char url[40];
             IPAddress ip = Ethernet.localIP();
             sprintf_P(url, PSTR("http://%u.%u.%u.%u/dashboard.esp"), ip[0], ip[1], ip[2], Nodes[prev].octet);
             client.print(F("<a class='button link' href="));
             client.print(url);
             client.print(F(">&lt;</a>"));
             sprintf_P(url, PSTR("http://%u.%u.%u.%u/dashboard.esp"), ip[0], ip[1], ip[2], Nodes[next].octet);
             client.print(F("<a class='button' href="));
             client.print(url);
             client.print(F(">&gt;</a>"));

             client.print(F("<BR><BR><a class='button' href=/d?cmd=gpio,"));
             client.print(Settings.Pin);
             client.print(F(",1>On</a>"));
             client.print(F("<a class='button' href=/d?cmd=gpio,"));
             client.print(Settings.Pin);
             client.print(F(",0>Off</a>"));

              handle_control(webdata);

            #endif
            break;
            }
            default:
              addHeader(true,true);
              #ifdef __ENC28J60__
                client.print(F("Build:"));
                client.print(BUILD);
                client.print(F("<BR>Uptime:"));
                client.print(wdcounter / 2);
                client.print(F("<BR>FreeMem:"));
                client.print(freeMem);
              #else
                #if FEATURE_WEB_ROOT
                  handle_root(webdata);
                #endif
              #endif
          }
          break;
        }

        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          getrequest = false;
          request[count] = 0;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(100);
    client.stop();
    if(timeOut)
    {
      #ifdef __ENC28J60__
        Enc28J60.init(mac);
      #endif
    }
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
void addHeader(boolean showTitle, boolean showMenu)
{
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));  // the connection will be closed after completion of the response
  client.println();

  #ifndef __ENC28J60__
    client.println(F("<style>"));
    client.println(F("* {font-family:sans-serif; font-size:12pt;}"));
    client.println(F(".button {margin:4px; padding:4px 16px; background-color:#07D; color:#FFF; text-decoration:none; border-radius:4px}"));
    client.println(F("th {padding:10px; background-color:black; color:#ffffff;}"));
    client.println(F("td {padding:7px;}"));
    client.println(F("table {color:black;}"));
    client.println(F("</style>"));
  #endif
  
  if (showTitle)
  {
    client.print(F("<h1>Nano Easy: "));
    client.print(Settings.Name);
    client.print(F("</h1>"));
  }
  
  if (showMenu)
  {
  #ifdef __ENC28J60__
    client.print(F("<a href=\".\">Main</a>"));
    client.print(F(" <a href=\"n\">Network</a>"));
    client.print(F(" <a href=\"c\">Config</a>"));
  #else
    client.print(F("<a class=\"button\" href=\".\">Main</a>"));
    client.print(F("<a class=\"button\" href=\"n\">Network</a>"));
    client.print(F("<a class=\"button\" href=\"c\">Config</a><BR><BR>"));
  #endif
  }
  client.print(F("<form method='post'><table>"));
}


//********************************************************************************
// Add footer
//********************************************************************************
void addFooter(boolean button = false)
{
  if (button)
    client.print(F("<TR><TD><TD><input type='submit' value='Submit'>"));
  client.print(F("</table></form>"));
}


//********************************************************************************
// Web Interface root page
//********************************************************************************
#if FEATURE_WEB_ROOT
void handle_root(String &post) {

  //if (!isLoggedIn()) return;

    IPAddress ip = Ethernet.localIP();
    IPAddress gw = Ethernet.gatewayIP();

    client.print(F("<table><TH>System Info<TH>Value<TH><TH>System Info<TH>Value"));

    client.print(F("<TR><TD>Unit:<TD>"));
    client.print(Settings.Unit);

    client.print(F("<TD><TD>Build:<TD>"));
    client.print(BUILD);

    client.print(F("<TR><TD>No System Time:<TD>"));
    client.print(F("<TD><TD>Uptime:<TD>"));
    char strUpTime[40];
    int minutes = wdcounter / 2;
    int days = minutes / 1440;
    minutes = minutes % 1440;
    int hrs = minutes / 60;
    minutes = minutes % 60;
    sprintf_P(strUpTime, PSTR("%d days %d hours %d minutes"), days, hrs, minutes);
    client.print(strUpTime);

    client.print(F("<TR><TD>Load:<TD>"));
    if (wdcounter > 0)
    {
      //reply += 100 - (100 * loopCounterLast / loopCounterMax);
      client.print(F("% (LC="));
      //reply += int(loopCounterLast / 30);
      client.print(F(")"));
    }

    client.print(F("<TD><TD>Free Mem:<TD>"));
    client.print(FreeMem());

    char str[20];
    sprintf_P(str, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
    client.print(F("<TR><TD>IP:<TD>"));
    client.print(str);

    sprintf_P(str, PSTR("%u.%u.%u.%u"), gw[0], gw[1], gw[2], gw[3]);
    client.print(F("<TD><TD>GW:<TD>"));
    client.print(str);

    #if FEATURE_NODE_LIST
    #if FEATURE_NODELIST_NAMES
        client.print(F("<TR><TH>Node List:<TH>Build<TH>Name<TH>IP<TH>Age<TH><TR><TD><TD><TD>"));
    #else
        client.print(F("<TR><TH>Node List:<TH>Build<TH>IP<TH>Age<TH><TR><TD><TD><TD>"));
    #endif
    for (byte x = 0; x < UNIT_MAX; x++)
    {
      if (Nodes[x].octet != 0)
      {
        char url[80];
        IPAddress ip = Ethernet.localIP();
        sprintf_P(url, PSTR("<a class='button' href='http://%u.%u.%u.%u'>%u.%u.%u.%u</a>"), ip[0], ip[1], ip[2], Nodes[x].octet, ip[0], ip[1], ip[2], Nodes[x].octet);
        client.print(F("<TR><TD>Unit "));
        client.print(x);
        #if FEATURE_NODELIST_NAMES
        client.print(F("<TD>"));
        if (x != Settings.Unit)
          client.print(Nodes[x].nodeName);
        else
          client.print(Settings.Name);
        #endif
        client.print(F("<TD>"));
        //if (Nodes[x].build)
        //  reply += Nodes[x].build;
        client.print(F("<TD>"));
        client.print(url);
        client.print(F("<TD>"));
        client.print(Nodes[x].age);
      }
    }
    #endif
    client.print(F("</table>"));
    addFooter();
}
#endif

//********************************************************************************
// Web Interface main config page
//********************************************************************************
void handle_config(String &post) {

  update_config();
  webdata = "";
  addFormTextBox(F("Name"), '1', Settings.Name);
  Settings.Name[25] = 0;
  addFormNumericBox(F("Unit"), '2', Settings.Unit);
  addFormIPBox(F("Controller IP"), '3', Settings.Controller_IP);
  addFormNumericBox(F("Port"), '4', Settings.ControllerPort);
  
#if FEATURE_HTTP_AUTH
  addFormTextBox(F("User"), 'a', Settings.ControllerUser);
  addFormTextBox(F("Password"), 'b', Settings.ControllerPassword);
#endif

  addFormNumericBox(F("Delay"), '5', Settings.Delay);
  addFormNumericBox(F("IDX/Var"), '6', Settings.IDX);
  addFormNumericBox(F("Pin"), '7', Settings.Pin);
  addFormNumericBox(F("Mode"), '8', Settings.Mode);
  addFormIPBox(F("Syslog IP"), '9', Settings.Syslog_IP);
  addFormNumericBox(F("UDP Port"), 'c', Settings.UDPPort);
  addFooter(true);
}


void update_config()
{
  String arg = WebServerarg("1");

  if (arg[0] != 0)
  {
    strncpy(Settings.Name, arg.c_str(), sizeof(Settings.Name));

    Settings.Unit = WebServerarg("2").toInt();
    str2ip(WebServerarg("3").c_str(), Settings.Controller_IP);
    Settings.ControllerPort = WebServerarg("4").toInt();

#if FEATURE_HTTP_AUTH
    strncpy(Settings.ControllerUser, WebServerarg("a").c_str(), sizeof(Settings.ControllerUser));
    strncpy(Settings.ControllerPassword, WebServerarg("b").c_str(), sizeof(Settings.ControllerPassword));
#endif

    Settings.Delay = WebServerarg("5").toInt();
    Settings.IDX = WebServerarg("6").toInt();
    Settings.Pin = WebServerarg("7").toInt();
    Settings.Mode = WebServerarg("8").toInt();
    str2ip(WebServerarg("9").c_str(), Settings.Syslog_IP);
    Settings.UDPPort = WebServerarg("c").toInt();
    SaveSettings();
  }
}


//********************************************************************************
// Web Interface network config page
//********************************************************************************
void handle_network(String &post) {

  update_network();
  webdata = "";
  addFormIPBox(F("IP"), '1', Settings.IP);
  addFormIPBox(F("GW"), '2', Settings.Gateway);
  addFormIPBox(F("Subnet"), '3', Settings.Subnet);
  addFormIPBox(F("DNS"), '4', Settings.DNS);
  addFooter(true);
}


void update_network()
{
  String arg = "";
  arg = WebServerarg("1");

  if (arg[0] != 0)
  {
    str2ip(WebServerarg("1").c_str(), Settings.IP);
    str2ip(WebServerarg("2").c_str(), Settings.Gateway);
    str2ip(WebServerarg("3").c_str(), Settings.Subnet);
    str2ip(WebServerarg("4").c_str(), Settings.DNS);
    SaveSettings();
  }
}


//********************************************************************************
// Web Interface control page
//********************************************************************************
void handle_control(String &post) {

  String webrequest = WebServerarg(F("cmd"));
  if (webrequest == F("reboot"))
  {
    client.stop();
    Reboot();
  }
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

void addFormTextBox(const String& label, char id, const String& value)
{
  addRowLabel(label);
  addTextBox(id, value);
}

void addTextBox(char id, const String&  value)
{
  client.print(F("<input type='text' name='"));
  client.print(id);
  client.print(F("' value='"));
  client.print(value);
  client.print(F("'>"));
}

void addFormNumericBox(const String& label, char id, long value)
{
  addRowLabel(label);
  addNumericBox(id, value);
}

void addNumericBox(char id, long value)
{
  client.print(F("<input type='text' name='"));
  client.print(id);
  client.print(F("' value='"));
  client.print(value);
  client.print(F("'>"));
}

void addFormIPBox(const String& label, char id, const byte ip[4])
{
  char strip[20];
  if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0)
    strip[0] = 0;
  else
    sprintf_P(strip, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);

  addRowLabel(label);
  client.print(F("<input type='text' name='"));
  client.print(id);
  client.print(F("' value='"));
  client.print(strip);
  client.print(F("'>"));
}

void addRowLabel(const String& label)
{
  client.print(F("<TR><TD>"));
  client.print(label);
  client.print(F(":<TD>"));
}

