#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "Common.h"

class Memory
{
public:
	uint8_t* m_mem;
	uint32_t m_memSize;

	Memory()
	{
		m_memSize = 64*1024;
		m_mem = (uint8_t*)malloc(m_memSize);
		// memset(m_mem, 0xFF, m_memSize);
		assert(m_mem != 0);
	}

	bool ReadWord(uint32_t addr, uint32_t* word)
	{
		assert(word != 0);
		assert((addr & 0x3) == 0);
		if(addr >= m_memSize)
			return false;
		*word = SDL_SwapBE32(*(uint32_t*)&m_mem[addr]);
		return true;
	}

	bool WriteWord(uint32_t addr, uint32_t word)
	{
		assert((addr & 0x3) == 0);
		if(addr >= m_memSize)
			return false;
		*(uint32_t*)&m_mem[addr] = SDL_SwapBE32(word);
		return true;
	}

	bool ReadHalfWord(uint32_t addr, uint16_t* halfword)
	{
		assert(halfword != 0);
		assert((addr & 0x1) == 0);
		if(addr >= m_memSize)
			return false;
		*halfword = SDL_SwapBE16(*(uint16_t*)&m_mem[addr]);
		return true;
	}

	bool WriteHalfWord(uint32_t addr, uint16_t halfword)
	{
		assert((addr & 0x1) == 0);
		if(addr >= m_memSize)
			return false;
		*(uint16_t*)&m_mem[addr] = SDL_SwapBE16(halfword);
		return true;
	}

	bool ReadByte(uint32_t addr, uint8_t* byte)
	{
		assert(byte != 0);
		if(addr >= m_memSize)
			return false;
		*byte = m_mem[addr];
		return true;
	}

	bool WriteByte(uint32_t addr, uint8_t byte)
	{
		if(addr >= m_memSize)
			return false;
		m_mem[addr] = byte;
		return true;
	}
};

#endif // __MEMORY_H__
