// tcc test_decompose.c -lOpenGL32 -lkernel32 -luser32 -lgdi32 -run
// "C:\Program Files\mingw-w64\x86_64-8.1.0-win32-seh-rt_v6-rev0\mingw64\bin\gcc" test_decompose.c -lOpenGL32 -lkernel32 -luser32 -lgdi32 -O4 -o test_decompose.exe
#include <stdio.h>
#include <math.h>

#include "os_generic.h"

#define CNFG_IMPLEMENTATION
#define CNFGOGL
float sqrtf( float f ) { return sqrt( f ); }
#include "rawdraw_sf.h"

#define MSIZEW 128
#define MSIZEH 128
#define MIPLEVELS 7

//This is experimentally bad.
//#define ERROR_IS_RMS


int lastx;
int lasty;
int downness;
void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) {  lastx = x; lasty = y; downness = mask; }
void HandleDestroy() { }

#include "decompose.h"

// Taps are:
//  X = x location
//  Y = y locaiton
//  Z = LOD (0 = on 128x128 map)
//  W = Intensity
float SetTAPS[4*MAXTAPS];
float SetError;


int main()
{
	uint32_t calout[MSIZEW*MSIZEH]; // Visualization
	float tomatch[MSIZEW*MSIZEH];
	float summap[MSIZEW*MSIZEH];
	float errmap[MSIZEW*MSIZEH];
	float errmapSIGN[MSIZEW*MSIZEH];


	int x, y;
	CNFGSetup( "bakelppv", 1024, 768 );

	FILE * f = fopen( "test_decompose.dat", "rb" );
	fread( tomatch, MSIZEW*MSIZEH*4, 1, f );
	fclose( f );
	for( y = 0 ; y < MSIZEH; y++ )
		for( x = 0; x < MSIZEW; x++ )
		{
			tomatch[x+y*MSIZEW]/=16384.;
		}
#if 0
	for( y = 0 ; y < MSIZEH; y++ )
		for( x = 0; x < MSIZEW; x++ )
		{
			int lx = x - 0;
			int ly = y - 0;
			if( lx < 32 && lx >= 0 && ly < 32 && ly >= 0)
			//if( 1 )
				tomatch[x+y*MSIZEW] = (1.0/32768);
			else
				tomatch[x+y*MSIZEW] = 0;
		}
#endif

	PerformDecompose( SetTAPS, tomatch, &SetError );

	SetError = CalculateTestFromTaps( SetTAPS, tomatch, summap, errmapSIGN, errmap );

	float errorporp = .1;

	double Last = OGGetAbsoluteTime();
	while(1)
	{
		short w, h;
		CNFGClearFrame();
		CNFGHandleInput();
		CNFGGetDimensions( &w, &h );
		
		double Now = OGGetAbsoluteTime();
		
		int x, y;
		for( y = 0; y < MSIZEH; y++ )
		{
			for( x = 0; x < MSIZEW; x++ )
			{
				float tm = tomatch[y*MSIZEW+x]*MSIZEW*MSIZEH;
				int r = tm*255;
				if( r < 0 ) r = 0; if( r > 255 ) r = 255;
				calout[y*MSIZEW+x] = 0xff | (r<<24)| (r<<16)|(r<<8);
			}
		}
		
		
		CNFGBlitImage( calout, 0, 0, MSIZEW, MSIZEH );
		
		float TestTAPS[4*MAXTAPS];

		if( lastx < 128 && lasty < 128 )
		{
			if( downness & 1 )
			{
				SetTAPS[4] = lastx/(float)MSIZEW;
				SetTAPS[5] = lasty/(float)MSIZEH;
				SetTAPS[6] = 3;
				SetError = 1e20;
			}

			if( downness & 2 )
			{
				SetTAPS[0] = lastx/(float)MSIZEW;
				SetTAPS[1] = lasty/(float)MSIZEH;
				SetTAPS[2] = 3;
				SetError = 1e20;
			}
		}
		else if( lastx < 128 )
		{
			if( downness ) errorporp = (lasty - 128)/256.0;
		}

		memcpy( TestTAPS, SetTAPS, sizeof(TestTAPS) );
		
		
		float err = CalculateTestFromTaps( TestTAPS, tomatch, summap, errmapSIGN, errmap );
		for( y = 0; y < MSIZEH; y++ )
		{
			for( x = 0; x < MSIZEW; x++ )
			{
				float tm = summap[y*MSIZEW+x]*MSIZEW*MSIZEH;
				int r = tm*255;
				if( r < 0 ) r = 0; if( r > 255 ) r = 255;
				calout[y*MSIZEW+x] = 0xff | (r<<24)| (r<<16)|(r<<8);
			}
		}
		
		
		CNFGBlitImage( calout, 256, 0, MSIZEW, MSIZEH );
#if 0
		RunCeleste( 0, tomatch, summap, errmapSIGN, errmap, calout, &SetError );
		RunCeleste( 1, tomatch, summap, errmapSIGN, errmap, calout, &SetError );
		RunCeleste( 2, tomatch, summap, errmapSIGN, errmap, calout, &SetError );
		RunCeleste( 3, tomatch, summap, errmapSIGN, errmap, calout, &SetError );
#endif
#if 0
		float intenjump2 = rrnd()*errorporp;
		TestTAPS[4] += intenjump2*rrnd()*.5;
		TestTAPS[5] += intenjump2*rrnd()*.5;
		TestTAPS[6] += intenjump2*rrnd()*(MIPLEVELS/2.0);
		TestTAPS[7] += intenjump2*rrnd()*.2;

		err = CalculateTestFromTaps( TestTAPS, tomatch, summap, errmapSIGN, errmap );
		if( err < SetError )
		{
			memcpy( SetTAPS, TestTAPS, sizeof( TestTAPS ) );
			SetError = err;
		}
#endif

		err = CalculateTestFromTaps( TestTAPS, tomatch, summap, errmapSIGN, errmap );

		//err = CalculateTestFromTaps( TestTAPS, tomatch, summap, errmapSIGN, errmap );

		for( y = 0; y < MSIZEH; y++ )
		{
			for( x = 0; x < MSIZEW; x++ )
			{
				float tm = summap[y*MSIZEW+x]*MSIZEW*MSIZEH;
				int r = tm*255;
				if( r < 0 ) r = 0; if( r > 255 ) r = 255;
				calout[y*MSIZEW+x] = 0xff | (r<<24)| (r<<16)|(r<<8);
			}
		}
		
		CNFGBlitImage( calout, 128, 0, MSIZEW, MSIZEH );
		
		
		
		for( y = 0; y < MSIZEH; y++ )
		{
			for( x = 0; x < MSIZEW; x++ )
			{
				float tm = errmapSIGN[y*MSIZEW+x]*MSIZEW*MSIZEH*2.0;
				int r = tm*255;
				if( r < 0 ) r = 0; if( r > 255 ) r = 255;
				int g = -tm*255;
				if( g < 0 ) g = 0; if( g > 255 ) g = 255;
				calout[y*MSIZEW+x] = 0xff | (r<<24)| (g<<16)|(0<<8);
			}
		}
		
		CNFGBlitImage( calout, 384, 0, MSIZEW, MSIZEH );

		
		//Change color to white.
		CNFGColor( 0xffffffff ); 

		CNFGPenX = 1; CNFGPenY = 256;
		char cts[256];
		sprintf( cts, "%f %f %f %f\n%f %f %f %f\n%f\n", SetTAPS[0], SetTAPS[1], SetTAPS[2], SetTAPS[3], SetTAPS[4], SetTAPS[5], SetTAPS[6], SetTAPS[7], SetError );
		CNFGDrawText( cts, 2 );


		sprintf( cts, "MXY %d %d\n", lastx, lasty );
		CNFGPenX = 1; CNFGPenY = 300;
		CNFGDrawText( cts, 2 );
		
		static int frames;
		frames++;
		if( Now > Last+1 )
		{
			printf( "%d FPS\n", frames );
			frames = 0;
			Last = Now;
		}
		
		//Display the image and wait for time to display next frame.
		CNFGSwapBuffers();		
	}
	
	return 0;
}

