
.text
.globl _start

_start:
	la $gp, _gp

	la $sp, 0x10000
	subu $sp, $sp, 64
	
	/* Zero out the BSS section -- technically not necessary since emulator does this */
	la $t0, _fbss
	la $t1, _end
_zerobss_loop:
	sw $zero, 0($t0)
	addiu $t0, $t0, 4 
	bne $t0, $t1, _zerobss_loop

	
	jal _init

	jal main

_infinite_loop:
	j _infinite_loop
