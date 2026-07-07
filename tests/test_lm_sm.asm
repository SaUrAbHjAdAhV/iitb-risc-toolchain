; Initialize values in registers to prepare for saving
        LLI  R1, 0xAA      ; Test pattern A
        LLI  R2, 0xBB      ; Test pattern B
        LLI  R3, 0xCC      ; Test pattern C
        
        ; Set up a safe memory base address pointer in R4
        LLI  R4, 500       ; Base pointer = 500

        ; Store Multiple (SM) Test
        ; Mask: 0b00001110 = 0x0E (Decimal 14)
        ; This bitmask turns on bits 1, 2, and 3.
        ; Bit 1 -> R6, Bit 2 -> R5, Bit 3 -> R4
        ; Wait! Let's clear out other registers first to test cleanly.
        ; Let's store R1, R2, and R3. 
        ; Based on your mapping: regIdx = 7 - bit.
        ; To store R1, R2, R3 -> bits needed are 6, 5, 4 (7-6=1, 7-5=2, 7-4=3)
        ; Mask = (1<<6) | (1<<5) | (1<<4) = 0x40 | 0x20 | 0x10 = 0x70 (Decimal 112)
        SM   R4, 112       ; Copy R1, R2, R3 sequentially into memory starting at MEM[500]

        ; Clear out R1, R2, and R3 so we can prove the Load Multiple actually works
        LLI  R1, 0
        LLI  R2, 0
        LLI  R3, 0

        ; Load Multiple (LM) Test
        ; Use the exact same mask (112) to read those values back from MEM[500]
        LM   R4, 112       ; Restore R1, R2, R3 from memory!

        halt