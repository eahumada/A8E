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
#include <SDL/SDL_image.h>

#ifndef USE_REGAL
#include "SDL/SDL_opengl.h"
#else
#include "GL/Regal.h"
#endif

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
char *pDiskFileName = "assets/D1.ATR";
u32 lScreenWidth = 0;
u32 lScreenHeight = 0;
u32 lIndex;
u8 cUseOpenGl = 0;
u32 lSdlFlags = 0;

int cExit=0;

unsigned int iTextureId;

GLuint texture=0; // Texture object handle

void initOpenGL() {
    // Set the OpenGL state after creating the context with SDL_SetVideoMode
    glClearColor( 0, 0, 0, 0 );

    glViewport( 0, 0, lScreenWidth, lScreenHeight );
    glMatrixMode( GL_PROJECTION );
    glPushMatrix(); // just for testing
    glLoadIdentity();
    glOrtho( 0, lScreenWidth, lScreenHeight, 0, -1, 1 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();


    pDisplaySurface = SDL_CreateRGBSurface(
                                           SDL_SWSURFACE, 512, 512, 32,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#else
    0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#endif
    
}

static GLuint create_texture() {

    SDL_Surface *surface = NULL; // Gives us the information to make the texture
    
    if ( (surface = IMG_Load("assets/screenshot.png")) ) {
        
        // Check that the image's width is a power of 2
        if ( (surface->w & (surface->w - 1)) != 0 ) {
            printf("warning: image.bmp's width is not a power of 2\n");
        }
        
        // Also check if the height is a power of 2
        if ( (surface->h & (surface->h - 1)) != 0 ) {
            printf("warning: image.bmp's height is not a power of 2\n");
        }
        
        // Have OpenGL generate a texture object handle for us
        glGenTextures( 1, &texture );
        
        // Bind the texture object
        glBindTexture( GL_TEXTURE_2D, texture );
        
        // Set the texture's stretching properties
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        //SDL_LockSurface(surface);
        
        // Add some greyness
        memset(surface->pixels, 0x66, surface->w*surface->h);
        
        // Edit the texture object's image data using the information SDL_Surface gives us
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels );
        
        //SDL_UnlockSurface(surface);
    }
    else {
        printf("SDL could not load image.bmp: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Free the SDL_Surface only if it was successfully created
    if ( surface ) {
        SDL_FreeSurface( surface );
    }
    return texture;
}

int draw_textures()
{
    glEnable( GL_TEXTURE_2D ); // Needed when we're using the fixed-function pipeline.
    
    // Load the OpenGL texture
    if(texture==0) {
        texture=create_texture();
    }

    // Bind the texture to which subsequent calls refer to
    glBindTexture( GL_TEXTURE_2D, texture );

    glBegin( GL_QUADS );
        glTexCoord2f( 0, 0 ); glVertex3f( 410, 150, 0 );
        glTexCoord2f( 1, 0 ); glVertex3f( 600, 110, 0 );
        glTexCoord2f( 1, 1   ); glVertex3f( 630, 200, 0 );
        glTexCoord2f( 0, 1 ); glVertex3f( 310, 250, 0 );
    glEnd();

    // test calls to glVertex4f
    // Result should be equivalent as to glVertex3f( x/w, y/w, z/w )
    glBegin( GL_TRIANGLE_STRIP );
        glTexCoord2i( 0, 0 ); glVertex3f( 100, 300, 0 );
        glTexCoord2i( 1, 0 ); glVertex3f( 300, 300, 0 );
        glTexCoord2i( 1, 1 ); glVertex3f( 300, 400, 0 );
        glTexCoord2i( 0, 1 ); glVertex3f( 500, 410, 0 );
    glEnd();

    glDisable(GL_TEXTURE_2D);

    glColor3ub(90, 255, 255);
    glBegin( GL_QUADS );
        glVertex3f( 10, 410, 0 );
        glVertex3f( 300, 410, 0 );
        glVertex3f( 300, 480, 0 );
        glVertex3f( 10, 470, 0 );
    glEnd();

    glBegin( GL_QUADS );
        glColor3f(1.0, 0, 1.0);   glVertex3f( 410, 410, 0 );
        glColor3f(0, 1.0, 0);     glVertex3f( 600, 410, 0 );
        glColor3f(0, 0, 1.0);     glVertex3f( 600, 480, 0 );
        glColor3f(1.0, 1.0, 1.0); glVertex3f( 410, 470, 0 );
    glEnd();
    
    // Now we can delete the OpenGL texture and close down SDL
    //glDeleteTextures( 1, &texture );

    return 0;

}

static void a8e_end_draw(SDL_Surface *pSurface) {

    if (!cUseOpenGl && SDL_MUSTLOCK(pSurface))
        SDL_LockSurface(pSurface);
    
    // add some noise to test
    
     Uint8 * pixels = pSurface->pixels;
    
     for (int i=5000; i < 25000; i++) {
        char randomByte = rand() % 255;
        pixels[i] = randomByte;
     }
    
    if (!cUseOpenGl && SDL_MUSTLOCK(pSurface))
        SDL_UnlockSurface(pSurface);


}

void drawTexture(GLuint texture) {

    // Bind the texture to which subsequent calls refer to
    glBindTexture( GL_TEXTURE_2D, texture );

    glBegin( GL_QUADS );
        glTexCoord2f( 0, 0.5 ); glVertex3f( 410, 10, 0 );
        glTexCoord2f( 1, 0.5 ); glVertex3f( 600, 10, 0 );
        glTexCoord2f( 1, 1   ); glVertex3f( 630, 200, 0 );
        glTexCoord2f( 0.5, 1 ); glVertex3f( 310, 250, 0 );
    glEnd();
}


static void a8e_opengl_draw() {

    AtariIoDrawScreen(pAtariContext, pDisplaySurface, 336, 240);
    
    a8e_end_draw(pDisplaySurface);
    
    glColor3ub(255, 255, 255);
    
    glEnable(GL_TEXTURE_2D);

    glGenTextures(1, &iTextureId);
    
    glBindTexture(GL_TEXTURE_2D, iTextureId);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, pDisplaySurface->pitch / pDisplaySurface->format->BytesPerPixel);
#endif
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, pDisplaySurface->pixels);
    
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    
    // Free the SDL_Surface only if it was successfully created
    //if ( pDisplaySurface ) {
    //    SDL_FreeSurface( pDisplaySurface );
    //}
    
    glBegin( GL_QUADS );
    
    glTexCoord2f( 0, 0 ); glVertex2f( 0, 0 );
    glTexCoord2f( 1, 0 ); glVertex2f( lScreenWidth * 512 / 336, 0 );
    glTexCoord2f( 1, 1  ); glVertex2f( lScreenWidth * 512 / 336, lScreenHeight * 512 / 240 );
    glTexCoord2f( 0, 1 ); glVertex2f(0, lScreenHeight * 512 / 240 );
    
    glEnd();
    
    glDisable(GL_TEXTURE_2D);

    // Now we can delete the OpenGL texture
    glDeleteTextures( 1, &iTextureId );
}

/********************************************************************
*
*
* Funktionen
*
*
********************************************************************/

void draw() {
    
    SDL_Surface* pSdlAtariSurface=((IoData_t *)pAtariContext->pIoData)->tVideoData.pSdlAtariSurface;
    
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
        
    if(1) {
        if(cUseOpenGl)
        {
            //glClearColor( 0, 0, 0, 0 );
            // Clear the screen before drawing

            glClear( GL_COLOR_BUFFER_BIT );
            a8e_opengl_draw();
            draw_textures();
            SDL_GL_SwapBuffers();

        }
        else
        {
            // Lets make some noise into the SDL Atari Surface before copy it to the screen surface

            a8e_end_draw(pSdlAtariSurface);
            
            if(pSdlAtariSurface!=pScreenSurface) {
                AtariIoDrawScreen(pAtariContext, pScreenSurface, lScreenWidth, lScreenHeight);
            }

            SDL_Flip(pScreenSurface);
            


        }
    }
    
    while(SDL_PollEvent(&tEvent))
    {
        if(tEvent.type == SDL_QUIT)
		{
#ifdef __EMSCRIPTEN__
			emscripten_cancel_main_loop();
#else
            cExit=1;
#endif
            return;
        }

        if(tEvent.type == SDL_KEYDOWN)
        {
			if (tEvent.key.keysym.sym == SDLK_F10)
				cTurboFlag = 1;

            if(tEvent.key.keysym.sym == SDLK_ESCAPE) {
#ifdef __EMSCRIPTEN__
				emscripten_cancel_main_loop();
				return;
#else
                SDL_Quit();
                cExit=1;
                return;
#endif
			}
#ifdef ENABLE_VERBOSE_DEBUGGING
            if(tEvent.key.keysym.sym == SDLK_F12)
                cDisassembleFlag = 1;
#endif
        }
        
		if (tEvent.type == SDL_KEYUP)
			if (tEvent.key.keysym.sym == SDLK_F10)
				cTurboFlag = 0;

        AtariIoKeyboardEvent(pAtariContext, &tEvent.key);
    }
    return;
}

static void main_loop() {
    while(!cExit)
    {
        draw();
        
		if(!cTurboFlag) {
            while(SDL_GetTicks() - lLastTicks < 20)
                SDL_Delay(1);

            lLastTicks = SDL_GetTicks();
        }

    }

    return;
}

int main(int argc, char *argv[])
{

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

    // SDL initialization
    if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0 ) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

	if(cUseOpenGl)
	{
        lSdlFlags |= SDL_OPENGL | SDL_DOUBLEBUF; // |  SDL_HWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF;
        
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);
        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
        
		if(lScreenWidth == 0 || lScreenHeight == 0)
		{
			lScreenWidth = 640;
			lScreenHeight = 480;
		}
	}
	else
	{
		lSdlFlags |= SDL_HWSURFACE  | SDL_DOUBLEBUF; // | SDL_HWPALETTE

		if(lSdlFlags & SDL_FULLSCREEN)
			lScreenWidth = 320;
		else
            lScreenWidth = 336; // 456; // 336;

        lScreenHeight = 240; // 312; // 240;

#if __EMSCRIPTEN__
        lScreenWidth = 456; // 336;
        lScreenHeight = 240; // 240;
#endif
	}

    pScreenSurface = SDL_SetVideoMode( lScreenWidth, lScreenHeight, 32, (Uint32)lSdlFlags ); // *changed*

    if ( !pScreenSurface ) {
        printf("Unable to set video mode: %s\n", SDL_GetError());
        return -1;
    }

    printf("pScreenSurface");
    printf("BPP is %d\n", pScreenSurface->format->BitsPerPixel);

    
#if __EMSCRIPTEN__
    if(!cUseOpenGl) {
        EM_ASM(
               SDL.defaults.copyOnLock = false; // True for openGL
               SDL.defaults.discardOnLock = false;
               SDL.defaults.opaqueFrontBuffer = true;
               Module.screenIsReadOnly = true;
        );
    }
    else
    {
        EM_ASM(
               SDL.defaults.copyOnLock = true; // True for openGL
               SDL.defaults.discardOnLock = false;
               SDL.defaults.opaqueFrontBuffer = true;
               Module.screenIsReadOnly = true;
               Module.GL_MAX_TEXTURE_IMAGE_UNITS= 16;
        );
    }
#endif

	if(cUseOpenGl)
	{
        initOpenGL();
    }
    
	SDL_WM_SetCaption(APPLICATION_CAPTION, NULL);

	_6502_Init();
	
	pAtariContext = _6502_Open();
	
    AtariIoOpen(pAtariContext, lMode, pDiskFileName);
    
	_6502_Reset(pAtariContext);


#ifdef __EMSCRIPTEN__
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

