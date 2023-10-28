// MIT License
// 
// Copyright (c) 2023 Travis Smith
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
// and associated documentation files (the "Software"), to deal in the Software without 
// restriction, including without limitation the rights to use, copy, modify, merge, publish, 
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom 
// the Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or 
// substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


//Network, 6551 ACIA interface emulation

void IO1Hndlr_SwiftLink(uint8_t Address, bool R_Wn);  
void PollingHndlr_SwiftLink();                           
void InitHndlr_SwiftLink();                           
void CycleHndlr_SwiftLink();                           

stcIOHandlers IOHndlr_SwiftLink =
{
  "SwiftLink/Modem",        //Name of handler
  &InitHndlr_SwiftLink,     //Called once at handler startup
  &IO1Hndlr_SwiftLink,      //IO1 R/W handler
  NULL,                     //IO2 R/W handler
  NULL,                     //ROML Read handler, in addition to any ROM data sent
  NULL,                     //ROMH Read handler, in addition to any ROM data sent
  &PollingHndlr_SwiftLink,  //Polled in main routine
  &CycleHndlr_SwiftLink,    //called at the end of EVERY c64 cycle
};


#define NumPageLinkBuffs   99
#define NumPrevURLQueues   8
#define MaxURLHostSize     100
#define MaxURLPathSize     200
#define MaxTagSize         (MaxURLHostSize+MaxURLPathSize)
#define TxMsgMaxSize       128
#define RxQueueSize        (1024*320) 
#define C64CycBetweenRx    2300   //stops NMI from re-asserting too quickly. chars missed in large buffs when lower
#define NMITimeoutnS       300   //if Rx data not read within this time, deassert NMI anyway

// 6551 ACIA interface emulation
//register locations (IO1, DExx)
#define IORegSwiftData     0x00   // Swift Emulation Data Reg
#define IORegSwiftStatus   0x01   // Swift Emulation Status Reg
#define IORegSwiftCommand  0x02   // Swift Emulation Command Reg
#define IORegSwiftControl  0x03   // Swift Emulation Control Reg

//status reg flags
#define SwiftStatusIRQ     0x80   // high if ACIA caused interrupt;
#define SwiftStatusDSR     0x40   // reflects state of DSR line
#define SwiftStatusDCD     0x20   // reflects state of DCD line
#define SwiftStatusTxEmpty 0x10   // high if xmit-data register is empty
#define SwiftStatusRxFull  0x08   // high if receive-data register full
#define SwiftStatusErrOver 0x04   // high if overrun error
#define SwiftStatusErrFram 0x02   // high if framing error
#define SwiftStatusErrPar  0x01   // high if parity error

//command reg flags
#define SwiftCmndRxIRQEn   0x02   // low if Rx IRQ enabled
#define SwiftCmndDefault   0xE0   // Default command reg state

//PETSCII Colors/Special Symbols
#define PETSCIIpurple      0x9c
#define PETSCIIwhite       0x05
#define PETSCIIlightBlue   0x9a
#define PETSCIIyellow      0x9e
#define PETSCIIpink        0x96
#define PETSCIIlightGreen  0x99
#define PETSCIIdarkGrey    0x97
#define PETSCIIgrey        0x98

#define PETSCIIreturn      0x0d
#define PETSCIIrvsOn       0x12
#define PETSCIIrvsOff      0x92
#define PETSCIIclearScreen 0x93
#define PETSCIIcursorUp    0x91
#define PETSCIIhorizBar    0x60
#define PETSCIIspace       0x20

#define RxQueueUsed ((RxQueueHead>=RxQueueTail)?(RxQueueHead-RxQueueTail):(RxQueueHead+RxQueueSize-RxQueueTail))

struct stcURLParse
{
   char host[MaxURLHostSize];
   uint16_t port;
   char path[MaxURLPathSize];
};

extern volatile uint32_t CycleCountdown;
extern void EEPreadBuf(uint16_t addr, uint8_t* buf, uint8_t len);
extern void EEPwriteBuf(uint16_t addr, const uint8_t* buf, uint8_t len);
void AddBrowserCommandsToRxQueue();

uint8_t* RxQueue = NULL;  //circular queue to pipe data to the c64 
char* TxMsg = NULL;  //to hold messages (AT/browser commands) when off line
char* PageLinkBuff[NumPageLinkBuffs]; //hold links from tags for user selection in browser
stcURLParse* PrevURLQueue[NumPrevURLQueues]; //For browse previous

uint8_t  PrevURLQueueNum;   //where we are in the link history queue
uint8_t  UsedPageLinkBuffs;   //how many PageLinkBuff elements have been Used
uint32_t  RxQueueHead, RxQueueTail, TxMsgOffset;
bool ConnectedToHost, BrowserMode, PagePaused, PrintingHyperlink;
uint32_t PageCharsReceived;
uint32_t NMIassertMicros;
volatile uint8_t SwiftTxBuf, SwiftRxBuf;
volatile uint8_t SwiftRegStatus, SwiftRegCommand, SwiftRegControl;
uint8_t PlusCount;
uint32_t LastTxMillis = millis();


// Browser mode: Buffer saved in ASCII from host, converted before sending out
//               Uses Send...Immediate  commands for direct output
// AT/regular:   Buffer saved in (usually) PETSCII from host
//               Uses Add...ToRxQueue for direct output


FLASHMEM bool EthernetInit()
{
   uint32_t beginWait = millis();
   uint8_t  mac[6];
   bool retval = true;
   Serial.print("\nEthernet init ");
   
   EEPreadBuf(eepAdMyMAC, mac, 6);

   if (EEPROM.read(eepAdDHCPEnabled))
   {
      Serial.print("via DHCP... ");

      uint16_t DHCPTimeout, DHCPRespTO;
      EEPROM.get(eepAdDHCPTimeout, DHCPTimeout);
      EEPROM.get(eepAdDHCPRespTO, DHCPRespTO);
      if (Ethernet.begin(mac, DHCPTimeout, DHCPRespTO) == 0)
      {
         Serial.println("*Failed!*");
         // Check for Ethernet hardware present
         if (Ethernet.hardwareStatus() == EthernetNoHardware) Serial.println("Ethernet HW was not found.");
         else if (Ethernet.linkStatus() == LinkOFF) Serial.println("Ethernet cable is not connected.");   
         retval = false;
      }
      else
      {
         Serial.println("passed.");
      }
   }
   else
   {
      Serial.println("using Static");
      uint32_t ip, dns, gateway, subnetmask;
      EEPROM.get(eepAdMyIP, ip);
      EEPROM.get(eepAdDNSIP, dns);
      EEPROM.get(eepAdGtwyIP, gateway);
      EEPROM.get(eepAdMaskIP, subnetmask);
      Ethernet.begin(mac, ip, dns, gateway, subnetmask);
   }
   
   Serial.printf("Took %d mS\nIP: ", (millis() - beginWait));
   Serial.println(Ethernet.localIP());
   return retval;
}
   
FLASHMEM void SetEthEEPDefaults()
{
   EEPROM.write(eepAdDHCPEnabled, 1); //DHCP enabled
   uint8_t buf[6]={0xBE, 0x0C, 0x64, 0xC0, 0xFF, 0xEE};
   EEPwriteBuf(eepAdMyMAC, buf, 6);
   EEPROM.put(eepAdMyIP       , (uint32_t)IPAddress(192,168,1,10));
   EEPROM.put(eepAdDNSIP      , (uint32_t)IPAddress(192,168,1,1));
   EEPROM.put(eepAdGtwyIP     , (uint32_t)IPAddress(192,168,1,1));
   EEPROM.put(eepAdMaskIP     , (uint32_t)IPAddress(255,255,255,0));
   EEPROM.put(eepAdDHCPTimeout, (uint16_t)9000);
   EEPROM.put(eepAdDHCPRespTO , (uint16_t)4000);   
}
   
uint8_t PullFromRxQueue()
{  //assumes queue data is available before calling
  uint8_t c = RxQueue[RxQueueTail++]; 
  if (RxQueueTail == RxQueueSize) RxQueueTail = 0;
  //Printf_dbg("Pull H=%d T=%d Char=%c\n", RxQueueHead, RxQueueTail, c);
  return c;
}

bool ReadyToSendRx()
{
   //  if IRQ enabled, 
   //  and IRQ not set, 
   //  and enough time has passed
   //  then C64 is ready to receive...
   return ((SwiftRegCommand & SwiftCmndRxIRQEn) == 0 && \
      (SwiftRegStatus & (SwiftStatusRxFull | SwiftStatusIRQ)) == 0 && \
      CycleCountdown == 0);
}

bool CheckRxNMITimeout()
{
   //Check for Rx NMI timeout: Doesn't happen unless a lot of serial printing enabled (ie DbgMsgs_IO) causing missed reg reads
   if ((SwiftRegStatus & SwiftStatusIRQ)  && (micros() - NMIassertMicros > NMITimeoutnS))
   {
     Serial.println("Rx NMI Timeout!");
     SwiftRegStatus &= ~(SwiftStatusRxFull | SwiftStatusIRQ); //no longer full, ready to receive more
     SetNMIDeassert;
     return false;
   }
   return true;
}

void SendRxByte(uint8_t ToSend) 
{
   //send character if non-zero, otherwise skip it to save a full c64 char cycle
   //assumes ReadyToSendRx() is true before calling
   if(ToSend)
   {  
      SwiftRxBuf = ToSend;
      SwiftRegStatus |= SwiftStatusRxFull | SwiftStatusIRQ;
      SetNMIAssert;
      NMIassertMicros = micros();
   }
}

void SendPETSCIICharImmediate(char CharToSend)
{
   //wait for c64 to be ready or NMI timeout
   while(!ReadyToSendRx()) if(!CheckRxNMITimeout()) return;

   if (BrowserMode) PageCharsReceived++;
   
   SendRxByte(CharToSend);
}

void SendASCIIStrImmediate(const char* CharsToSend)
{
   for(uint16_t CharNum = 0; CharNum < strlen(CharsToSend); CharNum++)
      SendPETSCIICharImmediate(ToPETSCII(CharsToSend[CharNum]));
}

void ParseHTMLTag()
{ //retrieve and interpret HTML Tag
  //https://www.w3schools.com/tags/
  
   char TagBuf[MaxTagSize];
   uint16_t BufCnt = 0;
   
   //pull tag from queue until >, queue empty, or buff max size
   while (RxQueueUsed > 0)
   {
      TagBuf[BufCnt] = PullFromRxQueue();
      if(TagBuf[BufCnt] == '>') break;
      if(++BufCnt == MaxTagSize-1) break;
   }
   TagBuf[BufCnt] = 0;  //terminate it

   //check for known tags and do formatting, etc
   if(strcmp(TagBuf, "br")==0 || strcmp(TagBuf, "p")==0 || strcmp(TagBuf, "/p")==0) 
   {
      SendPETSCIICharImmediate(PETSCIIreturn);
      PageCharsReceived += 40-(PageCharsReceived % 40);
   }
   else if(strcmp(TagBuf, "/b")==0) SendPETSCIICharImmediate(PETSCIIwhite); //unbold
   else if(strcmp(TagBuf, "b")==0) //bold, but don't change hyperlink color
   {
      if(!PrintingHyperlink) SendPETSCIICharImmediate(PETSCIIyellow);
   } 
   else if(strcmp(TagBuf, "eoftag")==0) AddBrowserCommandsToRxQueue();  // special tag to signal complete
   else if(strcmp(TagBuf, "li")==0) //list item
   {
      SendPETSCIICharImmediate(PETSCIIdarkGrey); 
      SendASCIIStrImmediate("\r * ");
      SendPETSCIICharImmediate(PETSCIIwhite); 
      PageCharsReceived += 40-(PageCharsReceived % 40)+3;
   }
   else if(strncmp(TagBuf, "a href=", 7)==0) 
   { //start of hyperlink text, save hyperlink
      //Printf_dbg("LinkTag: %s\n", TagBuf);
      SendPETSCIICharImmediate(PETSCIIpurple); 
      SendPETSCIICharImmediate(PETSCIIrvsOn); 
      if (UsedPageLinkBuffs < NumPageLinkBuffs)
      {
         for(uint16_t CharNum = 8; CharNum < strlen(TagBuf); CharNum++) //skip a href="
         { //terminate at first 
            if(TagBuf[CharNum]==' '  || 
               TagBuf[CharNum]=='\'' ||
               TagBuf[CharNum]=='\"' ||
               TagBuf[CharNum]=='#') TagBuf[CharNum] = 0; //terminate at space, #, ', or "
         }
         strcpy(PageLinkBuff[UsedPageLinkBuffs], TagBuf+8); // remove quote from beginning
         
         Printf_dbg("Link #%d: %s\n", UsedPageLinkBuffs+1, PageLinkBuff[UsedPageLinkBuffs]);
         UsedPageLinkBuffs++;
         
         if (UsedPageLinkBuffs > 9) SendPETSCIICharImmediate('0' + UsedPageLinkBuffs/10);
         SendPETSCIICharImmediate('0' + (UsedPageLinkBuffs%10));
      }
      else SendPETSCIICharImmediate('*');
      
      SendPETSCIICharImmediate(PETSCIIlightBlue); 
      SendPETSCIICharImmediate(PETSCIIrvsOff);
      PageCharsReceived++;
      PrintingHyperlink = true;
   }
   else if(strcmp(TagBuf, "/a")==0)
   { //end of hyperlink text
      SendPETSCIICharImmediate(PETSCIIwhite); 
      PrintingHyperlink = false;
   }
   else if(strcmp(TagBuf, "html")==0)
   { //Start of HTML
      SendPETSCIICharImmediate(PETSCIIclearScreen); // comment these two lines out to 
      UsedPageLinkBuffs = 0;                        //  scroll header instead of clear
      PageCharsReceived = 0;
      PagePaused = false;
   }
   //else Printf_dbg("Unk Tag: <%s>\n", TagBuf);  //There can be a lot of these...
   
} 

void CheckSendRxQueue()
{  
   //  if queued Rx data available to send to C64, and C64 is ready, then read/send 1 character to C64...
   if (RxQueueUsed > 0 && ReadyToSendRx())
   {
      uint8_t ToSend = PullFromRxQueue();
      //Printf_dbg("RxBuf=%02x: %c\n", ToSend, ToSend); //not recommended
      
      if (BrowserMode)
      {  //browser data is stored in ASCII to preserve tag info, convert rest to PETSCII before sending
         if(ToSend == '<') 
         {
            ParseHTMLTag();
            ToSend = 0;
         }
         else 
         {
            if(ToSend == 13) ToSend = 0; //ignore return chars
            else
            {
               ToSend = ToPETSCII(ToSend);
               if (ToSend) PageCharsReceived++; //normal char
            }
         }
      } //BrowserMode
      
      SendRxByte(ToSend);
   }
   
   CheckRxNMITimeout();
}

void FlushRxQueue()
{
   while (RxQueueUsed) CheckSendRxQueue();  
}

void AddRawCharToRxQueue(uint8_t c)
{
  if (RxQueueUsed >= RxQueueSize-1)
  {
     Printf_dbg("RxOvf! ");
     //RxQueueHead = RxQueueTail = 0;
     ////just in case...
     //SwiftRegStatus &= ~(SwiftStatusRxFull | SwiftStatusIRQ); //no longer full, ready to receive more
     //SetNMIDeassert;
     return;
  }
  RxQueue[RxQueueHead++] = c; 
  if (RxQueueHead == RxQueueSize) RxQueueHead = 0;
}

void AddRawStrToRxQueue(const char* s)
{
   uint8_t CharNum = 0;
   
   while(s[CharNum] != 0) AddRawCharToRxQueue(s[CharNum++]);
}

void AddASCIIStrToRxQueue(const char* s)
{
   uint8_t CharNum = 0;
   
   //Printf_dbg("AStrToRx(Len=%d): %s\n", strlen(s), s);
   while(s[CharNum] != 0) AddRawCharToRxQueue(ToPETSCII(s[CharNum++]));
}

void AddASCIIStrToRxQueueLN(const char* s)
{
   AddASCIIStrToRxQueue(s);
   AddASCIIStrToRxQueue("\r");
}

FLASHMEM void AddIPaddrToRxQueueLN(IPAddress ip)
{
   char Buf[50];
   sprintf(Buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
   AddASCIIStrToRxQueueLN(Buf);
}

FLASHMEM void AddMACToRxQueueLN(uint8_t* mac)
{
   char Buf[50];
   sprintf(Buf, " MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
   AddASCIIStrToRxQueueLN(Buf);
}

FLASHMEM void AddInvalidFormatToRxQueueLN()
{
   AddASCIIStrToRxQueueLN("Invalid Format");
}

FLASHMEM void AddBrowserCommandsToRxQueue()
{
   PageCharsReceived = 0;
   PagePaused = false;

   SendPETSCIICharImmediate(PETSCIIreturn);
   SendPETSCIICharImmediate(PETSCIIpurple); 
   SendPETSCIICharImmediate(PETSCIIrvsOn); 
   SendASCIIStrImmediate("Browser Commands:\r");
   SendASCIIStrImmediate("S[Term]: Search    [Link#]: Go to link\r");
   SendASCIIStrImmediate(" U[URL]: Go to URL       X: Exit\r");
   SendASCIIStrImmediate(" Return: Continue        B: Back\r");
   SendPETSCIICharImmediate(PETSCIIlightGreen);
}

FLASHMEM void AddUpdatedToRxQueueLN()
{
   AddASCIIStrToRxQueueLN("Updated");
}

FLASHMEM void AddDHCPEnDisToRxQueueLN()
{
   AddASCIIStrToRxQueue(" DHCP: ");
   if (EEPROM.read(eepAdDHCPEnabled)) AddASCIIStrToRxQueueLN("Enabled");
   else AddASCIIStrToRxQueueLN("Disabled");
}
  
FLASHMEM void AddDHCPTimeoutToRxQueueLN()
{
   uint16_t invalU16;
   char buf[50];
   EEPROM.get(eepAdDHCPTimeout, invalU16);
   sprintf(buf, " DHCP Timeout: %dmS", invalU16);
   AddASCIIStrToRxQueueLN(buf);
}
  
FLASHMEM void AddDHCPRespTOToRxQueueLN()
{
   uint16_t invalU16;
   char buf[50];
   EEPROM.get(eepAdDHCPRespTO, invalU16);
   sprintf(buf, " DHCP Response Timeout: %dmS", invalU16);
   AddASCIIStrToRxQueueLN(buf);
} 
  
FLASHMEM void StrToIPToEE(char* Arg, uint8_t EEPaddress)
{
   uint8_t octnum =1;
   IPAddress ip;   
   
   AddASCIIStrToRxQueueLN(" IP Addr");
   ip[0]=atoi(Arg);
   while(octnum<4)
   {
      Arg=strchr(Arg, '.');
      if(Arg==NULL)
      {
         AddInvalidFormatToRxQueueLN();
         return;
      }
      ip[octnum++]=atoi(++Arg);
   }
   EEPROM.put(EEPaddress, (uint32_t)ip);
   AddUpdatedToRxQueueLN();
   AddASCIIStrToRxQueue("to ");
   AddIPaddrToRxQueueLN(ip);
}


//_____________________________________AT Commands_____________________________________________________

FLASHMEM void AT_BROWSE(char* CmdArg)
{  //ATBROWSE   Enter Browser mode
   AddBrowserCommandsToRxQueue();
   UsedPageLinkBuffs = 0;
   BrowserMode = true;
}

FLASHMEM void AT_DT(char* CmdArg)
{  //ATDT<HostName>:<Port>   Connect telnet
   uint16_t  Port = 6400; //default if not defined
   char* Delim = strstr(CmdArg, ":");


   if (Delim != NULL) //port defined, read it
   {
      Delim[0]=0; //terminate host name
      Port = atol(Delim+1);
      //if (Port==0) AddASCIIStrToRxQueueLN("invalid port #");
   }
   
   char Buf[100];
   sprintf(Buf, "Trying %s\r\non port %d...", CmdArg, Port);
   AddASCIIStrToRxQueueLN(Buf);
   FlushRxQueue();
   //Printf_dbg("Host name: %s  Port: %d\n", CmdArg, Port);
   
   if (client.connect(CmdArg, Port)) AddASCIIStrToRxQueueLN("Done");
   else AddASCIIStrToRxQueueLN("Failed!");
}

FLASHMEM void AT_C(char* CmdArg)
{  //ATC: Connect Ethernet
   AddASCIIStrToRxQueue("Connect Ethernet ");
   if (EEPROM.read(eepAdDHCPEnabled)) AddASCIIStrToRxQueue("via DHCP...");
   else AddASCIIStrToRxQueue("using Static...");
   FlushRxQueue();
   
   if (EthernetInit()==true)
   {
      AddASCIIStrToRxQueueLN("Done");
      
      byte mac[6]; 
      Ethernet.MACAddress(mac);
      AddMACToRxQueueLN(mac);
      
      uint32_t ip = Ethernet.localIP();
      AddASCIIStrToRxQueue(" Local IP: ");
      AddIPaddrToRxQueueLN(ip);

      ip = Ethernet.subnetMask();
      AddASCIIStrToRxQueue(" Subnet Mask: ");
      AddIPaddrToRxQueueLN(ip);

      ip = Ethernet.gatewayIP();
      AddASCIIStrToRxQueue(" Gateway IP: ");
      AddIPaddrToRxQueueLN(ip);
   }
   else
   {
      AddASCIIStrToRxQueueLN("Failed!");
      if (Ethernet.hardwareStatus() == EthernetNoHardware) AddASCIIStrToRxQueueLN(" HW was not found");
      else if (Ethernet.linkStatus() == LinkOFF) AddASCIIStrToRxQueueLN(" Cable is not connected");
   }
}

FLASHMEM void AT_S(char* CmdArg)
{
   uint32_t ip;
   uint8_t  mac[6];
   
   AddASCIIStrToRxQueueLN("General Settings:");

   EEPreadBuf(eepAdMyMAC, mac, 6);
   AddMACToRxQueueLN(mac);
   
   AddDHCPEnDisToRxQueueLN();
   
   AddASCIIStrToRxQueueLN("DHCP only:");    
   AddDHCPTimeoutToRxQueueLN();
   AddDHCPRespTOToRxQueueLN();
   
   AddASCIIStrToRxQueueLN("Static only:");    
   AddASCIIStrToRxQueue(" My IP: ");
   EEPROM.get(eepAdMyIP, ip);
   AddIPaddrToRxQueueLN(ip);

   AddASCIIStrToRxQueue(" DNS IP: ");
   EEPROM.get(eepAdDNSIP, ip);
   AddIPaddrToRxQueueLN(ip);

   AddASCIIStrToRxQueue(" Gateway IP: ");
   EEPROM.get(eepAdGtwyIP, ip);
   AddIPaddrToRxQueueLN(ip);

   AddASCIIStrToRxQueue(" Subnet Mask: ");
   EEPROM.get(eepAdMaskIP, ip);
   AddIPaddrToRxQueueLN(ip);

}

FLASHMEM void AT_RNDMAC(char* CmdArg)
{
   uint8_t mac[6];   
   
   AddASCIIStrToRxQueueLN("Random MAC Addr");
   for(uint8_t octnum =0; octnum<6; octnum++) mac[octnum]=random(0,256);
   mac[0] &= 0xFE; //Unicast
   mac[0] |= 0x02; //Local Admin
   EEPwriteBuf(eepAdMyMAC, mac, 6);
   AddUpdatedToRxQueueLN();
   AddMACToRxQueueLN(mac);
}

FLASHMEM void AT_MAC(char* CmdArg)
{
   uint8_t octnum =1;
   uint8_t mac[6];   
   
   AddASCIIStrToRxQueueLN("MAC Addr");
   mac[0]=strtoul(CmdArg, NULL, 16);
   while(octnum<6)
   {
      CmdArg=strchr(CmdArg, ':');
      if(CmdArg==NULL)
      {
         AddInvalidFormatToRxQueueLN();
         return;
      }
      mac[octnum++]=strtoul(++CmdArg, NULL, 16);     
   }
   EEPwriteBuf(eepAdMyMAC, mac, 6);
   AddUpdatedToRxQueueLN();
   AddMACToRxQueueLN(mac);
}

FLASHMEM void AT_DHCP(char* CmdArg)
{
   if(CmdArg[1]!=0 || CmdArg[0]<'0' || CmdArg[0]>'1')
   {
      AddInvalidFormatToRxQueueLN();
      return;
   }
   EEPROM.write(eepAdDHCPEnabled, CmdArg[0]-'0');
   AddUpdatedToRxQueueLN();
   AddDHCPEnDisToRxQueueLN();
}

FLASHMEM void AT_DHCPTIME(char* CmdArg)
{
   uint16_t NewTime = atol(CmdArg);
   if(NewTime==0)
   {
      AddInvalidFormatToRxQueueLN();
      return;
   }   
   EEPROM.put(eepAdDHCPTimeout, NewTime);
   AddUpdatedToRxQueueLN();
   AddDHCPTimeoutToRxQueueLN();
}

FLASHMEM void AT_DHCPRESP(char* CmdArg)
{
   uint16_t NewTime = atol(CmdArg);
   if(NewTime==0)
   {
      AddInvalidFormatToRxQueueLN();
      return;
   }
   EEPROM.put(eepAdDHCPRespTO, NewTime);
   AddUpdatedToRxQueueLN();
   AddDHCPRespTOToRxQueueLN();
}

FLASHMEM void AT_MYIP(char* CmdArg)
{
   AddASCIIStrToRxQueue("My");
   StrToIPToEE(CmdArg, eepAdMyIP);
}

FLASHMEM void AT_DNSIP(char* CmdArg)
{
   AddASCIIStrToRxQueue("DNS");
   StrToIPToEE(CmdArg, eepAdDNSIP);
}

FLASHMEM void AT_GTWYIP(char* CmdArg)
{
   AddASCIIStrToRxQueue("Gateway");
   StrToIPToEE(CmdArg, eepAdGtwyIP);
}

FLASHMEM void AT_MASKIP(char* CmdArg)
{
   AddASCIIStrToRxQueue("Subnet Mask");
   StrToIPToEE(CmdArg, eepAdMaskIP);
}

FLASHMEM void AT_DEFAULTS(char* CmdArg)
{
   AddUpdatedToRxQueueLN();
   SetEthEEPDefaults();
   AT_S(NULL);
}

FLASHMEM void AT_HELP(char* CmdArg)
{  //                      1234567890123456789012345678901234567890
   AddASCIIStrToRxQueueLN("General AT Commands:");
   AddASCIIStrToRxQueueLN(" AT?   This help menu");
   AddASCIIStrToRxQueueLN(" AT    Ping");
   AddASCIIStrToRxQueueLN(" ATC   Connect Ethernet, display info");
   AddASCIIStrToRxQueueLN(" ATDT<HostName>:<Port>  Connect to host");

   AddASCIIStrToRxQueueLN("Modify saved parameters:");
   AddASCIIStrToRxQueueLN(" AT+S  Display stored Ethernet settings");
   AddASCIIStrToRxQueueLN(" AT+DEFAULTS  Set defaults for all ");
   AddASCIIStrToRxQueueLN(" AT+RNDMAC  MAC address to random value");
   AddASCIIStrToRxQueueLN(" AT+MAC=<XX:XX:XX:XX:XX:XX>  Set MAC");
   AddASCIIStrToRxQueueLN(" AT+DHCP=<0:1>  DHCP On(1)/Off(0)");

   AddASCIIStrToRxQueueLN("DHCP mode only: ");
   AddASCIIStrToRxQueueLN(" AT+DHCPTIME=<D>  DHCP Timeout in mS");
   AddASCIIStrToRxQueueLN(" AT+DHCPRESP=<D>  DHCP Response Timeout");

   AddASCIIStrToRxQueueLN("Static mode only: ");
   AddASCIIStrToRxQueueLN(" AT+MYIP=<D.D.D.D>   Local IP address");
   AddASCIIStrToRxQueueLN(" AT+DNSIP=<D.D.D.D>  DNS IP address");
   AddASCIIStrToRxQueueLN(" AT+GTWYIP=<D.D.D.D> Gateway IP address");
   AddASCIIStrToRxQueueLN(" AT+MASKIP=<D.D.D.D> Subnet Mask");

   AddASCIIStrToRxQueueLN("When in connected/on-line mode:");
   AddASCIIStrToRxQueueLN(" +++   Disconnect from host");
}

#define MaxATcmdLength   20

struct stcATCommand
{
  char Command[MaxATcmdLength];
  void (*Function)(char*); 
};

stcATCommand ATCommands[] =
{
   "dt"        , &AT_DT,
   "c"         , &AT_C,
   "+s"        , &AT_S,
   "+rndmac"   , &AT_RNDMAC,
   "+mac="     , &AT_MAC,
   "+dhcp="    , &AT_DHCP,
   "+dhcptime=", &AT_DHCPTIME,
   "+dhcpresp=", &AT_DHCPRESP,
   "+myip="    , &AT_MYIP,
   "+dnsip="   , &AT_DNSIP,
   "+gtwyip="  , &AT_GTWYIP,
   "+maskip="  , &AT_MASKIP,
   "+defaults" , &AT_DEFAULTS,
   "?"         , &AT_HELP,
   "browse"    , &AT_BROWSE,
};
   
void ProcessATCommand()
{

   char* CmdMsg = TxMsg; //local copy for manipulation
      
   if (strstr(CmdMsg, "at")!=CmdMsg)
   {
      AddASCIIStrToRxQueueLN("AT not found");
      return;
   }
   CmdMsg+=2; //move past the AT
   if(CmdMsg[0]==0) return;  //ping
   
   uint16_t Num = 0;
   while(Num < sizeof(ATCommands)/sizeof(ATCommands[0]))
   {
      if (strstr(CmdMsg, ATCommands[Num].Command) == CmdMsg)
      {
         CmdMsg+=strlen(ATCommands[Num].Command); //move past the Command
         while(*CmdMsg==' ') CmdMsg++;  //Allow for spaces after AT command
         ATCommands[Num].Function(CmdMsg);
         return;
      }
      Num++;
   }
   
   Printf_dbg("Unk Msg: %s CmdMsg: %s\n", TxMsg, CmdMsg);
   AddASCIIStrToRxQueue("unknown command: ");
   AddASCIIStrToRxQueueLN(TxMsg);
}

void ParseURL(const char * URL, stcURLParse &URLParse)
{
   //https://en.wikipedia.org/wiki/URL
   //https://www.w3.org/Library/src/HTParse.html
   //https://stackoverflow.com/questions/726122/best-ways-of-parsing-a-url-using-c
   //https://gist.github.com/j3j5/8336b0224167636bed462950400ff2df       Test URLs
   //the format of a URI is as follows: "ACCESS :// HOST : PORT / PATH # ANCHOR"
   
   URLParse.host[0] = 0;
   URLParse.port = 80;
   URLParse.path[0] = 0;
   
   //Find/skip access ID
   if(strstr(URL, "http://") != URL && strstr(URL, "https://") != URL) //no access ID, relative path only
   {
      strcat(URLParse.path, URL);
   }
   else
   {
      const char * ptrServerName = strstr(URL, "://")+3; //move past the access ID
      char * ptrPort = strstr(ptrServerName, ":");  //find port identifier
      char * ptrPath = strstr(ptrServerName, "/");  //find path identifier
      
      //need to check for userid? http://userid@example.com:8080/
      
      //finalize server name and update port, if present
      if (ptrPort != NULL) //there's a port ID
      {
         URLParse.port = atoi(ptrPort+1);  //skip the ":"
         strncpy(URLParse.host, ptrServerName, ptrPort-ptrServerName);
         URLParse.host[ptrPort-ptrServerName]=0; //terminate it
      }
      else if (ptrPath != NULL)  //there's a path
      {
         strncpy(URLParse.host, ptrServerName, ptrPath-ptrServerName);
         URLParse.host[ptrPath-ptrServerName]=0; //terminate it
      }
      else strcpy(URLParse.host, ptrServerName);  //no port or path
   
      //copy path, if present
      if (ptrPath != NULL) strcpy(URLParse.path, ptrPath);
      else strcpy(URLParse.path, "/");
   }

   Printf_dbg("\nOrig  = \"%s\"\n", URL);
   Printf_dbg(" serv = \"%s\"\n", URLParse.host);
   Printf_dbg(" port = %d\n", URLParse.port);
   Printf_dbg(" path = \"%s\"\n", URLParse.path);
} 

bool ReadClientLine(char* linebuf, uint16_t MaxLen)
{
   uint16_t charcount = 0;
   
   while (client.connected()) 
   {
      while (client.available()) 
      {
         uint8_t c = client.read();
         linebuf[charcount++] = c;
         if(charcount == MaxLen) return false;
         if (c=='\n')
         {
            linebuf[charcount] = 0; //terminate it
            return true;
         }
      }
   }
   return false;
}

bool WebConnect(const stcURLParse &DestURL)
{
   //   case wc_Filter:   strcpy(UpdWebPage, "/read.php?a=http://");
   
   memcpy(PrevURLQueue[PrevURLQueueNum], &DestURL, sizeof(stcURLParse)); //overwrite previous entry
   if (++PrevURLQueueNum == NumPrevURLQueues) PrevURLQueueNum = 0; //inc/wrap around top
 
   client.stop();
   RxQueueHead = RxQueueTail = 0; //dump the queue
   
   Printf_dbg("Connect: \"%s%s\"\n", DestURL.host, DestURL.path);
   
   SendASCIIStrImmediate("\rConnecting to: ");
   SendASCIIStrImmediate(DestURL.host);
   SendASCIIStrImmediate(DestURL.path);
   SendPETSCIICharImmediate(PETSCIIreturn);
   
   if (client.connect(DestURL.host, DestURL.port))
   {
      const uint16_t MaxBuf = 200;
      char inbuf[MaxBuf];
      
      client.printf("GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", 
         DestURL.path, DestURL.host);

      while(ReadClientLine(inbuf, MaxBuf))
      {
         Printf_dbg("H: %s", inbuf); 
         if (strcmp(inbuf, "\r\n") == 0) 
         {
            SendASCIIStrImmediate("Connected\r");
            return true; //blank line indicates end of header
         }
      }
      client.stop();
      SendASCIIStrImmediate("Bad Header\r");
   }

   SendASCIIStrImmediate("Connect Failed\r");
   return false;
}

void DoSearch(const char *Term)
{
   char HexChar[] = "01234567890abcdef";
   stcURLParse URL =
   {
      "www.frogfind.com", //strcpy(URL.host = "www.frogfind.com");
      80,                 //URL.port = 80;
      "/?q=",             //strcpy(URL.path = "/?q=");
   };
      
   uint16_t UWPCharNum = strlen(URL.path);
   
   //encode special chars:
   //https://www.eso.org/~ndelmott/url_encode.html
   for(uint16_t CharNum=0; CharNum <= strlen(Term); CharNum++) //include terminator
   {
      //already lower case(?)
      uint8_t NextChar = Term[CharNum];
      if((NextChar >= 'a' && NextChar <= 'z') ||
         (NextChar >= 'A' && NextChar <= 'Z') ||
         (NextChar >= '.' && NextChar <= '9') ||  //   ./0123456789
          NextChar == 0)                          //include terminator
      {      
         URL.path[UWPCharNum++] = NextChar;      
      }
      else
      {
         //encode character (%xx hex val)
         URL.path[UWPCharNum++] = '%';
         URL.path[UWPCharNum++] = HexChar[NextChar >> 4];
         URL.path[UWPCharNum++] = HexChar[NextChar & 0x0f];
      }
   }
   
   WebConnect(URL);
}

void DownloadFile()
{  //assumes client connected and ready for download
   char FileName[] = "download1.prg";
   if (!client.connected())    
   {
      SendPETSCIICharImmediate(PETSCIIpink);
      SendASCIIStrImmediate("No data\r");  
      return;      
   }

   if (!SD.begin(BUILTIN_SDCARD))  // refresh, takes 3 seconds for fail/unpopulated, 20-200mS populated
   {
      SendPETSCIICharImmediate(PETSCIIpink);
      SendASCIIStrImmediate("No SD card\r");  
      return;      
   }
   
   //if (sourceFS->exists(FileNamePath))
   if (SD.exists(FileName))
   {
      SendPETSCIICharImmediate(PETSCIIpink);
      SendASCIIStrImmediate("File already exists\r");  
      return;      
   }
   
   File dataFile = SD.open(FileName, FILE_WRITE);
   if (!dataFile) 
   {
      SendPETSCIICharImmediate(PETSCIIpink);
      SendASCIIStrImmediate("Error opening file\r");
      return;
   }   
   
   SendASCIIStrImmediate("Downloading: ");
   SendASCIIStrImmediate(FileName);
   SendPETSCIICharImmediate(PETSCIIreturn);

   uint32_t BytesRead = 0;
   while (client.connected()) 
   {
      while (client.available()) 
      {
         dataFile.write(client.read());
         BytesRead++;
      }
   }      
   dataFile.close();
   char buf[100];
   sprintf(buf, "\rFinished: %lu bytes", BytesRead);
   SendASCIIStrImmediate(buf);
}

void ProcessBrowserCommand()
{
   char* CmdMsg = TxMsg; //local copy for manipulation
   
   if(strcmp(CmdMsg, "x") ==0) //Exit browse mode
   {
      client.stop();
      BrowserMode = false;
      RxQueueHead = RxQueueTail = 0; //dump the queue
      AddASCIIStrToRxQueueLN("\rBrowser mode exit");
   }
   
   else if(strcmp(CmdMsg, "b") ==0) // Back/previous web page
   {
      if (PrevURLQueueNum<2) PrevURLQueueNum += NumPrevURLQueues-2; //wrap around bottom
      else PrevURLQueueNum -= 2;
      
      Printf_dbg("PrevURL# %d\n", PrevURLQueueNum);
      WebConnect(*PrevURLQueue[PrevURLQueueNum]);
   }
   
   else if(*CmdMsg >= '0' && *CmdMsg <= '9') //Hyperlink #
   {
      uint8_t CmdMsgVal = atoi(CmdMsg);
      
      if (CmdMsgVal > 0 && CmdMsgVal <= UsedPageLinkBuffs)
      {
         //we have a valid link # to follow...
         stcURLParse URL;
         
         ParseURL(PageLinkBuff[CmdMsgVal-1], URL); //zero based
         while (*CmdMsg >='0' && *CmdMsg <='9') CmdMsg++;  //move pointer past numbers
         
         if(URL.host[0] == 0) //relative path, use same server/port, append path
         {
            uint8_t CurQueuNum;
            if (PrevURLQueueNum == 0) CurQueuNum = NumPrevURLQueues - 1;
            else  CurQueuNum = PrevURLQueueNum - 1;
            
            if(URL.path[0] != '/') //if not root ref, add previous path to beginning
            {  
               char temp[MaxURLPathSize];
               strcpy(temp, URL.path); 
               strcpy(URL.path, PrevURLQueue[CurQueuNum]->path);
               strcat(URL.path, temp);
            }
            URL.port = PrevURLQueue[CurQueuNum]->port;
            strcpy(URL.host, PrevURLQueue[CurQueuNum]->host);
         }
         WebConnect(URL);
         
         if (*CmdMsg == 'd') 
         {
            DownloadFile();   
            client.stop();  //in case of unfinished/error, don't read it in as text
         }            
      }
   }
   
   else if(*CmdMsg == 'u') //URL
   {
      CmdMsg++; //past the 'u'
      while(*CmdMsg==' ') CmdMsg++;  //Allow for spaces after command
      
      stcURLParse URL;
      char httpServer[MaxTagSize] = "http://";
      strcat(httpServer, CmdMsg);
      ParseURL(httpServer, URL);
      WebConnect(URL);
   }
   
   else if(*CmdMsg == 's') //search
   {
      CmdMsg++; //past the 's'
      while(*CmdMsg==' ') CmdMsg++;  //Allow for spaces after command   
      DoSearch(CmdMsg);  //includes WebConnect
   }
   
   else if(*CmdMsg != 0) //unrecognized command
   {
      SendPETSCIICharImmediate(PETSCIIpink);
      SendASCIIStrImmediate("Unknown Command\r");
   }
   
   else if(PagePaused) //empty command, and paused
   { 
      SendPETSCIICharImmediate(PETSCIIcursorUp); //Cursor up to overwrite prompt & scroll on
      //SendPETSCIICharImmediate(PETSCIIclearScreen); //clear screen for next page
   }
   
   SendPETSCIICharImmediate(PETSCIIwhite); 
   PageCharsReceived = 0; //un-pause on any command, or just return key
   PagePaused = false;
   UsedPageLinkBuffs = 0;
}

//_____________________________________Handlers_____________________________________________________

FLASHMEM void InitHndlr_SwiftLink()
{
   EthernetInit();
   SwiftRegStatus = SwiftStatusTxEmpty; //default reset state
   SwiftRegCommand = SwiftCmndDefault;
   SwiftRegControl = 0;
   CycleCountdown=0;
   PlusCount=0;
   PageCharsReceived = 0;
   PrevURLQueueNum = 0;
   NMIassertMicros = 0;
   PlusCount=0;
   ConnectedToHost = false;
   BrowserMode = false;
   PagePaused = false;
   PrintingHyperlink = false;
   
   RxQueueHead = RxQueueTail = TxMsgOffset =0;
   RxQueue = (uint8_t*)malloc(RxQueueSize);
   TxMsg = (char*)malloc(TxMsgMaxSize);
   for(uint8_t cnt=0; cnt<NumPageLinkBuffs; cnt++) PageLinkBuff[cnt] = (char*)malloc(MaxTagSize);
   for(uint8_t cnt=0; cnt<NumPrevURLQueues; cnt++) 
   {
      PrevURLQueue[cnt] = (stcURLParse*)malloc(sizeof(stcURLParse));
      strcpy(PrevURLQueue[cnt]->path, "/teensyrom/");
      strcpy(PrevURLQueue[cnt]->host, "sensoriumembedded.com");
      PrevURLQueue[cnt]->port = 80;
   }
   randomSeed(ARM_DWT_CYCCNT);
}   

void IO1Hndlr_SwiftLink(uint8_t Address, bool R_Wn)
{
   uint8_t Data;
   
   if (R_Wn) //IO1 Read  -------------------------------------------------
   {
      switch(Address)
      {
         case IORegSwiftData:   
            DataPortWriteWaitLog(SwiftRxBuf);
            CycleCountdown = C64CycBetweenRx;
            SetNMIDeassert;
            SwiftRegStatus &= ~(SwiftStatusRxFull | SwiftStatusIRQ); //no longer full, ready to receive more
            break;
         case IORegSwiftStatus:  
            DataPortWriteWaitLog(SwiftRegStatus);
            break;
         case IORegSwiftCommand:  
            DataPortWriteWaitLog(SwiftRegCommand);
            break;
         case IORegSwiftControl:
            DataPortWriteWaitLog(SwiftRegControl);
            break;
      }
   }
   else  // IO1 write    -------------------------------------------------
   {
      Data = DataPortWaitRead();
      switch(Address)
      {
         case IORegSwiftData:  
            //add to input buffer
            SwiftTxBuf=Data;
            SwiftRegStatus &= ~SwiftStatusTxEmpty; //Flag full until Tx processed
            break;
         case IORegSwiftStatus:  
            //Write to status reg is a programmed reset
            SwiftRegCommand = SwiftCmndDefault;
            break;
         case IORegSwiftControl:
            SwiftRegControl = Data;
            //Could confirm setting 8N1 & acceptable baud?
            break;
         case IORegSwiftCommand:  
            SwiftRegCommand = Data;
            //check for Tx/Rx IRQs enabled?
            //handshake line updates?
            break;
      }
      TraceLogAddValidData(Data);
   }
}

void PollingHndlr_SwiftLink()
{
   //detect connection change
   if (ConnectedToHost != client.connected())
   {
      ConnectedToHost = client.connected();
      if (BrowserMode)
      {
         if (!ConnectedToHost) AddRawStrToRxQueue("<br>*End of Page*<eoftag>");  //add special tag to catch when complete
      }
      else
      {
         AddASCIIStrToRxQueue("\r\r\r*** ");
         if (ConnectedToHost) AddASCIIStrToRxQueueLN("connected to host");
         else AddASCIIStrToRxQueueLN("not connected");
      }
   }
   
   //if client data available, add to Rx Queue
   #ifdef DbgMsgs_IO
      if(client.available())
      {
         uint16_t Cnt = 0;
         //Serial.printf("RxIn %d+", RxQueueUsed);
         while (client.available())
         {
            AddRawCharToRxQueue(client.read());
            Cnt++;
         }
         //Serial.printf("%d=%d\n", Cnt, RxQueueUsed);
         if (RxQueueUsed>3000) Serial.printf("Lrg RxQueue add: %d  total: %d\n", Cnt, RxQueueUsed);
      }
   #else
      while (client.available()) AddRawCharToRxQueue(client.read());
   #endif
   
   //if Tx data available, get it from C64
   if ((SwiftRegStatus & SwiftStatusTxEmpty) == 0) 
   {
      if (client.connected() && !BrowserMode) //send Tx data to host
      {
         //Printf_dbg("send %02x: %c\n", SwiftTxBuf, SwiftTxBuf);
         client.print((char)SwiftTxBuf);  //send it
         if(SwiftTxBuf=='+')
         {
            if(millis()-LastTxMillis>1000 || PlusCount!=0) //Must be preceded by at least 1 second of no characters
            {   
               if(++PlusCount>3) PlusCount=0;
            }
         }
         else PlusCount=0;
         
         SwiftRegStatus |= SwiftStatusTxEmpty; //Ready for more
      }
      else  //off-line/at commands or BrowserMode..................................
      {         
         Printf_dbg("echo %02x: %c -> ", SwiftTxBuf, SwiftTxBuf);
         
         if(BrowserMode) SendPETSCIICharImmediate(SwiftTxBuf); //echo it now, buffer may be paused or filling
         else AddRawCharToRxQueue(SwiftTxBuf); //echo it at end of buffer
         
         SwiftTxBuf &= 0x7f; //bit 7 is Cap in Graphics mode
         if (SwiftTxBuf & 0x40) SwiftTxBuf |= 0x20;  //conv to lower case PETSCII
         Printf_dbg("%02x: %c\n", SwiftTxBuf);
         
         if (TxMsgOffset && (SwiftTxBuf==0x08 || SwiftTxBuf==0x14)) TxMsgOffset--; //Backspace in ascii  or  Delete in PETSCII
         else TxMsg[TxMsgOffset++] = SwiftTxBuf; //otherwise store it
         
         if (SwiftTxBuf == 13 || TxMsgOffset == TxMsgMaxSize) //return hit or max size
         {
            SwiftRegStatus |= SwiftStatusTxEmpty; //clear the flag after last SwiftTxBuf access
            TxMsg[TxMsgOffset-1] = 0; //terminate it
            Printf_dbg("TxMsg: %s\n", TxMsg);
            if(BrowserMode) ProcessBrowserCommand();
            else
            {
               ProcessATCommand();
               if (!BrowserMode) AddASCIIStrToRxQueueLN("ok\r");
            }
            TxMsgOffset = 0;
         }
         else SwiftRegStatus |= SwiftStatusTxEmpty; //clear the flag after last SwiftTxBuf access
      }
      LastTxMillis = millis();
   }
   
   if(PlusCount==3 && millis()-LastTxMillis>1000) //Must be followed by one second of no characters
   {
      PlusCount=0;
      client.stop();
      AddASCIIStrToRxQueueLN("\r*click*");
   }

   if (PageCharsReceived < 880 || PrintingHyperlink) CheckSendRxQueue();
   else
   {
      if (!PagePaused)
      {
         PagePaused = true;
         SendPETSCIICharImmediate(PETSCIIrvsOn);
         SendPETSCIICharImmediate(PETSCIIpurple);
         SendASCIIStrImmediate("\rPause (#,S[],U[],X,B,Ret)");
         SendPETSCIICharImmediate(PETSCIIrvsOff);
         SendPETSCIICharImmediate(PETSCIIlightGreen);
      }
   }
}

void CycleHndlr_SwiftLink()
{
   if (CycleCountdown) CycleCountdown--;
}


