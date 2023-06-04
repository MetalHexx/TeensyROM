; MIT License
; 
; Copyright (c) 2023 Travis Smith
; 
; Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
; and associated documentation files (the "Software"), to deal in the Software without 
; restriction, including without limitation the rights to use, copy, modify, merge, publish, 
; distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom 
; the Software is furnished to do so, subject to the following conditions:
; 
; The above copyright notice and this permission notice shall be included in all copies or 
; substantial portions of the Software.
; 
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
; BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
; DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

PrintSerialString:
   ;for TR only, IO1 space is implied.
   ;load Acc with RegIO # that will serialize output string(& write to reset pointer)
   ;   lda #rwRegNextIOHndlrName,  jsr PrintSerialString
   sta PtrAddrLo
   ldy #>IO1Port
   sty PtrAddrHi
   ldy #0
   sta (PtrAddrLo), y ;write any value to reset pointer
-  lda (PtrAddrLo), y
   beq +
   jsr SendChar
   jmp -
+  rts
 
PrintString:
   ;replaces BASIC routine jsr $ab1e when unavailable
   ;usage: lda #<MsgM2SPolyMenu,  ldy #>MsgM2SPolyMenu,  jsr PrintString 
   sta PtrAddrLo
   sty PtrAddrHi
   ldy #0
-  lda (PtrAddrLo), y
   beq +
   jsr SendChar
   iny
   bne -
+  rts

DisplayTime:
   ldx #1 ;row
   ldy #29  ;col
   clc
   jsr SetCursor
   lda #TimeColor
   jsr SendChar
   lda TODHoursBCD ;latches time in regs (stops incrementing)
   tay ;save for re-use
   and #$1f
   bne nz   ;if hours is 0, make it 12...
   tya
   ora #$12
   tay ;re-save for re-use
nz tya
   and #$10
   bne +
   lda #ChrSpace
   jmp ++
+  lda #'1'
++ jsr SendChar
   tya
   and #$0f  ;ones of hours
   jsr PrintHexNibble
   lda #':'
   jsr SendChar
   lda TODMinBCD
   jsr PrintHexByte
   lda #':'
   jsr SendChar
   lda TODSecBCD
   jsr PrintHexByte
   ;lda #'.'
   ;jsr SendChar
   lda TODTenthSecBCD ;have to read 10ths to release latch
   ;jsr PrintHexNibble
   tya ;am/pm (pre latch release)
   and #$80
   bne +
   lda #'a'
   jmp ++
+  lda #'p'
++ jsr SendChar
   lda #'m'
   jsr SendChar
   rts
   
PrintHexByte:
   ;Print byte value stored in acc in hex (2 chars)
   pha
   lsr
   lsr
   lsr
   lsr
   jsr PrintHexNibble
   pla
   ;pha   ; preserve acc on return?
   jsr PrintHexNibble
   ;pla
   rts
   
PrintHexNibble:   
   ;Print value stored in lower nible acc in hex
   ;trashes acc
   and #$0f
   cmp #$0a
   bpl l 
   clc
   adc #'0'
   jmp pr
l  clc
   adc #'a'-$0a
pr jsr SendChar
   rts

PrintOnOff:
   ;Print "On" or "Off" based on Zero flag
   ;uses A and Y regs
   bne +
   lda #<MsgOff
   ldy #>MsgOff
   jmp ++
+  lda #<MsgOn
   ldy #>MsgOn
++ jsr PrintString 
   rts

Print4CharTableHiNib
   lsr
   lsr
   lsr
   lsr ; move to lower nibble
Print4CharTable:   
;prints 4 chars from a table of continuous 4 char sets (no termination)
;X=table base lo, y=table base high, acc=index to item# (63 max)
;   and #0xfc 
   stx PtrAddrLo
   sty PtrAddrHi
   asl
   asl  ;mult by 4
   tay
-  lda (PtrAddrLo),y
   jsr SendChar   ;type (4 chars)
   iny
   tya
   and #3
   bne -
   rts
   
