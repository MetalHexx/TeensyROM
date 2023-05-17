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


FASTRUN void isrButton()
{
   BtnPressed = true;
}


FASTRUN void isrPHI2() //Phi2 rising edge
{
   StartCycCnt = ARM_DWT_CYCCNT;
   uint32_t CycSinceLast = StartCycCnt-LastCycCnt;
   LastCycCnt = StartCycCnt;
   
   if (CycSinceLast > nSToCyc(nS_MaxAdjThresh)) // If we're late, adjust...
   {
      StartCycCnt += nSToCyc(nS_MaxAdjThresh) - CycSinceLast; 
      #ifdef DbgCycAdjLog      
         if (BigBuf != NULL)
         {
            BigBuf[BigBufCount] = CycSinceLast | AdjustedCycleTiming;
            if (BigBufCount < BigBufSize) BigBufCount++;
         }
      #endif
   }
   
   if (Phi2ISRState != P2I_Normal) 
   {
      if (Phi2ISRState == P2I_TimingCheck)
      {
         BigBuf[BigBufCount++] = CycSinceLast;
         if (BigBufCount == BigBufSize) Phi2ISRState = P2I_Normal;
      }
      return;
   }
   SetDebugAssert;

   WaitUntil_nS(nS_RWnReady); 
   register uint32_t GPIO_6 = ReadGPIO6; //Address bus and (almost) R/*W are valid on Phi2 rising, Read now
   register uint16_t Address = GP6_Address(GPIO_6); //parse out address
   
   WaitUntil_nS(nS_PLAprop); 
   register uint32_t GPIO_9 = ReadGPIO9; //Now read the derived signals 
   
   if (!GP9_ROML(GPIO_9)) //ROML: 8000-9FFF address space, read only
   {
      if (LOROM_Image!=NULL) DataPortWriteWait(LOROM_Image[Address & 0x1FFF]);  
   }  //ROML
   else if (!GP9_ROMH(GPIO_9)) //ROMH: A000-BFFF or E000-FFFF address space, read only
   {
      if (HIROM_Image!=NULL) DataPortWriteWait(HIROM_Image[Address & 0x1FFF]); 
   }  //ROMH
   else if (!GP9_IO1n(GPIO_9)) //IO1: DExx address space
   {
      Address &= 0xFF;
      #ifdef DbgIOTraceLog
         BigBuf[BigBufCount] = Address; //initialize w/ address
      #endif
      switch(IO1Handler)
      {
         case IO1H_TeensyROM:
            IO1Hndlr_TeensyROM(Address, GP6_R_Wn(GPIO_6));
            break;
         case IO1H_MIDI_Datel:
         //case IO1H_MIDI_Sequential:  //changed to Datel after init (reg addrs set)
         //case IO1H_MIDI_Passport:    //changed to Datel after init (reg addrs set)
         //case IO1H_MIDI_NamesoftIRQ: //changed to Datel after init (reg addrs set)
            IO1Hndlr_MIDI(Address, GP6_R_Wn(GPIO_6));
            break;
         case IO1H_SwiftLink:
            IO1Hndlr_SwiftLink(Address, GP6_R_Wn(GPIO_6));
            break;
         case IO1H_Debug:
            IO1Hndlr_Debug(Address, GP6_R_Wn(GPIO_6));
            break;
      }
      #ifdef DbgIOTraceLog
         if (GP6_R_Wn(GPIO_6)) BigBuf[BigBufCount] |= IOTLRead;
         if (BigBufCount < BigBufSize) BigBufCount++;
      #endif
   }  //IO1
   //IO2: DFxx address space
   //else if (!GP9_IO2n(GPIO_9)) Serial.printf("IO2 %s %d\n", GP6_R_Wn(GPIO_6) ? "Rd from" : "Wr to", Address);

if (EmulateVicCycles)
{
   while(GP6_Phi2(ReadGPIO6)); //Re-align to phi2 falling   
   //phi2 has gone low..........................................................................
   
   StartCycCnt = ARM_DWT_CYCCNT;

   WaitUntil_nS(nS_VICStart);
   
   GPIO_6 = ReadGPIO6; //Address bus and R/*W 
   Address = GP6_Address(GPIO_6); //parse out address
   GPIO_9 = ReadGPIO9; //Now read the derived signals

   if (!GP9_ROMH(GPIO_9)) //ROMH: A000-BFFF or E000-FFFF address space, read only
   {
      DataPortWriteWait(HIROM_Image[(Address & 0x1FFF)]); //uses same hold time as normal cycle
   } 
}
   
   //leave time enough time to re-trigger on rising edge!
   SetDebugDeassert;    
}

