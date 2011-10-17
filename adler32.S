
.data
str:
.asciiz "Blah blah ah ah aahhh !!2~"
MOD_ADLER:
.word 65521

.text
.globl main

main:
	li $t0, 1		# A = 1
	li $t1, 0 		# B = 0
	la $t2, str		# t2 = &str[0]
	lw $t4, MOD_ADLER	# MOD_ADLER
loop:
	lbu $t3, 0($t2)	# load byte at t2
	beqz $t3, done	# if it's null, end
	addu $t2, 1		# otherwise increment t2
	
	addu $t0, $t0, $t3
	remu $t0, $t0, $t4
	
	addu $t1, $t1, $t0
	remu $t1, $t1, $t4	
	
	j loop
	nop
	
done:
	sll $t1, $t1, 16			# Finalize checksum in t0
	or $t0, $t0, $t1

forever:
	j forever
	nop
	
	
