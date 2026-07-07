        LLI  R1, 10
        LLI  R2, 3
        ADA  R3, R1, R2
        NDU  R4, R1, R2
        ADI  R3, R3, -3
        LLI  R5, 200
        SW   R3, R5, 0
        LW   R6, R5, 0
        BEQ  R3, R6, done
        LLI  R7, 99
done:
        LLI  R7, 42
        halt