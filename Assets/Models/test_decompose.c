// tcc test_decompose.c -lOpenGL32 -lkernel32 -luser32 -lgdi32 -run
// "C:\Program Files\mingw-w64\x86_64-8.1.0-win32-seh-rt_v6-rev0\mingw64\bin" test_decompose.c -lOpenGL32 -lkernel32 -luser32 -lgdi32
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
#define ERROR_IS_RMS

float tomatch[MSIZEW*MSIZEH];
uint32_t calout[MSIZEW*MSIZEH]; // Visualization
float summap[MSIZEW*MSIZEH];
float errmap[MSIZEW*MSIZEH];

int lastx;
int lasty;
int downness;
void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) {  lastx = x; lasty = y; downness = mask; }
void HandleDestroy() { }


float rrnd()
{
	return ((rand()%10000)-4999.5)/5000.;
}
#define MAXTAPS 4

// Taps are:
//  X = x location
//  Y = y locaiton
//  Z = LOD (0 = on 128x128 map)
//  W = Intensity
float SetTAPS[4*MAXTAPS];
float SetError;

void HandleMip( int miplo, float intensity, float fx, float fy, float * summap )
{
	// Size, in pixels of a sample
	int pxsize = 1<<miplo;
	float localintensity = intensity;// / (pxsize*pxsize);
	
	if( fx < pxsize/2 ) fx = pxsize/2;
	if( fy < pxsize/2 ) fy = pxsize/2;
	if( fx > MSIZEW-pxsize/2 ) fx = MSIZEW-pxsize/2;
	if( fy > MSIZEH-pxsize/2 ) fy = MSIZEH-pxsize/2;
	
	int lxlo = fx / pxsize - 0.5;
	int lxhi = fx / pxsize + 0.5;
	int lylo = fy / pxsize - 0.5;
	int lyhi = fy / pxsize + 0.5;
	float dfhix = fx / pxsize - lxlo - 0.5;
	float dfhiy = fy / pxsize - lylo - 0.5;
	float dflox = 1.0-dfhix;
	float dfloy = 1.0-dfhiy;
	
	lxlo *= pxsize;
	lylo *= pxsize;
	lxhi *= pxsize;
	lyhi *= pxsize;
	
	//printf(" %d %d %d %d %f %d\n", lxlo, lxhi, lylo, lyhi, intensity, miplo );
	
	int x, y;
	for( y = 0; y < pxsize; y++ )
	for( x = 0; x < pxsize; x++ )
	{
		int lxo = lxlo + x;
		int lyo = lylo + y;
		summap[lxo+lyo*MSIZEW] += localintensity * dflox * dfloy;
		lxo = lxhi + x;
		lyo = lylo + y;
		summap[lxo+lyo*MSIZEW] += localintensity * dfhix * dfloy;
		lxo = lxlo + x;
		lyo = lyhi + y;
		summap[lxo+lyo*MSIZEW] += localintensity * dflox * dfhiy;
		lxo = lxhi + x;
		lyo = lyhi + y;
		summap[lxo+lyo*MSIZEW] += localintensity * dfhix * dfhiy;
	}
}

float CalculateTestFromTaps( float * testtaps )
{
	// Returns sum error, not RMS.
	
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
		// Correct faults.
		if( tt[3] < 0 ) tt[3] = 0;
		if( tt[2] < 0 ) tt[2] = 0;
		if( tt[2] > MIPLEVELS ) tt[2] = MIPLEVELS;
		if( tt[0] < 0 ) tt[0] = 0;
		if( tt[1] < 0 ) tt[1] = 0;
		if( tt[0] > MSIZEW-1 ) tt[0] = MSIZEW-1;
		if( tt[1] > MSIZEW-1 ) tt[1] = MSIZEW-1;
		float mip = tt[2];
		int miplo = (int)mip;
		int miphi = miplo+1;
		float mip_porportion_hi = mip - miplo;
		float mip_porportion_lo = 1.0 - mip_porportion_hi;
		
		// First low-mip (highest res)
		HandleMip( miplo, mip_porportion_lo * tt[3], tt[0], tt[1], summap );
		HandleMip( miphi, mip_porportion_hi * tt[3], tt[0], tt[1], summap );
		
		// Need to pick both levels.
		//XXX PICK UP HERE
	}

	float sumerror = 0;
	for( y = 0; y < MSIZEH; y++ )
	{
		for( x = 0; x < MSIZEW; x++ )
		{
			float diff = tomatch[y*MSIZEW+x] - summap[y*MSIZEW+x];
#ifdef ERROR_IS_RMS
			diff = diff*diff;
#else
			if( diff < 0 ) diff = -diff;
#endif
			sumerror += diff;
			errmap[y*MSIZEW+x] = diff*30;
		}
	}
#ifdef ERROR_IS_RMS
	sumerror = sqrtf( sumerror );
#endif
	return sumerror;
}

float absf( float f )
{
	return (f<0)?-f:f;
}

void GenFromErrMap( float * TargTaps )
{
	int x, y;
	float avgx = 0;
	float avgy = 0;
	float sum = 0;
	for( y = 0; y < MSIZEH; y++ )
	{
		for( x = 0; x < MSIZEW; x++ )
		{
			float m = errmap[y*MSIZEW+x];
			avgx += x*m;
			avgy += y*m;
			sum += m;
		}
	}
	avgx /= sum;
	avgy /= sum;
	
	float stddev = 0;
	for( y = 0; y < MSIZEH; y++ )
	{
		for( x = 0; x < MSIZEW; x++ )
		{
			float m = errmap[y*MSIZEW+x];
			stddev += sqrt( ((x-avgx)*(x-avgx))+((y-avgy)*(y-avgy))*m );
		}
	}
	
	printf( "%f %f %f\n", avgx, avgy, stddev );

	TargTaps[0] = avgx;
	TargTaps[1] = avgy;
	TargTaps[2] = MIPLEVELS-1;
	TargTaps[3] = 1;
}

int main()
{
	CNFGSetup( "bakelppv", 1024, 768 );

	FILE * f = fopen( "test_decompose.dat", "rb" );
	fread( tomatch, MSIZEW*MSIZEH*4, 1, f );
	fclose( f );

	SetError = 1e20;

	SetTAPS[0] = MSIZEW/2.0;
	SetTAPS[1] = MSIZEH/2.0;
	SetTAPS[2] = 0.0;
	SetTAPS[3] = 0;
	SetTAPS[4] = MSIZEW/2.0;
	SetTAPS[5] = MSIZEH/2.0;
	SetTAPS[6] = 0.0;
	SetTAPS[7] = 0;

	CalculateTestFromTaps( SetTAPS );
	GenFromErrMap( SetTAPS );

	CalculateTestFromTaps( SetTAPS );
	GenFromErrMap( SetTAPS + 4 );

	CalculateTestFromTaps( SetTAPS );
	GenFromErrMap( SetTAPS + 8 );

	float errorporp = .1;

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
		
		
		
		
		
		CNFGBlitImage( calout, 0, 0, MSIZEW, MSIZEH );
		
		float TestTAPS[4*MAXTAPS];

		if( lastx < 128 && lasty < 128 )
		{
			if( downness & 1 )
			{
				SetTAPS[4] = lastx;
				SetTAPS[5] = lasty;
				SetTAPS[6] = 3;
				SetError = 1e20;
			}

			if( downness & 2 )
			{
				SetTAPS[0] = lastx;
				SetTAPS[1] = lasty;
				SetTAPS[2] = 3;
				SetError = 1e20;
			}
		}
		else if( lastx < 128 )
		{
			if( downness ) errorporp = (lasty - 128)/256.0;
		}

		memcpy( TestTAPS, SetTAPS, sizeof(TestTAPS) );
		
		
		float err = CalculateTestFromTaps( TestTAPS );
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
		
		
		CNFGBlitImage( calout, 256, 0, MSIZEW, MSIZEH );

		float intenjump1 = rrnd()*errorporp;
		TestTAPS[0] += intenjump1*rrnd()*24;
		TestTAPS[1] += intenjump1*rrnd()*24;
		TestTAPS[2] += intenjump1*rrnd()*(MIPLEVELS/2.0);
		TestTAPS[3] += intenjump1*rrnd()*.5;

		err = CalculateTestFromTaps( TestTAPS );
		if( err < SetError )
		{
			memcpy( SetTAPS, TestTAPS, sizeof( TestTAPS ) );
			SetError = err;
		}
		
		float intenjump2 = rrnd()*errorporp;
		TestTAPS[4] += intenjump2*rrnd()*24;
		TestTAPS[5] += intenjump2*rrnd()*24;
		TestTAPS[6] += intenjump2*rrnd()*(MIPLEVELS/2.0);
		TestTAPS[7] += intenjump2*rrnd()*.2;

		err = CalculateTestFromTaps( TestTAPS );
		if( err < SetError )
		{
			memcpy( SetTAPS, TestTAPS, sizeof( TestTAPS ) );
			SetError = err;
		}



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
		
		CNFGBlitImage( calout, 128, 0, MSIZEW, MSIZEH );
		
		
		
		for( y = 0; y < MSIZEH; y++ )
		{
			for( x = 0; x < MSIZEW; x++ )
			{
				float tm = errmap[y*MSIZEW+x];
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

		//Display the image and wait for time to display next frame.
		CNFGSwapBuffers();		
	}
	
	return 0;
}

