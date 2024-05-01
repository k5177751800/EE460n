   .ORIG x3000
    AND R1,R1,#0
    AND R2,R2,#0
    AND R3,R3,#0
    AND R4,R4,#0
    AND R5,R5,#0
    AND R6,R6,#0
    AND R7,R7,#0

    LEA R0,A
    LDW R1,R0,#0
    LDB R2,R1,#0 ;R2 == x3100
    CMP R2,#0
    BRzp B
    ADD R5,R5,#1

B   LDB R3,R1,#1 ;R3 x3101
    CMP R3,#0
    BRnz C
    ADD R5,R5,#-1

C   ADD R4,R3,R2
    CMP R5,#0
    BRz en
    CMP R5,#0
    BRp neg
    CMP R5,#0
    BRn pos

neg CMP R4,#0
    BRp OF
    BR en

pos CMP R4,#0
    BRn OF
    BR en

OF  ADD R6,R6,#1
    STB R6,R1,#3
en  STB R4,R1,#2

A .FIll x3100
   .END 