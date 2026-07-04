; ===================================================
; Test 2: Verification of NAND Operations and JLR Flows
; ===================================================

        LLI  R1, 255     ; Load upper unsigned 8-bit limit (0xFF)
        LLI  R2, 15      ; Load lower 4-bit nibble limit (0x0F)
        NDU  R3, R1, R2  ; Core Test: NDU R3, R1, R2 -> Expected machine word: 0x2648
        JLR  R7, R1      ; Control flow check: Save return context to R7 and jump to R1