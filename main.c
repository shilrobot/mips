#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void writech(char);
void writes(char* s);

unsigned x = 0xCAFEBABE;

extern int _end;
void* heap_start;
void* break_loc;

void* sbrk(size_t x)
{
	void* old = break_loc;
	break_loc += x;
	/*char buf[32];
	sprintf(buf, "sbrk(%d)\n", x);
	writes(buf);*/
	return old;
}


void writech(char c)
{
	asm volatile("addu $k0, $zero, %0 \n syscall" ::"r"(c));
}

void writes(char* s)
{
	while(*s) writech(*s++);
}

void main()
{
	writes("Hello world!\n");
	heap_start = &_end;
	break_loc = heap_start;
	char buf[256];
	/*sprintf(buf, "%.3f %.3f\n", cos(M_PI), sin(M_PI));
	writes(buf);*/
	int i;
	for(i=0; i<=360; i+=5) {
		sprintf(buf, "cos(%d)=%.3f sin(%d)=%.3f\n",
			i, cos(i*2*M_PI/360.0), i, sin(i*2*M_PI/360.0));
		writes(buf);
	}

}

