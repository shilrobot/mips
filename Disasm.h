#ifndef __DISASM_H__
#define __DISASM_H__

#include "Common.h"

class Disassembler
{
public:
	static void Disassemble(uint32_t instr, uint32_t pc, char* buf, size_t bufLen);
	static const char* GetGPRName(int gpr);

private:
	Disassembler() {}
};

#endif // __DISASM_H__
