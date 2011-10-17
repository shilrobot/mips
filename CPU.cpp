#include "Common.h"
#include "CPU.h"
#include "Memory.h"

void Log(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[256];
	vsnprintf(buf, 256, fmt, args);
	buf[255] = '\0';
	va_end(args);

	printf("%s\n",buf);
};

class InstrCounter
{
public:
	const char* m_name;
	uint64_t m_count;
	InstrCounter* m_next;

	static InstrCounter* s_head;
public:
	InstrCounter(const char* name) : m_name(name), m_count(0), m_next(0)
	{
		m_next = s_head;
		s_head = this;
	}
};

InstrCounter* InstrCounter::s_head = 0;

typedef void (*ExecFunc)(CPU* cpu, uint32_t instr);

/*
#define INSTR(_name) \
	static InstrCounter g_Counter##_name(#_name); \
	static void _Inner##_name(CPU* cpu, uint32_t instr); \
	static void _##_name(CPU* cpu, uint32_t instr) { g_Counter##_name.m_count++; _Inner##_name(cpu,instr); } \
	static void _Inner##_name(CPU* cpu, uint32_t instr)
*/

#define INSTR(_name) static void _##_name(CPU* cpu, uint32_t instr)

#define RS_INDEX	((instr>>21)&0x1F)
#define RT_INDEX	((instr>>16)&0x1F)
#define RD_INDEX	((instr>>11)&0x1F)

#define RS	(cpu->m_gprs[RS_INDEX])
#define RT	(cpu->m_gprs[RT_INDEX])
#define RD	(cpu->m_gprs[RD_INDEX])
#define IMM_SEXT ((uint32_t)(short)(instr&0xFFFF))
#define IMM_ZEXT ((instr&0xFFFF))
#define SA ((instr>>6)&0x1F)

#define PC (cpu->m_pc)
#define RA (cpu->m_gprs[31])

// TODO: is this pc+4 or pc?
#define JUMP_TARGET (((cpu->m_pc+4) & 0xF0000000) | ((instr & 0x3FFFFFF)<<2))
#define BRANCH_TARGET (cpu->m_pc + 4 + ((short)(instr & 0xFFFF)<<2))

#define LO (cpu->m_lo)
#define HI (cpu->m_hi)

#define SAVE_RT(_val) do { if(RT_INDEX != 0) cpu->m_gprs[RT_INDEX] = (_val); } while(0)
#define SAVE_RD(_val) do { if(RD_INDEX != 0) cpu->m_gprs[RD_INDEX] = (_val); } while(0)

#define OFFSET_BASE (RS + IMM_SEXT)

#define EXC(_val) do { printf("Exception %d at %s:%d PC=%X\n", (_val), __FILE__,__LINE__, PC); cpu->m_exception = (_val); return; } while(0)

//----------------------------------------------------------------------------
// ARITHMETIC INSTRUCTIONS
//----------------------------------------------------------------------------

INSTR(ADD) {
	int64_t sum = (int64_t)(int32_t)RT + (int64_t)(int32_t)RT;
	if(((sum >> 32) & 0x1) != ((sum >> 31) & 0x1))
		EXC(Exc_IntegerOverflow);
	SAVE_RD((uint32_t)sum);
}

INSTR(ADDI) {
	int64_t sum = (int64_t)(int32_t)RS + (int64_t)(int32_t)IMM_SEXT;
	if(((sum >> 32) & 0x1) != ((sum >> 31) & 0x1))
		EXC(Exc_IntegerOverflow);
	SAVE_RT((uint32_t)sum);
}

INSTR(ADDU) {
	SAVE_RD(RS + RT);
}

INSTR(ADDIU) {
	SAVE_RT(RS + IMM_SEXT);
}

INSTR(LUI) {
	SAVE_RT(IMM_ZEXT << 16);
}

INSTR(SUB) {
	int64_t diff = (int64_t)(int32_t)RS - (int64_t)(int32_t)RT;
	if(((diff >> 32) & 0x1) != ((diff >> 31) & 0x1))
		EXC(Exc_IntegerOverflow);
	SAVE_RD((uint32_t)diff);	
}

INSTR(SUBU) {
	SAVE_RD(RS - RT);	
}

INSTR(AND) {
	SAVE_RD(RS & RT);
}

INSTR(ANDI) {
	SAVE_RT(RS & IMM_ZEXT);
}

INSTR(OR) {
	SAVE_RD(RS | RT);
}

INSTR(ORI) {
	SAVE_RT(RS | IMM_ZEXT);
}

INSTR(XOR) {
	SAVE_RD(RS ^ RT);
}

INSTR(XORI) {
	SAVE_RT(RS ^ IMM_ZEXT);
}

INSTR(NOR) {
	SAVE_RD(~(RS | RT));
}

INSTR(SLL) {
	SAVE_RD(RT << SA);
}

INSTR(SLLV) {
	SAVE_RD(RT << (RS & 0x1F));
}

INSTR(SRL) {
	SAVE_RD(RT >> SA);
}

INSTR(SRLV) {
	SAVE_RD(RT >> (RS & 0x1F));
}

// TODO: TEST SRA & SRAV
INSTR(SRA) {
	SAVE_RD((int32_t)RT >> SA);
}

INSTR(SRAV) {
	SAVE_RD((int32_t)RT >> (RS & 0x1F));
}

INSTR(MFLO) {
	SAVE_RD(LO);
}

INSTR(MFHI) {
	SAVE_RD(HI);
}

INSTR(MTLO) {
	LO = RS;
}

INSTR(MTHI) {
	HI = RS;
}

INSTR(SLT) {
	if((int32_t)RS < (int32_t)RT)
		SAVE_RD(1);
	else
		SAVE_RD(0);
}

INSTR(SLTI) {
	if((int32_t)RS < (int32_t)IMM_SEXT)
		SAVE_RT(1);
	else
		SAVE_RT(0);
}

INSTR(SLTU) {
	if(RS < RT)
		SAVE_RD(1);
	else
		SAVE_RD(0);
}

INSTR(SLTIU) {
	if(RS < IMM_ZEXT)
		SAVE_RT(1);
	else
		SAVE_RT(0);
}

INSTR(MULT) {
	int64_t product = (int64_t)(int32_t)RS * (int64_t)(int32_t)RT;
	LO = (int32_t)(product & 0xFFFFFFFF);
	HI = (int32_t)((product >> 32) & 0xFFFFFFFF);
}

INSTR(MULTU) {
	uint64_t product = (uint64_t)RS * (uint64_t)RT;
	LO = (product & 0xFFFFFFFF);
	HI = ((product >> 32) & 0xFFFFFFFF);
}

INSTR(DIV) {
	if(RT != 0)
	{
		if(RS == 0x80000000 && RT == 0xFFFFFFFF)
		{
			// TODO: Is this the correct thing?
			EXC(Exc_IntegerOverflow);
		}
		else
		{
			LO = (int32_t)RS / (int32_t)RT;
			HI = (int32_t)RS % (int32_t)RT;
		}
	}
	else
	{
		// TODO: What should these be set to?
		LO = HI = 0;
	}
}

INSTR(DIVU) {
	if(RT != 0)
	{
		LO = RS / RT;
		HI = RS % RT;
	}
	else
	{
		// TODO: What should these be set to?
		LO = HI = 0;
	}
}

//----------------------------------------------------------------------------
// MEMORY INSTRUCTIONS
//----------------------------------------------------------------------------

INSTR(LW) {
	uint32_t addr = OFFSET_BASE;
	if((addr & 0x3) != 0)
		EXC(Exc_AddressError);
	uint32_t value;
	if(!cpu->m_mem->ReadWord(addr, &value))
		EXC(Exc_BusError);
	SAVE_RT(value);
}

INSTR(LH) {
	uint32_t addr = OFFSET_BASE;
	if((addr & 0x1) != 0)
		EXC(Exc_AddressError);
	uint16_t value;
	if(!cpu->m_mem->ReadHalfWord(addr, &value))
		EXC(Exc_BusError);
	SAVE_RT((int16_t)value);
}

INSTR(LHU) {
	uint32_t addr = OFFSET_BASE;
	if((addr & 0x1) != 0)
		EXC(Exc_AddressError);
	uint16_t value;
	if(!cpu->m_mem->ReadHalfWord(addr, &value))
		EXC(Exc_BusError);
	SAVE_RT(value);
}

INSTR(LB) {
	uint8_t value;
	if(!cpu->m_mem->ReadByte(OFFSET_BASE, &value))
		EXC(Exc_BusError);
	SAVE_RT((int8_t)value);
}

INSTR(LBU) {
	uint8_t value;
	if(!cpu->m_mem->ReadByte(OFFSET_BASE, &value))
		EXC(Exc_BusError);
	SAVE_RT(value);
}

INSTR(SW) {
	uint32_t addr = OFFSET_BASE;
	if((addr & 0x3) != 0)
		EXC(Exc_AddressError);
	if(!cpu->m_mem->WriteWord(addr, RT))
		EXC(Exc_BusError);
}

INSTR(SH) {
	uint32_t addr = OFFSET_BASE;
	if((addr & 0x1) != 0)
		EXC(Exc_AddressError);
	if(!cpu->m_mem->WriteHalfWord(addr, (uint16_t)RT))
		EXC(Exc_BusError);
}

INSTR(SB) {
	uint32_t addr = OFFSET_BASE;
	if(!cpu->m_mem->WriteByte(addr, (uint8_t)RT))
		EXC(Exc_BusError);
}

// NOTE: LWL/LWR/SWL/SWR assume big endian system here

INSTR(LWL) {
	uint32_t addr = OFFSET_BASE;
	uint32_t bytesToCopy = 4 - (addr & 0x3);

	uint8_t buf[4] = {
		(uint8_t)(RT >> 24),
		(uint8_t)(RT >> 16),
		(uint8_t)(RT >> 8),
		(uint8_t)(RT >> 0),
	};

	for(uint32_t i=0; i<bytesToCopy; ++i)
		if(!cpu->m_mem->ReadByte(addr++, &buf[i]))
			EXC(Exc_BusError);

	if(RT_INDEX != 0)
	{
		RT = (buf[0] << 24) |
			(buf[1] << 16) |
			(buf[2] << 8) |
			buf[3];
	}
}

INSTR(LWR) {
	uint32_t addr = OFFSET_BASE;
	uint32_t startFrom = 4 - (addr & 0x3);

	uint8_t buf[4] = {
		(uint8_t)(RT >> 24),
		(uint8_t)(RT >> 16),
		(uint8_t)(RT >> 8),
		(uint8_t)(RT >> 0),
	};

	addr += startFrom;

	for(uint32_t i=startFrom; i<4; ++i)
		if(!cpu->m_mem->ReadByte(addr++, &buf[i]))
			EXC(Exc_BusError);

	if(RT_INDEX != 0)
	{
		RT = (buf[0] << 24) |
			(buf[1] << 16) |
			(buf[2] << 8) |
			buf[3];
	}
}

INSTR(SWL) {
	uint32_t addr = OFFSET_BASE;
	uint32_t bytesToCopy = 4 - (addr & 0x3);

	uint8_t buf[4] = {
		(uint8_t)(RT >> 24),
		(uint8_t)(RT >> 16),
		(uint8_t)(RT >> 8),
		(uint8_t)(RT >> 0),
	};

	for(uint32_t i=0; i<bytesToCopy; ++i)
		if(!cpu->m_mem->WriteByte(addr++, buf[i]))
			EXC(Exc_BusError);
}

INSTR(SWR) {
	uint32_t addr = OFFSET_BASE;
	uint32_t startFrom = 4 - (addr & 0x3);

	addr += startFrom;

	uint8_t buf[4] = {
		(uint8_t)(RT >> 24),
		(uint8_t)(RT >> 16),
		(uint8_t)(RT >> 8),
		(uint8_t)(RT >> 0),
	};

	for(uint32_t i=startFrom; i<4; ++i)
		if(!cpu->m_mem->WriteByte(addr++, buf[i]))
			EXC(Exc_BusError);
}

//----------------------------------------------------------------------------
// BRANCH INSTRUCTIONS
//----------------------------------------------------------------------------

void ComplainDelaySlot(CPU* cpu)
{
	Log("%8x: Dangerous instruction in delay slot of taken jump or branch", cpu->m_pc);
}

#define CHECK_DELAY_SLOT do { if(cpu->m_inDelaySlot) { ComplainDelaySlot(cpu); } } while(0)
#define JUMP(_target) do { cpu->m_createDelaySlot = true; cpu->m_jumpTarget = (_target); } while(0)

INSTR(J) {
	CHECK_DELAY_SLOT;
	JUMP(JUMP_TARGET);
}

INSTR(JAL) {
	CHECK_DELAY_SLOT;
	JUMP(JUMP_TARGET);
	RA = PC + 8;
}

INSTR(JR) {
	CHECK_DELAY_SLOT;
	JUMP(RS);
}

INSTR(JALR) {
	CHECK_DELAY_SLOT;
	JUMP(RS);
	RA = PC + 8;
}

INSTR(BEQ) {
	CHECK_DELAY_SLOT;
	if(RS == RT)
		JUMP(BRANCH_TARGET);
}

INSTR(BNE) {
	CHECK_DELAY_SLOT;
	if(RS != RT)
		JUMP(BRANCH_TARGET);
}

INSTR(BGEZ) {
	CHECK_DELAY_SLOT;
	if((int32_t)RS >= 0)
		JUMP(BRANCH_TARGET);
}

INSTR(BGEZAL) {
	CHECK_DELAY_SLOT;
	if(RS_INDEX == 31)
		Log("%8x: r31 used as argument to BGEZAL", PC);
	if((int32_t)RS >= 0)
		JUMP(BRANCH_TARGET);
	RA = PC+8;
}

INSTR(BGTZ) {
	CHECK_DELAY_SLOT;
	if((int32_t)RS > 0)
		JUMP(BRANCH_TARGET);
}

INSTR(BLTZ) {
	CHECK_DELAY_SLOT;
	if((int32_t)RS < 0)
		JUMP(BRANCH_TARGET);
}

INSTR(BLTZAL) {
	CHECK_DELAY_SLOT;
	if(RS_INDEX == 31)
		Log("%8x: r31 used as argument to BGEZAL", PC);
	if((int32_t)RS < 0)
		JUMP(BRANCH_TARGET);
	RA = PC+8;
}

INSTR(BLEZ) {
	CHECK_DELAY_SLOT;
	if((int32_t)RS <= 0)
		JUMP(BRANCH_TARGET);
}


//----------------------------------------------------------------------------
// MISC
//----------------------------------------------------------------------------

INSTR(SYSCALL) {
	/*Log("%8x: Syscall", PC);
	EXC(Exc_Syscall);*/
	printf("%c", (char)cpu->m_gprs[26]); // a0
}

INSTR(BREAK) {
	Log("%8x: Break", PC);
	EXC(Exc_Break);
}

INSTR(BAD) {
	Log("%8x: Invalid instruction encoding", PC);
	EXC(Exc_InvalidInstruction);
}

//----------------------------------------------------------------------------
// DISPATCH CODE
//----------------------------------------------------------------------------

static ExecFunc g_regimmTable[] = {
	/* 00 */	_BLTZ,		_BGEZ,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 01 */	_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 10 */	_BLTZAL,	_BGEZAL,	_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 11 */	_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
};

void _REGIMM(CPU* cpu, uint32_t instr) { g_regimmTable[(instr >> 16) & 0x1F](cpu, instr); }

static ExecFunc g_specialTable[] = {
	/* 000 */	_SLL,		_BAD,		_SRL,		_SRA,		_SLLV,		_BAD,		_SRLV,		_SRAV,
	/* 001 */	_JR,		_JALR,		_BAD,		_BAD,		_SYSCALL,	_BREAK,		_BAD,		_BAD,
	/* 010 */	_MFHI,		_MTHI,		_MFLO,		_MTLO,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 011 */	_MULT,		_MULTU,		_DIV,		_DIVU,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 100 */	_ADD,		_ADDU,		_SUB,		_SUBU,		_AND,		_OR,		_XOR,		_NOR,
	/* 101 */	_BAD,		_BAD,		_SLT,		_SLTU,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 110 */	_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 111 */	_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
};

void _SPECIAL(CPU* cpu, uint32_t instr) { g_specialTable[instr & 0x3F](cpu, instr); }

static ExecFunc g_opcodeTable[] = {
	/* 000 */	_SPECIAL,	_REGIMM,	_J,			_JAL,		_BEQ,		_BNE,		_BLEZ,		_BGTZ,
	/* 001 */	_ADDI,		_ADDIU,		_SLTI,		_SLTIU,		_ANDI,		_ORI,		_XORI,		_LUI,
	/* 010 */	_BAD,	_BAD/*_COP1*/,_BAD/*_COP2*/,_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 011 */	_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 100 */	_LB,		_LH,		_LWL,		_LW,		_LBU,		_LHU,		_LWR,		_BAD,
	/* 101 */	_SB,		_SH,		_SWL,		_SW,		_BAD,		_BAD,		_SWR,		_BAD,
	/* 110 */	_BAD,	_BAD/*_LWC1*/,_BAD/*_LWC2*/,_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 111 */	_BAD,	_BAD/*_SWC1*/,_BAD/*_SWC2*/,_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
};

void CPU::Step(uint32_t instrCount)
{
	CPU* cpu = this;

	for(uint32_t i = 0; i < instrCount; ++i)
	{
		// TODO: Exceptions
		if(cpu->m_exception != Exc_None)
			return;

		// Fetch the instruction
		if((PC & 0x3) != 0)
			EXC(Exc_AddressError);// TODO: Dammit who handles this
		uint32_t instr;
		if(!cpu->m_mem->ReadWord(PC, &instr))
			EXC(Exc_BusError); // TODO: Dammit who handles this

		/*uint32_t nextPC = cpu->m_createDelaySlot ? cpu->m_jumpTarget : PC+4;
		cpu->m_inDelaySlot = cpu->m_createDelaySlot;
		cpu->m_createDelaySlot = false;*/

		// Dispatch
		g_opcodeTable[instr>>26](cpu, instr);

		// TODO: Actually jump to right exception handler here, etc.
		PC = cpu->m_inDelaySlot ? cpu->m_jumpTarget : PC+4;

		cpu->m_inDelaySlot = cpu->m_createDelaySlot;
		cpu->m_createDelaySlot = false;
	}
}

void CPU::SaveStatistics()
{
	FILE* fp = fopen("cpustats.csv", "wt");
	if(!fp)
		return;
	InstrCounter* count = InstrCounter::s_head;
	while(count)
	{
#ifdef _MSC_VER
		fprintf(fp, "%s,%I64d\n", count->m_name, count->m_count);
#else
		fprintf(fp, "%s,%llu\n", count->m_name, count->m_count);
#endif
		count = count->m_next;
	}
	fclose(fp);
}

//----------------------------------------------------------------------------
// END OF INSTRUCTIONS
//----------------------------------------------------------------------------
