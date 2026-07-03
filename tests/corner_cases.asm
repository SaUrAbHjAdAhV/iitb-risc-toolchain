; ===================================================
; IITB-RISC Core Engine Complete Stress Test Pipeline
; ===================================================

_start:
        ; 1. J-Type Boundary Tests (LLI, Vector Blocks)
        LLI  R1, 511        ; Max Unsigned 9-bit Boundary (0x1FF)
        LLI  R2, -256       ; Min Signed 9-bit Boundary
        LM   R3, 255        ; Max Unsigned 8-bit Vector Mask (0xFF)
        SM   R4, 0          ; Min Unsigned 8-bit Vector Mask

        ; 2. R-Type Variant Checklist (ALU Operations)
        ADA  R1, R2, R3     ; Regular ADD (op=0000, comp=0, cz=00)
        ADZ  R4, R5, R6     ; ADD if Zero (op=0000, comp=0, cz=01)
        ACA  R7, R1, R2     ; Complement ADD (op=0000, comp=1, cz=00)
        ACW  R3, R4, R5     ; Complement ADD + Carry Writeback (op=0000, comp=1, cz=11)
        NDU  R6, R7, R1     ; Regular NAND (op=0010, comp=0, cz=00)
        NCC  R2, R3, R4     ; Complement NAND if Carry (op=0010, comp=1, cz=10)

        ; 3. I-Type Immediate Boundaries & Quirks
        ADI  R5, R6, 31     ; Max Positive 6-bit Signed Immediate
        ADI  R7, R1, -32    ; Min Negative 6-bit Signed Immediate
        JLR  R2, R3         ; Quirky I-Type (No immediate value processed)

        ; 4. Control Flow Jumps & Offset Tracking
forward_target:
        BEQ  R1, R2, forward_target ; Forward Jump (Positive Offset)
        BEQ  R3, R4, backward_loop  ; Backward Jump (Negative Offset)

backward_loop:
        JAL  R0, _start     ; Max 9-bit Signed Jump Reference