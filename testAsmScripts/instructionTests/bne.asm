#~~~BRANCH NOT EQUAL TEST~~~

ADDI $s0, $zero, 0
ADDI $s1, $zero, 10
branch1: ADDI $s0, $s0, 2
BNE $s0, $s1, branch1