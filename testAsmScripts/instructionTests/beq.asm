#~~~BRANCH EQUAL TEST~~~

ADDI $s0, $zero, 0
ADDI $s1, $zero, 2
branch1: ADDI $s0, $s0, 2
BEQ $s0, $s1, branch1