multiply:
LLI R3, 0
add_loop:
BEQ R2, R0, ret
ADA R3, R3, R1
ADI R2, R2, -1
BEQ R0, R0, add_loop
ret:
JLR R0, R7