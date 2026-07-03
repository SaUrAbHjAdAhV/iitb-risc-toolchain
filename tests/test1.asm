; Simple Integration Loop Test
        LLI  R1, 5       ; R1 = 5
        LLI  R2, 0       ; R2 = 0
loop:
        ADA  R2, R2, R1  ; R2 = R2 + R1
        ADI  R1, R1, -1  ; R1 = R1 - 1
        BEQ  R1, R0, done ; If R1 == 0 jump to done
        BEQ  R0, R0, loop ; Unconditional jump to loop
done:
        SW   R2, R0, 0   ; Store calculated result