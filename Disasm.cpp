// Table-driven MIPS I disassembler.
// Inspired by PCSX2 (http://code.google.com/p/pcsx2/)

#include "Common.h"
#include "Disasm.h"

struct DisasmInfo
{
	char*		buf;
	size_t		bufLen;
	uint32_t	pc;
	uint32_t	instr;
};

typedef void (*DispatchFunc)(DisasmInfo&);

static const char* g_gprNames[] = {
	"r0",  "at",  "v0",  "v1",  "a0",  "a1",  "a2",  "a3",
	"t0",  "t1",  "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
};

/*
static const char* g_fprNames[] = {
	"f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
	"f8",  "f9",  "f10", "f11", "f12", "f13", "f14", "f15",
	"f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
	"f24", "f25", "f26", "f29", "f28", "f29", "f30", "f31",
};

static const char* g_fmtNames[] = {
	"?", "?", "?", "?", "?", "?", "?", "?",
	"?", "?", "?", "?", "?", "?", "?", "?",
	"F", "D", "?", "?", "W", "?", "?", "?",
	"?", "?", "?", "?", "?", "?", "?", "?",
};
*/

void ToLowerInplace(char* buf)
{
	while(*buf != '\0')
	{
		*buf = tolower(*buf);
		++buf;
	}
}

void Format(DisasmInfo& di, const char* fmt, ...)
{
	size_t currLen = strlen(di.buf);

	va_list args;
	va_start(args, fmt);
	vsnprintf(di.buf+currLen, di.bufLen-currLen, fmt, args);
	di.buf[di.bufLen-1] = '\0';
	va_end(args);
}

#define RS	Format(di, " %s", g_gprNames[(di.instr>>21)&0x1F]);
#define RT	Format(di, " %s", g_gprNames[(di.instr>>16)&0x1F]);
#define RD	Format(di, " %s", g_gprNames[(di.instr>>11)&0x1F]);

#define FT	Format(di, " %s", g_fprNames[(di.instr>>16)&0x1F]);
#define FS	Format(di, " %s", g_fprNames[(di.instr>>11)&0x1F]);
#define FD	Format(di, " %s",g_fprNames[(di.instr>>6)&0x1F]);

//#define FMT snprintf(di.buf, di.bufLen, "%s", di.buf, g_fmtNames[(di.instr>>21)&0x1F]);
#define SA Format(di, " %d", (di.instr>>6)&0x1F);

#define IMM_SEXT Format(di, " %d", (int)(short)(di.instr & 0xFFFF));
#define IMM_ZEXT Format(di, " 0x%x", di.instr & 0xFFFF);

#define TARGET_16 Format(di, " %x", di.pc + 4 + (short)(di.instr & 0xFFFF)*4);
#define TARGET_26 Format(di, " %x", (di.pc & 0xF0000000) | ((di.instr & 0x3FFFFFF)<<2));
#define OFFSET_BASE Format(di, " %d(%s)", (int)(short)(di.instr & 0xFFFF), g_gprNames[(di.instr>>21)&0x1F]);

//#define CODE snprintf(di.buf, di.bufLen, "%s 0x%x", di.buf, (di.instr >> 6) & 0xFFFFF);

#define INSTR(_name) static void _ ## _name (DisasmInfo& di) { Format(di, "%-7s", #_name);  ToLowerInplace(di.buf); 
#define INSTR2(_name, _name2) static void _ ## _name (DisasmInfo& di) { Format(di, "%s", #_name2); ToLowerInplace(di.buf); 
#define END }

void _BAD(DisasmInfo& di) { Format(di, "???"); }

//INSTR2(ABS_FMT, "ABS.") FMT FD FS END
INSTR(ADD) RD RS RT END
//INSTR2(ADD_FMT, "ADD.") FMT FD FS FT END
INSTR(ADDI) RT RS IMM_SEXT END
INSTR(ADDIU) RT RS IMM_SEXT END
INSTR(ADDU) RD RS RT END
INSTR(AND) RD RS RT END
INSTR(ANDI) RT RS IMM_ZEXT END
// BC1F
// BC1T
// BC2F
// BC2T
INSTR(BEQ) RS RT TARGET_16 END
INSTR(BGEZ) RS TARGET_16 END
INSTR(BGEZAL) RS TARGET_16 END
INSTR(BGTZ) RS TARGET_16 END
INSTR(BLEZ) RS TARGET_16 END
INSTR(BLTZ) RS TARGET_16 END
INSTR(BLTZAL) RS TARGET_16 END
INSTR(BNE) RS RT TARGET_16 END
INSTR(BREAK) /*CODE*/ END
// TODO: C.cond.fmt
//INSTR(CFC1) RT FS END
//INSTR(CFC2) RT RD END
//INSTR(CTC1) RT FS END
//INSTR(CTC2) RT RD END
// TODO: CVT.fmt1.fmt2
INSTR(DIV) RS RT END
//INSTR2(DIV_FMT, "DIV.") FMT FD FS FT END
INSTR(DIVU) RS RT END
INSTR(J) TARGET_26 END
INSTR(JAL) TARGET_26 END
INSTR(JALR) RD RS END
INSTR(JR) RS END
INSTR(LB) RT OFFSET_BASE END
INSTR(LBU) RT OFFSET_BASE END
INSTR(LH) RT OFFSET_BASE END
INSTR(LHU) RT OFFSET_BASE END
INSTR(LUI) RT IMM_ZEXT END
INSTR(LW) RT OFFSET_BASE END
//INSTR(LWC1) FT OFFSET_BASE END
//INSTR(LWC2) RT OFFSET_BASE END
INSTR(LWL) RT OFFSET_BASE END
INSTR(LWR) RT OFFSET_BASE END
//INSTR(MFC1) RT FS END
INSTR(MFHI) RD END
INSTR(MFLO) RD END
//INSTR2(MOV_FMT, "MOV.") FMT FD FS END
//INSTR(MTC1) RT FS END
INSTR(MTHI) RS END
INSTR(MTLO) RS END
INSTR(MULT) RS RT END
INSTR(MULTU) RS RT END
//INSTR2(NEG_FMT, "NEG.") FMT FD FS END
INSTR(NOR) RD RS RT END
INSTR(OR) RD RS RT END
INSTR(ORI) RT RS IMM_ZEXT END
INSTR(SB) RT OFFSET_BASE END
INSTR(SH) RT OFFSET_BASE END
INSTR(SLL) RD RT SA END
INSTR(SLLV) RD RT RS END
INSTR(SLT) RD RS RT END
INSTR(SLTI) RT RS IMM_SEXT END
INSTR(SLTIU) RT RS IMM_SEXT END
INSTR(SLTU) RD RS RT END
INSTR(SRA) RD RT SA END
INSTR(SRAV) RD RT RS END
INSTR(SRL) RD RT SA END
INSTR(SRLV) RD RT RS END
INSTR(SUB) RD RS RT END
//INSTR2(SUB_FMT, "SUB.") FMT FD FS FT END
INSTR(SUBU) RD RS RT END
INSTR(SW) RT OFFSET_BASE END
//INSTR(SWC1) FT OFFSET_BASE END
//INSTR(SWC2) RT OFFSET_BASE END
INSTR(SWL) RT OFFSET_BASE END
INSTR(SWR) RT OFFSET_BASE END
INSTR(SYSCALL) /*CODE*/ END
INSTR(XOR) RD RS RT END
INSTR(XORI) RT RS IMM_ZEXT END

// TODO: Blah.

static DispatchFunc g_regimmTable[] = {
	/* 00 */	_BLTZ,		_BGEZ,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 01 */	_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 10 */	_BLTZAL,	_BGEZAL,	_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 11 */	_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
};

void _REGIMM(DisasmInfo& di) { g_regimmTable[(di.instr >> 16) & 0x1F](di); }

static DispatchFunc g_specialTable[] = {
	/* 000 */	_SLL,		_BAD,		_SRL,		_SRA,		_SLLV,		_BAD,		_SRLV,		_SRAV,
	/* 001 */	_JR,		_JALR,		_BAD,		_BAD,		_SYSCALL,	_BREAK,		_BAD,		_BAD,
	/* 010 */	_MFHI,		_MTHI,		_MFLO,		_MTLO,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 011 */	_MULT,		_MULTU,		_DIV,		_DIVU,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 100 */	_ADD,		_ADDU,		_SUB,		_SUBU,		_AND,		_OR,		_XOR,		_NOR,
	/* 101 */	_BAD,		_BAD,		_SLT,		_SLTU,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 110 */	_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 111 */	_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
};

void _SPECIAL(DisasmInfo& di) { g_specialTable[di.instr & 0x3F](di); }

static DispatchFunc g_opcodeTable[] = {
	/* 000 */	_SPECIAL,	_REGIMM,	_J,			_JAL,		_BEQ,		_BNE,		_BLEZ,		_BGTZ,
	/* 001 */	_ADDI,		_ADDIU,		_SLTI,		_SLTIU,		_ANDI,		_ORI,		_XORI,		_LUI,
	/* 010 */	_BAD,	_BAD/*_COP1*/,_BAD/*_COP2*/,_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 011 */	_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 100 */	_LB,		_LH,		_LWL,		_LW,		_LBU,		_LHU,		_LWR,		_BAD,
	/* 101 */	_SB,		_SH,		_SWL,		_SW,		_BAD,		_BAD,		_SWR,		_BAD,
	/* 110 */	_BAD,	_BAD/*_LWC1*/,_BAD/*_LWC2*/,_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
	/* 111 */	_BAD,	_BAD/*_SWC1*/,_BAD/*_SWC2*/,_BAD,		_BAD,		_BAD,		_BAD,		_BAD,
};

void Disassembler::Disassemble(uint32_t instr, uint32_t pc, char* buf, size_t bufLen)
{
	DisasmInfo di;
	di.buf = buf;
	di.bufLen = bufLen;
	di.instr = instr;
	di.pc = pc;
	di.buf[0] = '\0';
	if(instr == 0)
		strncpy(buf, "nop", bufLen);
	else
		g_opcodeTable[instr >> 26](di);
	buf[bufLen-1] = '\0';
}

const char* Disassembler::GetGPRName(int gpr)
{
	assert(gpr >= 0 && gpr < 32);
	return g_gprNames[gpr];
}
