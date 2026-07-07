LLI  R3, 50       ; R3 = 50 (use as base address)
LLI  R1, 42       ; R1 = 42 (value to store)
SW   R1, R3, 0    ; MEM[50] = 42
LW   R2, R3, 0    ; R2 = MEM[50]
; expect R2 = 42