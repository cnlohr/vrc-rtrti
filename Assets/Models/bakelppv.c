//
// tcc bakelppv.c -lOpenGL32 -lkernel32 -luser32 -lgdi32 -run
//

#include <stdio.h>
#include <math.h>
#include "unitytexturewriter.h"

#include "os_generic.h"

#define CNFG_IMPLEMENTATION
#define CNFGOGL
float sqrtf( float f ) { return sqrt( f ); }
#include "rawdraw_sf.h"

#include "os_generic.h"
#include "decompose.h"
#include "chew.c"

// If running without OpenGL headers, we have to define these.
#ifndef GL_RGBA32F
#define GL_RGBA32F				0x8814
#define GL_R32F                 0x822E
#define GL_FRAMEBUFFER			0x8D40
#define GL_COLOR_ATTACHMENT0	0x8CE0
#define GL_FRAMEBUFFER_COMPLETE	0x8CD5
#define GL_RENDERBUFFER			0x8D41
#define GL_DEPTH_COMPONENT16	0x81A5
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#define GL_DEPTH24_STENCIL8               0x88F0
#define GL_PIXEL_PACK_BUFFER              0x88EB
#define GL_STATIC_READ                    0x88E5
#define GL_READ_ONLY                      0x88B8
#endif

#define RENDERW 256
#define RENDERH 128

#define PASS2W 128
#define PASS2H 128

int quit = 0;

void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
void HandleDestroy() { }

float * frame_to_process = 0;

og_sema_t * sem_ftp;
//XXX TODO TODO PASS IN DATA AND CALL DECOMPOSE.


void * ProcessThread( void * v )
{
	while( !quit )
	{
		OGLockSema( sem_ftp );
		// So, we need to calculate the histograms of the uv for each of the SH.
		// Thought: What if it were a direction and an intensity?
		FILE * f = fopen( "test_decompose.dat", "wb" );
		fwrite( frame_to_process, PASS2W * PASS2H, sizeof(float), f );
		fclose( f );
		
		
		frame_to_process = 0;
	}
}


char * FileToString( const char * fname )
{
	FILE * f = fopen( fname, "rb" );
	if( !f || ferror( f ) ) return 0;
	fseek( f, 0, SEEK_END );
	int len = ftell( f );
	char * ret = malloc( len+1 );
	fseek( f, 0, SEEK_SET );
	fread( ret, len, 1, f );
	ret[len] = 0;
	fclose( f );
	return ret;
}

void CheckShader( const char * vertname, const char * fragname, int slot, int * rendershader )
{
	#define MAXSHADERS 10
	static double slottimes[MAXSHADERS*2];
	double rsvert = slottimes[slot*2+0];
	double rsfrag = slottimes[slot*2+1];
	
	if( OGGetFileTime( vertname ) != rsvert || OGGetFileTime( fragname ) != rsfrag )
	{
		OGUSleep( 100000 );
		printf( "Compiling: %s (%f) %s (%f): %d\n", vertname, slottimes[slot*2+0], fragname, slottimes[slot*2+1], slot );
		slottimes[slot*2+0] = OGGetFileTime( vertname );
		slottimes[slot*2+1] = OGGetFileTime( fragname );
		if( *rendershader >= 0 )
		{
			CNFGglDeleteShader( *rendershader );
		}
		char * vert = FileToString( vertname );
		char * frag = FileToString( fragname );
		GLint r = -1;
		if( vert && frag )
		{
			r = CNFGGLInternalLoadShader( vert, frag );
		}
		*rendershader = (r>=0)?r:-1;				
	}
}

const float base_verts[] = {
	0,0, RENDERW,0, RENDERW,RENDERH,
	0,0, RENDERW,RENDERH, 0,RENDERH, };
const float base_verts_square
[] = {
	0,0, PASS2W,0, PASS2W,PASS2H,
	0,0, PASS2W,PASS2H, 0,PASS2H, };
static const uint8_t base_colors[] = {
	0,0,   255,0,  255,255,
	0,0,  255,255, 0,255 };

float base2_verts[RENDERW*RENDERH*18];

int main()
{
	CNFGSetup( "bakelppv", 1024, 768 );

	GLint lppv_render = -1;
	GLint lppv_render_stage2 = -1;
	GLint debug_blit = -1;
	float * texdata = 0;
	double textime = -1;
	int texwid = 0;
	int texhei = 0;
	GLuint geotex;
	GLuint texS1;
	GLuint fboS1;
	GLuint rboS1;
	GLuint texS2;
	GLuint fboS2;
	GLuint pbo = 0;

	sem_ftp = OGCreateSema();
	chewInit();
	
	glGenBuffers( 1, &pbo );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, pbo );
	glBufferData( GL_PIXEL_PACK_BUFFER, PASS2W*PASS2H*4*4, 0, GL_STATIC_READ );
	
	debug_blit = CNFGGLInternalLoadShader(
		"uniform vec4 xfrm;"
		"attribute vec3 a0;"
		"attribute vec4 a1;"
		"varying vec2 tc;"
		"void main() { gl_Position = vec4( a0.xy*xfrm.xy+xfrm.zw, a0.z, 0.5 ); tc = a1.xy; }",
		
		"varying vec2 tc;"
		"uniform sampler2D tex;"
		"void main() { gl_FragColor = texture2D(tex,tc).rgba; }" );

	
	
	{
		int lx, ly;
		int i = 0;
		int k = 0;
		for( ly = 0; ly < RENDERH; ly++ )
			for( lx = 0; lx < RENDERW; lx++ )
			{
				base2_verts[i++] = 0;
				base2_verts[i++] = 0;
				base2_verts[i++] = k;
				base2_verts[i++] = PASS2W;
				base2_verts[i++] = 0;
				base2_verts[i++] = k;
				base2_verts[i++] = PASS2W;
				base2_verts[i++] = PASS2H;
				base2_verts[i++] = k;
				base2_verts[i++] = 0;
				base2_verts[i++] = 0;
				base2_verts[i++] = k;
				base2_verts[i++] = PASS2W;
				base2_verts[i++] = PASS2H;
				base2_verts[i++] = k;
				base2_verts[i++] = 0;
				base2_verts[i++] = PASS2H;
				base2_verts[i++] = k;
				k++;
			}
	}


	void (*SwapInterval)( int ) = CNFGGetProcAddress("wglSwapIntervalEXT");
	if( SwapInterval ) SwapInterval( 0 );

	glGenTextures( 1, &geotex );	

	glGenTextures( 1, &texS1 );
	glGenFramebuffers( 1, &fboS1 );
	glGenTextures( 1, &texS2 );
	glGenFramebuffers( 1, &fboS2 );


	glBindTexture( GL_TEXTURE_2D, texS1 );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, RENDERW, RENDERH, 0, GL_RGBA, GL_FLOAT, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glBindFramebuffer( GL_FRAMEBUFFER, fboS1 );  
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texS1, 0 );
	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE ) printf( "Framebuffer OK\n" ); else return -1;
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );  
	
	glBindTexture( GL_TEXTURE_2D, texS2 );
	// Oddly, this is a case where we can benefit from a lower bit count on our buffer.
	// GL_R32F is roughly 2x the perf of RGBA32F
	glTexImage2D( GL_TEXTURE_2D, 0, GL_R32F, PASS2W, PASS2H, 0, GL_RGBA, GL_FLOAT, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glBindFramebuffer( GL_FRAMEBUFFER, fboS2 );  
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texS2, 0 );
	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE ) printf( "Framebuffer OK\n" ); else return -1;
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );  

	CNFGglEnableVertexAttribArray(0);
	CNFGglEnableVertexAttribArray(1);
	glDisable(GL_DEPTH_TEST);
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_ONE, GL_ZERO );


	double Now, LastSecond = OGGetAbsoluteTime();
	int FrameCt = 0;
	int FPSCt = 0;
	int pingpong = 0;
	
	int threads = 12;
	int tid = 0;
	for( tid = 0; tid < threads; tid++ )
		OGCreateThread( ProcessThread, 0 );

	while( !quit )
	{
		Now = OGGetAbsoluteTime();
		CNFGBGColor = 0x000080ff; //Dark Blue Background


		if( OGGetFileTime( "geometryimage.asset" ) != textime || texdata == 0 )
		{
			if( texdata ) free( texdata ); texdata = 0;
			textime = OGGetFileTime( "geometryimage.asset" );
			char * ftex = FileToString( "geometryimage.asset" );
			texwid = texhei = 0;
			char * temp = strstr( ftex, "m_Width: " );
			if( temp ) texwid = atoi( temp + 9 );
			temp = strstr( ftex, "m_Height: " );
			if( temp ) texhei = atoi( temp + 10 );
			if( texwid && texhei )
			{
				temp = strstr( ftex, "typelessdata: " );
				if( temp )
				{
					temp+=14;
					int byte = sizeof( float ) * 4 * texwid * texhei;
					texdata = malloc( byte );
					int i;
					for( i = 0; i < byte; i++ )
					{
						char a = temp[i*2+0];
						char b = temp[i*2+1];
						if( !a || !b )
						{
							break;
						}
						if( a >= 'a' ) a = a - 'a' + '0' + 10;
						if( a >= 'A' ) a = a - 'A' + '0' + 10;
						if( b >= 'a' ) b = b - 'a' + '0' + 10;
						if( b >= 'A' ) b = b - 'A' + '0' + 10;
						a -= '0'; b -= '0';
						((uint8_t*)texdata)[i] = (a<<4)|b;
					}
					if( i != byte )
					{
						free( texdata );
						texdata = 0;
					}
				}
			}
			free( ftex );
			if( texdata )
			{
				glBindTexture( GL_TEXTURE_2D, geotex );

				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

				glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, texwid, texhei, 0, GL_RGBA, GL_FLOAT, texdata );
			}
		}

		CheckShader( "lppv_render.vert", "lppv_render.frag", 0, &lppv_render );
		CheckShader( "lppv_render_stage2.vert", "lppv_render_stage2.frag", 1, &lppv_render_stage2 );

		short w, h;
		CNFGClearFrame();
		CNFGHandleInput();
		CNFGGetDimensions( &w, &h );
		CNFGColor( 0xffffffff ); 
		CNFGglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, base_verts);
		CNFGglVertexAttribPointer(1, 2, GL_UNSIGNED_BYTE, GL_TRUE, 0, base_colors);


		if( lppv_render > 0 && lppv_render_stage2 > 0 && texdata )
		{		
			glBindFramebuffer( GL_FRAMEBUFFER, fboS1 );
			glViewport( 0, 0, RENDERW, RENDERH );

			int x = 0; int y = 0;
			CNFGglUseProgram( lppv_render );
			int temp;

			temp = CNFGglGetUniformLocation ( lppv_render, "xfrm" );
			if( temp >= 0 ) CNFGglUniform4f( temp,
				1.f/RENDERW, -1.f/RENDERH,
				-0.5f+x/(float)RENDERW, 0.5f-y/(float)RENDERH );
			temp = CNFGglGetUniformLocation ( lppv_render, "geotex" );
			if( temp >= 0 ) CNFGglUniform1i( temp, 0 );
			temp = CNFGglGetUniformLocation ( lppv_render, "geowidth" );
			if( temp >= 0 ) CNFGglUniform1i( temp, texwid );
			temp = CNFGglGetUniformLocation ( lppv_render, "geoheight" );
			if( temp >= 0 ) CNFGglUniform1i( temp, texhei );
			glBindTexture(GL_TEXTURE_2D, geotex );
			glDrawArrays( GL_TRIANGLES, 0, 6);	
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			
			glBindFramebuffer( GL_FRAMEBUFFER, fboS2 );
			glEnable( GL_BLEND );
			glBlendFunc( GL_ONE, GL_ONE );
			glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
			glClear( GL_COLOR_BUFFER_BIT  );
			glViewport( 0, 0, PASS2W, PASS2H );
			CNFGglUseProgram( lppv_render_stage2 );
			temp = CNFGglGetUniformLocation ( lppv_render_stage2, "xfrm" );
			if( temp >= 0 ) CNFGglUniform4f( temp,
				1.f/PASS2W, -1.f/PASS2H,
				-0.5f+x/(float)PASS2W, 0.5f-y/(float)PASS2H );
			glBindTexture(GL_TEXTURE_2D, texS1 );
			CNFGglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, base2_verts);
			glDrawArrays( GL_TRIANGLES, 0, 6*RENDERW*RENDERH);	
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			
			glEnable( GL_BLEND );
			glBlendFunc( GL_ONE, GL_ZERO );

		}
		else
		{
			CNFGPenX = 1; CNFGPenY = 1;
			char cts[256];
			sprintf( cts, "Compile Error: %d %d\n", lppv_render, lppv_render_stage2 );
			CNFGDrawText( cts, 2 );
		}

		glViewport( 0, 0, w, h );

		int LX = 0;
		int LY = 0;
		CNFGglUseProgram( debug_blit );
		CNFGglUniform4f( gRDBlitProgUX, 1.f/w, -1.f/h, -0.5f+LX/(float)w, 0.5f-LY/(float)h );
		CNFGglUniform1i( gRDBlitProgUT, 0 );
		glBindTexture(GL_TEXTURE_2D, texS2 );
		CNFGglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, base_verts_square);
		CNFGglVertexAttribPointer(1, 2, GL_UNSIGNED_BYTE, GL_TRUE, 0, base_colors);
		glDrawArrays( GL_TRIANGLES, 0, 6);
		
		// read data back
		while( frame_to_process );
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, (void*)(0));
		glBindTexture(GL_TEXTURE_2D, 0 );



		LX = 256;
		CNFGglUseProgram( debug_blit );
		CNFGglUniform4f( gRDBlitProgUX, 1.f/w, -1.f/h, -0.5f+LX/(float)w, 0.5f-LY/(float)h );
		CNFGglUniform1i( gRDBlitProgUT, 0 );
		glBindTexture(GL_TEXTURE_2D, texS1 );
		CNFGglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, base_verts );
		CNFGglVertexAttribPointer(1, 2, GL_UNSIGNED_BYTE, GL_TRUE, 0, base_colors);
		glDrawArrays( GL_TRIANGLES, 0, 6);
		glBindTexture(GL_TEXTURE_2D, 0 );
		
		
		frame_to_process = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		OGUnlockSema( sem_ftp );

		
		CNFGPenX = 1;
		CNFGPenY = 256;
		char st[128];
		sprintf( st, "FPS: %d\n", FPSCt );
		CNFGDrawText( st, 2 );
		
		FrameCt++;
		if( Now - LastSecond >= 1 )
		{
			FPSCt = FrameCt;
			FrameCt = 0;
			LastSecond++;
		}

		GLenum gle = glGetError();
		if( gle )
		{
			printf( "GL Error Code: %d / %08x\n", gle, gle );
		}

		CNFGSwapBuffers();		
	}
}
