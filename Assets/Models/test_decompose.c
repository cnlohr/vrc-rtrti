// tcc test_decompose.c -lOpenGL32 -lkernel32 -luser32 -lgdi32

#include <stdio.h>
#include <math.h>

#include "os_generic.h"

#define CNFG_IMPLEMENTATION
#define CNFGOGL
float sqrtf( float f ) { return sqrt( f ); }
#include "rawdraw_sf.h"

#define MSIZEW 128
#define MSIZEH 128

float tomatch[MSIZEW*MSIZEH];
float summap[MSIZEW*MSIZEH];
uint32_t calout[MSIZEW*MSIZEH];

void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
void HandleDestroy() { }


#define MAXTAPS 6
// Taps are:
//  X = x location
//  Y = y locaiton
//  Z = LOD (0 = on 128x128 map)
//  W = Intensity
float SetTAPS[4*MAXTAPS];
float TestTAPS[4*MAXTAPS];

float CalculateTestFromTaps( float * testtaps )
{
	// Returns error.
	
	int x, y;
	for( y = 0; y < MSIZEH; y++ )
	{
		for( x = 0; x < MSIZEW; x++ )
		{
			summap[y*MSIZEW+x] = 0;
		}
	}
	
	//Now, we have to "sample" the taps
	int i;
	for( i = 0; i < MAXTAPS; i++ )
	{
		float * tt = testtaps + 4*i;
		// Need to pick both levels.
		//XXX PICK UP HERE
	}
}

int main()
{
	CNFGSetup( "bakelppv", 1024, 768 );

	FILE * f = fopen( "test_decompose.dat", "rb" );
	fread( tomatch, MSIZEW*MSIZEH*4, 1, f );
	fclose( f );

	while(1)
	{
		short w, h;
		CNFGClearFrame();
		CNFGHandleInput();
		CNFGGetDimensions( &w, &h );
		
		
		int x, y;
		for( y = 0; y < MSIZEH; y++ )
		{
			for( x = 0; x < MSIZEW; x++ )
			{
				float tm = tomatch[y*MSIZEW+x];
				int r = tm*255;
				if( r < 0 ) r = 0; if( r > 255 ) r = 255;
				calout[y*MSIZEW+x] = 0xff | (r<<24)| (r<<16)|(r<<8);
			}
		}
		
		
		CNFGBlitImage( calout, 150, 30, MSIZEW, MSIZEH );

		for( y = 0; y < MSIZEH; y++ )
		{
			for( x = 0; x < MSIZEW; x++ )
			{
				float tm = summap[y*MSIZEW+x];
				int r = tm*255;
				if( r < 0 ) r = 0; if( r > 255 ) r = 255;
				calout[y*MSIZEW+x] = 0xff | (r<<24)| (r<<16)|(r<<8);
			}
		}
		
		
		CNFGBlitImage( calout, 290, 30, MSIZEW, MSIZEH );
		
		//Change color to white.
		CNFGColor( 0xffffffff ); 

		CNFGPenX = 1; CNFGPenY = 1;
		CNFGDrawText( "Hello, World", 2 );

		//Display the image and wait for time to display next frame.
		CNFGSwapBuffers();		
	}
	
	return 0;
}

