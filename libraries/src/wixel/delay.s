    .module delay
    .optsdcc -mmcs51 --model-medium
    .area CSEG (CODE)

; void delayMicroseconds(unsigned char microseconds)
;   microseconds: number of microseconds delay; any value between 0 and 255
; 
;   This function delays for the specified number of microseconds using
;   a simple loop.  If an interrupt occurs during this function, the delay
;   will be longer than desired.
; 
; Prerequisites:  The chip must be running at 24 MHz, and flash prefetching
;   and caching must be enabled (MEMCTR = 0;).  
;
; Tests:
;    This function was tested using the following code:
;    P1_0 = 1;
;    delayMicroseconds(100);
;    P1_0 = 0;
;    delayMicroseconds(10);
;    P1_0 = 1;
;    delayMicroseconds(1);
;    P1_0 = 0;
;    The code above produced a high pulse of 100.20/100.16 microseconds,
;    followed by a low pulse of 10.20/10.16 microseconds,
;    followed by a high pulse of 1.20/1.16 microseconds.
;    (The exact length of the pulses depended on the parity of the address
;    of the lcall instruction used to call delayMicroseconds.)
;
; Implementation Details:
;    By experimenting, David determined that jumping (e.g. lcall, ret, or
;    ajmp) to an odd address takes one more instruction cycle than jumping
;    to an even address.
;
;    To get around this, the delayMicroseconds loop contains two jumps:
;    one of these jumps will be to an odd address, and one will be to an
;    even address, so the effects of the object's parity will always
;    cancel out.
;
;    The ".even" directive doesn't guarantee that the absolute address of
;    a label will be even, it seems to only effect relative positioning
;    within an object file, so we could not use the .even directive.
;
    .globl _delayMicroseconds
loopStart:
    nop
    nop
    nop
    nop
    ljmp loopJump
    nop    ; unreachable instruction
    nop    ; unreachable instruction
    nop    ; unreachable instruction
    nop    ; unreachable instruction
loopJump:  ; Guaranteed to have a different parity from loopStart.
    nop
    nop
    nop
    nop
_delayMicroseconds:
    mov a,dpl
    jz loopEnd
    djnz dpl, loopStart
loopEnd: ret
