#~~~JUMP TEST~~~

#init registers
addi $s6, $zero, 5
addi $s7, $zero, 3
addi $s1, $zero, 0

#increment s0 from 0 to 5
func1: addi $s0, $zero, 0
loop1: addi $s0, $s0, 1
bne $s0, $s6, loop1

#increment s1 from 0 to 3
addi $s1, $s1, 1
BEQ $s1, $s7, end
J func1

end: