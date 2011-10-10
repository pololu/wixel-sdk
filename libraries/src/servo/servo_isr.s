    .module delay
    .optsdcc -mmcs51 --model-medium
    .area CSEG (CODE)

    _P1_4 = 0x94
    
    ; Servo ISR
    .globl _ISR_T1
_ISR_T1:
    reti
