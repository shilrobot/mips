#ifndef __CPU_H__
#define __CPU_H__

#include "Common.h"

class Memory;

enum Exception
{
	Exc_None,
	Exc_IntegerOverflow,
	Exc_BusError,
	Exc_AddressError,
	Exc_InvalidInstruction, // Is this the same as reserved? I dunno.
	Exc_Break,
	Exc_Syscall,
	// ...
};

class CPU
{
public:
	CPU(Memory* mem) : m_mem(mem)
	{
		assert(mem != 0);
		Reset();
	}

	void Reset()
	{
		memset(&m_gprs, 0, sizeof(m_gprs));
		m_hi = m_lo = 0;
		m_pc = 0;
		m_inDelaySlot = false;
		m_createDelaySlot = false;
		m_jumpTarget = 0;
		m_exception = Exc_None;
	}

	void Step(uint32_t instrCount);

	static void SaveStatistics();

	Memory*		m_mem;			
	uint32_t	m_gprs[32];		// 32 general programmable registers
	uint32_t	m_hi;			// HI register
	uint32_t	m_lo;			// LO register
	uint32_t	m_pc;			// current PC
	bool		m_inDelaySlot;	// Are we in a delay slot? 
	bool		m_createDelaySlot;
	uint32_t	m_jumpTarget;	// If m_inDelaySlot, where does our PC go next
	Exception	m_exception;	// Did an exception occur?

};

#endif // __CPU_H__
