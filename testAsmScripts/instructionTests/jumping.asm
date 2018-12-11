addi $s6, $zero, 5
addi $s7, $zero, 3
addi $s1, $zero, 0
func1: addi $s0, $zero, 0
loop1: addi $s0, $s0, 1
bne $s0, $s6, loop1
addi $s1, $s1, 1
BEQ $s1, $s7, end
J func1
end: