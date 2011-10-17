#include "Common.h"
#include "Disasm.h"
#include "Memory.h"
#include "CPU.h"


SDL_Surface* g_screen;
TTF_Font* g_font;

bool g_gprsChanged[32];
bool g_hiChanged = false;
bool g_loChanged = false;
uint32_t g_scroll = 0;
const int DISASSEMBLY_LINES = 29;

void FindRegisterChanges(CPU& oldState, CPU& newState)
{
	memset(&g_gprsChanged, 0, sizeof(g_gprsChanged));
	for(int i=0; i<32; ++i)
		g_gprsChanged[i] = newState.m_gprs[i] != oldState.m_gprs[i];
	g_hiChanged = newState.m_hi != oldState.m_hi;
	g_loChanged = newState.m_lo != oldState.m_lo;

	int halfScreen = DISASSEMBLY_LINES/2;

	if(newState.m_pc < g_scroll || newState.m_pc >= (g_scroll + (uint32_t)(DISASSEMBLY_LINES*4)))
	{
		if(newState.m_pc < (uint32_t)halfScreen*4)
			g_scroll = 0;
		else
			g_scroll = newState.m_pc - halfScreen*4;
	}
}

void DrawText(int x, int y, SDL_Color& col, SDL_Color& bg, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char buf[256];
	vsnprintf(buf, 256, format, args);
	buf[255] = '\0';
	va_end(args);

	SDL_Rect destRect = { x,y,0,0};

	SDL_Surface* text = TTF_RenderText_Shaded(g_font, buf, col,bg);
	destRect.w = text->w;
	destRect.h = text->h;
	SDL_BlitSurface(text, NULL, g_screen, &destRect);
	SDL_FreeSurface(text);
}

void DrawScreen(CPU& cpu)
{
	SDL_FillRect(g_screen, NULL, SDL_MapRGB(g_screen->format, 255,255,255));
	
	/*
	snprintf(buf, sizeof(buf), "  PC=%08x\n\n", cpu.m_pc);
	for(int i=0; i<16; i++)
		snprintf(buf, sizeof(buf),"%s%4s=%08x %4s=%08X\n", buf, Disassembler::GetGPRName(i), cpu.m_gprs[i], Disassembler::GetGPRName(i+16), cpu.m_gprs[i+16]);
	snprintf(buf, sizeof(buf),"%s  HI=%08x   LO=%08X\n",  buf, cpu.m_hi, cpu.m_lo);
	snprintf(buf, sizeof(buf),"%s\n", buf);
	snprintf(buf, sizeof(buf),"%sDelay slot: %s %s\n", buf, cpu.m_inDelaySlot ? "YES" : "NO", cpu.m_exception != Exc_None ? "-- EXCEPTION :(":"");
	snprintf(buf, sizeof(buf),"%s\n", buf);
	buf[1023] = '\0';
*/
	int margin = 5;

	SDL_Color white = {255,255,255};
	SDL_Color black = {0,0,0};
	SDL_Color red = {255,0,0};
	SDL_Color highlight = {255,255,0};

	char buf[256];
	for(uint32_t i=0; i <(uint32_t)DISASSEMBLY_LINES; ++i)
	{
		uint32_t addr = g_scroll + i*4;
		uint32_t instr = 0;
		SDL_Color currBG = addr == cpu.m_pc ? highlight:white;
		if(cpu.m_mem->ReadWord(addr, &instr))
		{
			Disassembler::Disassemble(instr, addr, buf, sizeof(buf));
			DrawText(margin, margin+16*i, black, currBG, "%08X:   %08X   %-24s", addr, instr, buf);
		}
		else
			DrawText(margin, margin+16*i, black, currBG, "%08X:   UNMAPPED", addr, instr, buf);
	}

	int registerX = 640-170;

	DrawText(registerX, margin, black, white, "PC=%08X", cpu.m_pc);
	for(int i=0; i<16; i++)
	{
		DrawText(registerX, 2*margin+(i+1)*15, g_gprsChanged[i]?red:black, white, "%2s=%08X", Disassembler::GetGPRName(i), cpu.m_gprs[i]);
		DrawText(registerX+85, 2*margin+(i+1)*15, g_gprsChanged[i+16]?red:black, white, "%2s=%08X", Disassembler::GetGPRName(i+16), cpu.m_gprs[i+16]);
	}
	DrawText(registerX, 3*margin+(17)*15, g_hiChanged?red:black, white, "HI=%08X", cpu.m_hi);
	DrawText(registerX+85, 3*margin+(17)*15, g_loChanged?red:black, white, "LO=%08X", cpu.m_lo);

	SDL_Flip(g_screen);
}

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage: srt9k <romfile>\n");
		return 1;
	}

	memset(&g_gprsChanged, 0, sizeof(g_gprsChanged));

	if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

	if(TTF_Init() < 0)
	{
		fprintf(stderr, "Couldn't initialize SDL_ttf: %s\n", TTF_GetError());
		return 1;
	}

	const char* fontFile = "lucon.ttf";
	g_font = TTF_OpenFont(fontFile, 12);
	if(!g_font)
	{
		fprintf(stderr, "Couldn't load font %s: %s\n", fontFile, TTF_GetError());
		return 1;
	}

	g_screen = SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE);
	if(!g_screen)
	{
		fprintf(stderr, "Couldn't set video mode: %s\n", SDL_GetError());
		return 1;
	}

	const char* title = "SHilRavTron 9000";
	SDL_WM_SetCaption(title,title);
	SDL_EnableKeyRepeat(500, 50);

	Memory* mem = new Memory();
	char* romFilename = argv[1];
	FILE* fp = fopen(romFilename, "rb");
	if(!fp)
	{
		printf("Cannot open ROM file %s: %s\n", romFilename, strerror(errno));
		return 1;
	}

	size_t bytesRead = 0;
	int32_t addr=0;
	for(;;)
	{
		uint8_t buf[4096];
		size_t read = fread(buf, 1, 4096, fp);

		for(size_t i=0; i<read; ++i)
		{
			if(!mem->WriteByte(addr+i, buf[i]))
				break;
		}

		bytesRead += read;

		addr += read;

		if(read < 4096)
			break;		
	}
	fclose(fp);

	CPU cpu(mem);
	CPU lastState(cpu);

	DrawScreen(cpu);

	uint32_t ticks = 0;
	uint32_t millionExecuted = 0;

	SDL_Event evt;
	bool done = false;
	while(!done && SDL_WaitEvent(&evt))
	{
		switch(evt.type)
		{
		case SDL_KEYDOWN:
			if(evt.key.keysym.sym == SDLK_ESCAPE)
				done = true;
			else
			{			
				//while(1) {
				uint32_t start = SDL_GetTicks();
				/*for(int i=0; i<1000000; ++i)*/ cpu.Step(1000000);
				uint32_t end = SDL_GetTicks();
				ticks += (end-start);
				millionExecuted++;
				//printf("MIPS: %f\n", millionExecuted * 1000.0f / (ticks));
				FindRegisterChanges(lastState, cpu);
				lastState = CPU(cpu);
				//}
				//DrawScreen(cpu);
			}
			break;

		case SDL_QUIT:
			done = true;
			break;
		}
	}

	CPU::SaveStatistics();

	SDL_Quit();
	TTF_Quit();
	return 0;
}
