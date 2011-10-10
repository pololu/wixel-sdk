    .module delay
    .optsdcc -mmcs51 --model-medium
    .area CSEG (CODE)

    _P1_4 = 0x94

    .globl _mainLoop
_mainLoop:
    ljmp _mainLoop
    
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    ; Servo ISR
    .globl _ISR_T1
_ISR_T1:
    cpl _P1_4
    reti
