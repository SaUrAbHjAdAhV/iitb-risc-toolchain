; ===================================================
; Test 3: Predicated execution and remaining branches
; ===================================================

        LLI  R1, 10      ; R1 = 10
        LLI  R2, 5       ; R2 = 5
        ADA  R3, R1, R2  ; R3 = 15, sets Z=0, C=0
        ADZ  R4, R1, R2  ; should NOT execute (Z=0)
        ADC  R4, R1, R2  ; should NOT execute (C=0)
        ACA  R5, R1, R2  ; R5 = R1 + ~R2 (two's complement subtract)
        BLT  R2, R1, done ; R2 < R1, so branch taken
        LLI  R6, 99      ; should be skipped
done:
        BLE  R1, R1, end  ; R1 <= R1 (equal), branch taken
        LLI  R7, 99      ; should be skipped
end:
        ADA  R0, R0, R0  ; NOP equivalent