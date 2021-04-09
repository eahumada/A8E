/********************************************************************
*
*
*
* Atari 800 XL Emulator
*
* (c) 2004 Sascha Springer
*
*
*
*
********************************************************************/

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include "6502.h"
#include "AtariIo.h"

_6502_Context_t *pAtariContext;
SDL_Event tEvent;
SDL_Surface *pScreenSurface = NULL;
SDL_Surface *pDisplaySurface = NULL;
u8 cTurboFlag = 0;
u32 lLastTicks = 0;
u32 lCounter;
u8 cDisassembleFlag = 0;
u64 llCycles = CYCLES_PER_LINE * LINES_PER_SCREEN_PAL;
u32 lMode = 0;
char *pDiskFileName = "assets/d1.atr";
u32 lScreenWidth = 0;
u32 lScreenHeight = 0;
u32 lIndex;
u8 cUseOpenGl = 0;
u32 lSdlFlags = 0;
unsigned int iTextureId;

/********************************************************************
*
*
* Funktionen
*
*
********************************************************************/

static void draw() {
    if(cDisassembleFlag)
    {
        lCounter = CYCLES_PER_LINE * LINES_PER_SCREEN_PAL / 3;
        
        while(lCounter)
        {
            _6502_Run(pAtariContext, pAtariContext->llCycleCounter + 1);
            
            _6502_Status(pAtariContext);
            printf(" ");
            _6502_DisassembleLive(pAtariContext, pAtariContext->tCpu.pc);
            
            lCounter--;
        }
    }
    else
    {

        _6502_Run(pAtariContext, llCycles);
        
        llCycles += CYCLES_PER_LINE * LINES_PER_SCREEN_PAL;

    }
    
    if(cUseOpenGl)
    {
      
        AtariIoDrawScreen(pAtariContext, pDisplaySurface, 336, 240);
                       
        glColor3ub(255, 255, 255);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, iTextureId);
        
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, pDisplaySurface->pitch / pDisplaySurface->format->BytesPerPixel);
#endif
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, pDisplaySurface->pixels);
        //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 512, 512, GL_RGBA, GL_UNSIGNED_BYTE, pDisplaySurface->pixels);
        
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif

        glBegin(GL_QUADS);
        
        glTexCoord2f(0, 0);
        glVertex2i(0, 0);
        
        glTexCoord2f(1, 0);
        glVertex2i(lScreenWidth * 512 / 336, 0);
        
        glTexCoord2f(1, 1);
        glVertex2i(lScreenWidth * 512 / 336, lScreenHeight * 512 / 240);
        
        glTexCoord2f(0, 1);
        glVertex2i(0, lScreenHeight * 512 / 240);
        
        glEnd();

        SDL_GL_SwapBuffers();

    }
    else
    {
        //if (SDL_MUSTLOCK(pScreenSurface)) SDL_LockSurface(pScreenSurface);
        
        //SDL_FillRect(pScreenSurface, NULL, SDL_MapRGBA(pSurface->format, 0, 0, 0, 255));

        
        //for (int i = 1; i < 100; i++) {
        //  for (int j = 1; j < 100; j++) {
        //    int alpha = 0;
        //*((Uint32*)pSurface->pixels + i * lScreenWidth + j) = SDL_MapRGBA(pSurface->format, i, j, i, alpha);
        //  }
        //}
        
        // Lets make some noise to the SDL Atari Surface before copy it to the screen surface
        SDL_Surface *pSurface = ((IoData_t *)pAtariContext->pIoData)->tVideoData.pSdlAtariSurface;

        if (SDL_MUSTLOCK(pSurface)) SDL_LockSurface(pSurface);

        Uint8 * pixels = pSurface->pixels;
        
        for (int i=4000; i < 20000; i++) {
            char randomByte = rand() % 255;
            pixels[i] = randomByte;
        }

        if (SDL_MUSTLOCK(pSurface)) SDL_UnlockSurface(pSurface);

        
        AtariIoDrawScreen(pAtariContext, pScreenSurface, lScreenWidth, lScreenHeight);
        
        /*

        Uint8 * pixels = pScreenSurface->pixels;
        
        for (int i=0; i < 1048576; i++) {
            char randomByte = rand() % 255;
            pixels[i] = randomByte;
        }
         */
               
        SDL_Flip(pScreenSurface);

    }
    
    while(SDL_PollEvent(&tEvent))
    {
        if(tEvent.type == SDL_QUIT)
            goto Exit;
        
        if(tEvent.type == SDL_KEYDOWN)
        {
            if(tEvent.key.keysym.sym == SDLK_ESCAPE)
                goto Exit;
#ifdef ENABLE_VERBOSE_DEBUGGING
            if(tEvent.key.keysym.sym == SDLK_F12)
                cDisassembleFlag = 1;
#endif
        }
        
        AtariIoKeyboardEvent(pAtariContext, &tEvent.key);
    }
Exit:
    return;
}

static void main_loop() {
    while(1)
    {
        draw();

        while(SDL_GetTicks() - lLastTicks < 20)
            SDL_Delay(1);
        
        lLastTicks = SDL_GetTicks();
    }

    return;
}


// Our "main loop" function. This callback receives the current time as
// reported by the browser, and the user data we provide in the call to
// emscripten_request_animation_frame_loop().
#ifdef __EMSCRIPTEN__
EM_BOOL one_iter(double time, void* userData) {
  // Can render to the screen here, etc.
  puts("one iteration");
    draw();
  // Return true to keep the loop running.
  return EM_FALSE;
}
#endif

int main(int argc, char *argv[])
{
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER); //
    
    //SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 ); // *new*
    
	for(lIndex = 1; lIndex < argc; lIndex++)
	{
		if(argv[lIndex][0] == '-')
		{
			switch(argv[lIndex][1])
			{
			case 'o':
			case 'O':
				cUseOpenGl = 1;
			
				break;
				
			case 'b':
			case 'B':
				lMode = 1;
			
				break;
				
			case 'f':
			case 'F':
				lSdlFlags |= SDL_FULLSCREEN;
			
				break;
				
			default:
				break;
			}
		}
		else
		{
			pDiskFileName = argv[lIndex];
		}
	}

	if(cUseOpenGl)
	{
		lSdlFlags |= SDL_OPENGL;
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		
		if(lScreenWidth == 0 || lScreenHeight == 0)
		{
			lScreenWidth = 640;
			lScreenHeight = 480;
		}
	}
	else
	{
		lSdlFlags |= SDL_HWSURFACE | SDL_DOUBLEBUF;

		if(lSdlFlags & SDL_FULLSCREEN)
			lScreenWidth = 320;
		else
			lScreenWidth = 336;

		lScreenHeight = 240;
	}

	pScreenSurface = SDL_SetVideoMode(lScreenWidth, lScreenHeight, 8, lSdlFlags);
	
	if(pScreenSurface == NULL)
	{
		printf("SDL_SetVideoMode() failed!\n");

		exit(-1);
	}
    
#if __EMSCRIPTEN__
    EM_ASM(
           SDL.defaults.copyOnLock = false;
           SDL.defaults.discardOnLock = false;
           SDL.defaults.opaqueFrontBuffer = true;
           Module.screenIsReadOnly = false;
    );
#endif
    
	if(cUseOpenGl)
	{
        pDisplaySurface = SDL_CreateRGBSurface(
			SDL_SWSURFACE, 512, 512, 32,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#else
			0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#endif
        
		glGenTextures(1, &iTextureId);
		glBindTexture(GL_TEXTURE_2D, iTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glViewport(0, 0, lScreenWidth, lScreenHeight);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glOrtho(0, lScreenWidth, lScreenHeight, 0, -1.0, 1.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

    }
	
	SDL_WM_SetCaption(APPLICATION_CAPTION, NULL);

	_6502_Init();
	
	pAtariContext = _6502_Open();
	AtariIoOpen(pAtariContext, lMode, pDiskFileName);
	
	_6502_Reset(pAtariContext);


#ifdef __EMSCRIPTEN__
    // Receives a function to call and some user data to provide it.
    emscripten_request_animation_frame_loop(one_iter, 0);
    printf("emscripten_set_main_loop\n");
    emscripten_set_main_loop(draw, 50, 1);
#else
    printf("main_loop");
    main_loop();
#endif

	AtariIoClose(pAtariContext);
	_6502_Close(pAtariContext);

	return 0;
}

