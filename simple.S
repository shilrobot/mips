
.data
foo: .asciiz "HELLO WORLD!\n"

.text
.globl main

main:
	la $t0, foo
loop:
	lb $t1, 0($t0)
	beqz $t1, done
	addi $a0, $t1, 0
	syscall
	addi $t0, $t0, 1
	j loop
	
done:
	j done