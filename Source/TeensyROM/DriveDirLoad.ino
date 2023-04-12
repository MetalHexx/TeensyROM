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


//these functions triggered from ISR and use current menu selection information while c64 code waits

void HandleExecution()
{
   StructMenuItem MenuSel = MenuSource[IO1[rwRegSelItem]]; //local copy of selected menu item
   
   if (MenuSel.ItemType == rtNone || MenuSel.ItemType == rtUnk) return;  //no action taken for these types
   
   //if SD card or USB Drive,  update path & load dir   or   load file to RAM
   if (IO1[rWRegCurrMenuWAIT] == rmtSD || IO1[rWRegCurrMenuWAIT] == rmtUSBDrive) 
   {
      bool SD_nUSBDrive = (IO1[rWRegCurrMenuWAIT] == rmtSD);
      
      if (MenuSel.ItemType == rtDir)
      {  //edit path as needed and load the new directory from SD/USB
         char * myPath;
         if (SD_nUSBDrive) myPath = SDPath;
         else myPath = USBDrivePath;
         
         if(strcmp(MenuSel.Name, UpDirString)==0)
         {  //up dir
            char * LastSlash = strrchr(myPath, '/'); //find last slash
            if (LastSlash != NULL) LastSlash[0] = 0;  //terminate it there 
         }
         else strcat(myPath, MenuSel.Name); //append selected dir name
         
         LoadDirectory(SD_nUSBDrive); 
         return;  //we're done here...
      }
      
      if(!LoadFile(&MenuSel, SD_nUSBDrive)) MenuSel.ItemType=rtUnk; //mark unknown if error      

      MenuSel.Code_Image = RAM_Image;
      if (MenuSel.ItemType == rtCrt) ParseCRTFile(&MenuSel); //will convert type, if checks ok
   }
    
   if (IO1[rWRegCurrMenuWAIT] == rmtUSBHost)
   {
      MenuSel.Code_Image = HOST_Image; 
      if (MenuSel.ItemType == rtCrt) ParseCRTFile(&MenuSel);
   }
   
   //has to be distilled down to one of these by this point, only ones supported so far.
   //Emulate ROM or prep PRG tranfer
   switch(MenuSel.ItemType)
   {
      case rt16k:
         SetGameAssert;
         SetExROMAssert;
         LOROM_Image = MenuSel.Code_Image;
         HIROM_Image = MenuSel.Code_Image+0x2000;
         doReset=true;
         break;
      case rt8kHi:
         SetGameAssert;
         SetExROMDeassert;
         LOROM_Image = NULL;
         HIROM_Image = MenuSel.Code_Image;
         doReset=true;
         break;
      case rt8kLo:
         SetGameDeassert;
         SetExROMAssert;
         LOROM_Image = MenuSel.Code_Image;
         HIROM_Image = NULL;
         doReset=true;
         break;
      case rtPrg:
         //set up for transfer
         MenuSource[IO1[rwRegSelItem]].Code_Image = MenuSel.Code_Image; 
         MenuSource[IO1[rwRegSelItem]].Size = MenuSel.Size; //only copy the pointer & Size back, not type
         IO1[rRegStrAddrLo]=MenuSel.Code_Image[0];
         IO1[rRegStrAddrHi]=MenuSel.Code_Image[1];
         IO1[rRegStrAvailable]=0xff;
         StreamOffsetAddr = 2; //set to start of data
         break;
   }
}

void MenuChange()
{
   switch(IO1[rWRegCurrMenuWAIT])
   {
      case rmtTeensy:
         MenuSource = ROMMenu; 
         IO1[rRegNumItems] = sizeof(ROMMenu)/sizeof(ROMMenu[0]);
         break;
      case rmtSD:
         stpcpy(SDPath, "/");
         LoadDirectory(true);
         MenuSource = SDMenu; 
         break;
      case rmtUSBDrive:
         stpcpy(USBDrivePath, "/");
         LoadDirectory(false);
         MenuSource = USBDriveMenu; 
         break;
      case rmtUSBHost:
         MenuSource = &USBHostMenu; 
         IO1[rRegNumItems] = NumUSBHostItems;
         break;
   }
}

bool LoadFile(StructMenuItem* MyMenuItem, bool SD_nUSBDrive) 
{
   char *myPath;
   if (SD_nUSBDrive) myPath= SDPath;
   else myPath = USBDrivePath;
   
   char FullFilePath[300];

   if (strlen(myPath) == 1 && myPath[0] == '/') sprintf(FullFilePath, "%s%s", myPath, MenuSource[IO1[rwRegSelItem]].Name);  // at root
   else sprintf(FullFilePath, "%s/%s", myPath, MenuSource[IO1[rwRegSelItem]].Name);
      
   Serial.printf("Openning: %s\n", FullFilePath);
   
   File myFile;
   if (SD_nUSBDrive) myFile= SD.open(FullFilePath, FILE_READ);
   else myFile= firstPartition.open(FullFilePath, FILE_READ);
   
   if (!myFile) return false;

   free(RAM_Image);
   RAM_Image = (uint8_t*)malloc(myFile.size());

   uint16_t count=0;
   while (myFile.available() && count < myFile.size()) RAM_Image[count++]=myFile.read();

   if (count != myFile.size()) return false;
   MyMenuItem->Size = count;
   myFile.close();
   return true;
}

void LoadDirectory(bool SD_nUSBDrive) 
{
   uint8_t NumItems = 0;
   File dir;
   char *DrvPath;
   StructMenuItem* DrvMenuItem;
 
   if (SD_nUSBDrive)
   {  //SD card
      dir         = SD.open(SDPath);
      DrvPath     = SDPath;
      DrvMenuItem = SDMenu;
   }
   else 
   {  //USB Drive
      dir         = firstPartition.open(USBDrivePath);
      DrvPath     = USBDrivePath;
      DrvMenuItem = USBDriveMenu;
   }
   
   if (!(strlen(DrvPath) == 1 && DrvPath[0] == '/'))
   {  // *not* at root, add up dir option
      NumItems = 1;
      strcpy(DrvMenuItem[0].Name, UpDirString);
      DrvMenuItem[0].ItemType = rtDir;
   }
   
   while (File entry = dir.openNextFile()) 
   {
      if (entry.isDirectory())
      {
         DrvMenuItem[NumItems].Name[0] = '/';
         memcpy(DrvMenuItem[NumItems].Name+1, entry.name(), MaxItemNameLength-1);
      }
      else memcpy(DrvMenuItem[NumItems].Name, entry.name(), MaxItemNameLength);
      
      DrvMenuItem[NumItems].Name[MaxItemNameLength-1]=0; //terminate in case too long. 
      
      if (entry.isDirectory()) DrvMenuItem[NumItems].ItemType = rtDir;
      else 
      {
         char* Extension = (DrvMenuItem[NumItems].Name + strlen(DrvMenuItem[NumItems].Name) - 4);
         for(uint8_t cnt=1; cnt<=3; cnt++) if(Extension[cnt]>='A' && Extension[cnt]<='Z') Extension[cnt]+=32;
         
         if (strcmp(Extension, ".prg")==0) DrvMenuItem[NumItems].ItemType = rtPrg;
         else if (strcmp(Extension, ".crt")==0) DrvMenuItem[NumItems].ItemType = rtCrt;
         else DrvMenuItem[NumItems].ItemType = rtUnk;
      }
      
      //Serial.printf("%d- %s\n", NumItems, DrvMenuItem[NumItems].Name); 
      entry.close();
      if (NumItems++ == MaxMenuItems)
      {
         Serial.println("Too many files!");
         break;
      }
   }
   
   IO1[rRegNumItems] = NumItems;
  
}

void ParseCRTFile(StructMenuItem* MyMenuItem)   
{
   //https://vice-emu.sourceforge.io/vice_17.html#SEC369
   //http://ist.uwaterloo.ca/~schepers/formats/CRT.TXT
   
   uint8_t* CRT_Image = MyMenuItem->Code_Image;
   MyMenuItem->ItemType = rtUnk; //in case we fail
   
   if (memcmp(CRT_Image, "C64 CARTRIDGE", 13)!=0)
   {
      Serial.println("\"C64 CARTRIDGE\" not found");
      return;
   }
   
   uint32_t HeaderLen = toU32(CRT_Image+0x10);
   Serial.printf("Header len: %lu\n", HeaderLen);
   if (HeaderLen < 0x40) HeaderLen = 0x40;
   
   Serial.printf("HW Type: %d\n", toU16(CRT_Image+0x16));
   if (toU16(CRT_Image+0x16) !=0)
   {
      Serial.println("Only \"Normal\" carts *currently* supported");
      return;
   }
   
   uint8_t EXROM = CRT_Image[0x18];
   uint8_t GAME = CRT_Image[0x19];
   Serial.printf("EXROM: %d\n", EXROM);
   Serial.printf(" GAME: %d\n", GAME);
   
   Serial.printf("Name: %s\n", (CRT_Image+0x20));
   
   uint8_t *ChipImage = CRT_Image+HeaderLen;
   //On to CHIP packet(s)...
   if (memcmp(ChipImage, "CHIP", 4)!=0)
   {
      Serial.println("\"CHIP\" not found");
      return;
   }
  
   Serial.printf("Packet len: $%08x\n",  toU32(ChipImage+0x04));
   Serial.printf("Chip Type: %d\n",      toU16(ChipImage+0x08));
   Serial.printf(" Bank Num: %d\n",      toU16(ChipImage+0x0A));
   uint16_t LoadAddress = toU16(ChipImage+0x0C);
   uint16_t ROMSize = toU16(ChipImage+0x0E);
   Serial.printf("Load Addr: $%04x\n",   LoadAddress);
   Serial.printf(" ROM Size: $%04x\n",   ROMSize);
      
   //We have a good CRT image!
   //Is it a config we support?
   MyMenuItem->Code_Image += HeaderLen+0x10;
   
   if(EXROM==0 &&            LoadAddress == 0x8000 && ROMSize == 0x2000) //GAME is usually==1, Centiped calls for low but doesn't use it
   {
      MyMenuItem->ItemType = rt8kLo;
      Serial.println("\n 8kLo config");
      return;
   }      

   if(EXROM==1 && GAME==0 && LoadAddress == 0xe000 && ROMSize == 0x2000)
   {
      MyMenuItem->ItemType = rt8kHi;
      Serial.println("\n 8kHi config");
      return;
   }      

   if(EXROM==0 && GAME==0 && LoadAddress == 0x8000 && ROMSize == 0x4000)
   {
      MyMenuItem->ItemType = rt16k;
      Serial.println("\n 16k config");
      return;
   }      
   
   Serial.println("\nHW config unknown!");
}

