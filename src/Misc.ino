//********************************************************************************
void SaveSettings(void)
//********************************************************************************
{
  char ByteToSave, *pointerToByteToSave = pointerToByteToSave = (char*)&Settings; //pointer to settings struct

  for (int x = 0; x < sizeof(struct SettingsStruct) ; x++)
  {
    EEPROM.write(x, *pointerToByteToSave);
    pointerToByteToSave++;
  }
}


//********************************************************************************
boolean LoadSettings()
//********************************************************************************
{
  byte x;

  char ByteToSave, *pointerToByteToRead = (char*)&Settings; //pointer to settings struct

  for (int x = 0; x < sizeof(struct SettingsStruct); x++)
  {
    *pointerToByteToRead = EEPROM.read(x);
    pointerToByteToRead++;// next byte
  }
}


/********************************************************************************************\
  Reset all settings to factory defaults
  \*********************************************************************************************/
void ResetFactory(void)
{
  Settings.PID                 = NANO_PROJECT_PID;
  Settings.Version             = VERSION;
  Settings.Unit                = DEFAULT_UNIT;
  str2ip((char*)DEFAULT_SERVER, Settings.Controller_IP);
  Settings.ControllerPort      = DEFAULT_PORT;
  Settings.Delay               = DEFAULT_DELAY;
  strcpy_P(Settings.Name, PSTR(DEFAULT_NAME));
  Settings.Build               = BUILD;
  SaveSettings();
  delay(1000);
  Reboot();
}


/********************************************************************************************\
  Convert a char string to IP byte array
  \*********************************************************************************************/
boolean str2ip(const char *string, byte* IP)
{
  byte c;
  byte part = 0;
  int value = 0;

  for (int x = 0; x <= strlen(string); x++)
  {
    c = string[x];
    if (isdigit(c))
    {
      value *= 10;
      value += c - '0';
    }

    else if (c == '.' || c == 0) // next octet from IP address
    {
      if (value <= 255)
        IP[part++] = value;
      else
        return false;
      value = 0;
    }
    else if (c == ' ') // ignore these
      ;
    else // invalid token
      return false;
  }
  if (part == 4) // correct number of octets
    return true;
  return false;
}


/********************************************************************************************\
  Convert a char string to IP byte array
  \*********************************************************************************************/
void ip2str(char *string, byte ip[4])
{
  sprintf_P(string, PSTR("%u.%u.%u.%u"), ip[0],ip[1],ip[2],ip[3]);
}

/********************************************************************************************\
  Get free system mem
\*********************************************************************************************/
uint8_t *heapptr, *stackptr;

unsigned long FreeMem(void)
  {
  stackptr = (uint8_t *)malloc(4);        // use stackptr temporarily
  heapptr = stackptr;                     // save value of heap pointer
  free(stackptr);                         // free up the memory again (sets stackptr to 0)
  stackptr =  (uint8_t *)(SP);            // save value of stack pointer
  return (stackptr-heapptr);
}

