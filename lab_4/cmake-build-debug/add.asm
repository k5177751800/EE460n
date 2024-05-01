.ORIG x3000
        LEA R0, ADR
        LDW R0, R0, #0
        AND R1, R1, #0
        ADD R1, R1, #1
        STW R1, R0, #0

        LEA R1, INT
        LDW R0, R1, #0
        ADD R0, R0, #9
        LEA R0, SUM
        LDW R1, R0, #0
        ADD R2, R2, #10
        ADD R2, R2, #10

LOOP    LDW R0, R0, #0
        ADD R1, R1, R0
        ADD R1, R1, #-1
        ADD R0, R1, #2
        BRp LOOP
        STW R0, R1, #0
HALT
ADR .FILL x4000
INT .FILL xC000
SUM .FILL x0000
.END