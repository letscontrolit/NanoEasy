#if FEATURE_SERIAL_CMD
#define INPUT_COMMAND_SIZE          80
void ExecuteCommand(byte source, const char *Line)
{
  String status = "";
  boolean success = false;
  char TmpStr1[80];
  TmpStr1[0] = 0;
  char Command[80];
  Command[0] = 0;
  int Par1 = 0;
  int Par2 = 0;
  int Par3 = 0;

  GetArgv(Line, Command, 1);
  if (GetArgv(Line, TmpStr1, 2)) Par1 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 3)) Par2 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 4)) Par3 = str2int(TmpStr1);

  // ****************************************
  // commands for debugging
  // ****************************************

  if (strcasecmp_P(Command, PSTR("Reboot")) == 0)
  {
    success = true;
    Reboot();
  }

  if (strcasecmp_P(Command, PSTR("Settings")) == 0)
  {
    success = true;
    char str[20];
    Serial.println();

    Serial.println(F("System Info"));
    IPAddress ip = Ethernet.localIP();
    sprintf_P(str, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
    Serial.print(F("  IP Address    : ")); Serial.println(str);
    Serial.print(F("  Build         : ")); Serial.println((int)BUILD);
    Serial.print(F("  Unit          : ")); Serial.println((int)Settings.Unit);
    Serial.print(F("  Free mem      : ")); Serial.println(FreeMem());
  }
  
  if (success)
    status += F("\nOk");
  else  
    status += F("\nUnknown command!");
  //SendStatus(source,status);
}
#endif

