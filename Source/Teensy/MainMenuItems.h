
#include "ROMs\TeensyROMC64.h"


#include "ROMs\1541_Diagnostics.h" 
#include "ROMs\SID_Tester.h" 
#include "ROMs\586220ast_Diagnostics.h" 
#include "ROMs\781220_Dead_Test.h"
#include "ROMs\game_controller_tester.prg.h"
#include "ROMs\rom_chksum_137kernals.prg.h"
#include "ROMs\C128_789010.crt.h"

#include "ROMs\Epyx_Fast_Load.crt.h"
//#include "ROMs\cynthcart_201.prg.h"
#include "ROMs\cynthcart_plus_datel.crt.h"
#include "ROMs\sta64_v2_6.prg.h"
#include "ROMs\ccgms_2021.prg.h"
#include "ROMs\80columns.prg.h"
#include "ROMs\hex_mon.prg.h"
 
#include "ROMs\Jupiter_Lander.h" 
#include "ROMs\joust.prg.h"

//Not including these to save mem space
//when compiling, make sure RAM1 free for local variables >~30k

// #include "ROMs\Joystick_Tester.h" 
// #include "ROMs\Keyboard_Tester.h" 
// #include "ROMs\Donkey_Kong.h" 
// #include "ROMs\DualCopy.prg.h"
// #include "ROMs\ember_head.prg.h"
// #include "ROMs\disp_fract.prg.h"
// #include "ROMs\draw01.prg.h"

StructMenuItem ROMMenu[] = 
{

   rtNone   , "Utilities/MIDI-------------", NULL                                 , 1, //sepparator
   rtFilePrg, " CCGMS 2021 Terminal"       , (uint8_t*)ccgms_2021_prg             , sizeof(ccgms_2021_prg) ,
   rtFileCrt, " Cynthcart 2.0.1+Datel MIDI", (uint8_t*)cynthcart_cart_Datel_crt   , 1, //size not needed for CRTs   
   //rtFilePrg, " Cynthcart v2.0.1"          , (uint8_t*)cynthcart_201_prg          , sizeof(cynthcart_201_prg) ,    
   rtFilePrg, " Station64 v2.6 MIDI/ASID"  , (uint8_t*)sta64_v2_6_prg             , sizeof(sta64_v2_6_prg) ,
   rtFileCrt, " Epyx Fast Load Cart"       , (uint8_t*)Epyx_Fast_Load_crt         , 1, //size not needed for CRTs
   rtFilePrg, " 80 Columns"                , (uint8_t*)a80columns_prg             , sizeof(a80columns_prg) ,
   rtFilePrg, " Hex Mon"                   , (uint8_t*)hex_mon_prg                , sizeof(hex_mon_prg) ,
   rtNone   , ""                           , NULL                                 , 1, //sepparator

   rtNone   , "Test/Diags-----------------", NULL                                 , 1, //sepparator
   rtBin8kHi, " 781220 C64 Dead Test"      , (uint8_t*)a781220_Dead_Test_BIN      , 1, //size not needed for ROMs
   rtBin8kLo, " 586220* C64 Diagnostics"   , (uint8_t*)a586220ast_Diagnostics_BIN , 1, //size not needed for ROMs
   rtFileCrt, " 789010 C128 Diagnostics"   , (uint8_t*)C128_789010_crt            , 1, //size not needed for CRTs
   rtBin8kLo, " 1541 Diagnostics"          , (uint8_t*)a1541_Diagnostics_BIN      , 1, //size not needed for ROMs
   rtBin8kLo, " SID Tester"                , (uint8_t*)SID_Tester_BIN             , 1, //size not needed for ROMs
   rtFilePrg, " Game Controller Tester"    , (uint8_t*)game_controller_tester_prg , sizeof(game_controller_tester_prg) ,
   rtFilePrg, " ROM Checksum read"         , (uint8_t*)rom_chksum_137kernals_prg  , sizeof(rom_chksum_137kernals_prg) ,
   //end of first page

   rtNone   , "Games:---------------------", NULL                                 , 1, //sepparator
   rtBin8kHi, " Jupiter Lander"            , (uint8_t*)Jupiter_Lander_BIN         , 1, //size not needed for ROMs  
   rtFilePrg, " Joust!"                    , (uint8_t*)joust_prg                  , sizeof(joust_prg) ,


   //rtBin16k , " Donkey Kong"               , (uint8_t*)Donkey_Kong_BIN            , 1, //size not needed for ROMs      
   //rtFilePrg, " DualCopy"                  , (uint8_t*)DualCopy_prg               , sizeof(DualCopy_prg) ,
   //rtBin8kLo, " Keyboard Tester"           , (uint8_t*)Keyboard_Tester_BIN        , 1, //size not needed for ROMs
   //rtBin8kLo, " Joystick Tester"           , (uint8_t*)Joystick_Tester_BIN        , 1, //size not needed for ROMs
   //rtFilePrg, " Ember Head"                , (uint8_t*)ember_head_prg             , sizeof(ember_head_prg) ,
   //rtFilePrg, " Display Fractal"           , (uint8_t*)disp_fract_prg             , sizeof(disp_fract_prg) ,
   //rtFilePrg, " Draw!"                     , (uint8_t*)draw01_prg                 , sizeof(draw01_prg) , //max Name length  :)
   //rtNone   , ""                           , NULL                                 , 1, //sepparator

};

