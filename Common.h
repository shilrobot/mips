#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef WIN32
	#include "SDL.h"
	#include "SDL_ttf.h"
#elif __APPLE__
	#include "SDL/SDL.h"
	#include "SDL_ttf/SDL_ttf.h"
#else
	#include "SDL/SDL.h"
	#include "SDL/SDL_ttf.h"
#endif

#include <cassert>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#ifdef _MSC_VER
	#define snprintf _snprintf
#endif


#endif // __COMMON_H__
