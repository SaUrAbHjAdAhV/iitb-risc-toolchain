; Simple Integration Loop Test
        LLI  R1, 5       ; R1 = 5
        LLI  R2, 0       ; R2 = 0
        LLI  R3, 0
loop:
        ADA  R2, R2, R1  ; R2 = R2 + R1
        ADI  R1, R1, -1  ; R1 = R1 - 1
        BEQ  R1, R3, done ; If R1 == 0 jump to done
        BEQ  R3, R3, loop ; Unconditional jump to loop
done:
        LLI  R4, 100     ; R4 = 100 (Safe, dedicated memory base address) 
        SW   R2, R4, 0   ; Store R2 (15) at memory address 100 
        LW   R5, R4, 0   ; Load it back into R5 to verify 