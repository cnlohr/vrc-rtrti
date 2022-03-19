//
// tcc bakelppv.c -lOpenGL32 -lkernel32 -luser32 -lgdi32
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

#ifndef GL_RGBA32F
#define GL_RGBA32F   0x8814
#endif

#define RENDERW 512
#define RENDERH 256

int quit = 0;

void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
void HandleDestroy() { }

uint32_t * frame_to_process = 0;

void * ProcessThread( void * v )
{
	while( !quit )
	{
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



int main()
{
	CNFGSetup( "bakelppv", 1024, 768 );

	GLint rendershader = -1;
	double rsvert = -1;
	double rsfrag = -1;

	float * texdata = 0;
	double textime = -1;
	int texwid = 0;
	int texhei = 0;
	GLuint geotex;
	glGenTextures( 1, &geotex );
	
	void (*SwapInterval)( int ) = CNFGGetProcAddress("wglSwapIntervalEXT");
	if( SwapInterval ) SwapInterval( 0 );


	double Now, LastSecond = OGGetAbsoluteTime();
	int FrameCt = 0;
	int FPSCt = 0;
	int pingpong = 0;
	
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


		if( OGGetFileTime( "lppv_render.vert" ) != rsvert || OGGetFileTime( "lppv_render.frag" ) != rsfrag )
		{
			OGUSleep( 100000 );
			rsvert = OGGetFileTime( "lppv_render.vert" );
			rsfrag = OGGetFileTime( "lppv_render.frag" );
			if( rendershader >= 0 )
			{
				CNFGglDeleteShader( rendershader );
			}
			char * vert = FileToString( "lppv_render.vert" );
			char * frag = FileToString( "lppv_render.frag" );
			GLint r = -1;
			if( vert && frag )
			{
				r = CNFGGLInternalLoadShader( vert, frag );
			}
			if( r >= 0 )
			{
				rendershader = r;				
			}
			else
			{
				rendershader = -1;
			}
			
			
			CNFGglEnableVertexAttribArray(0);
			CNFGglEnableVertexAttribArray(1);

		}

		short w, h;
		CNFGClearFrame();
		CNFGHandleInput();
		CNFGGetDimensions( &w, &h );
		CNFGColor( 0xffffffff ); 

		if( rendershader > 0 && texdata )
		{
			int x = 0;
			int y = 0;
			CNFGglUseProgram( rendershader );
			int temp;


			CNFGFlushRender();

			glDisable(GL_DEPTH_TEST);
			glDepthMask( GL_FALSE );
			glEnable( GL_BLEND );
			glBlendFunc( GL_ONE, GL_ZERO );

			temp = CNFGglGetUniformLocation ( rendershader, "xfrm" );
			if( temp >= 0 ) CNFGglUniform4f( temp,
				1.f/gRDLastResizeW, -1.f/gRDLastResizeH,
				-0.5f+x/(float)gRDLastResizeW, 0.5f-y/(float)gRDLastResizeH );
			temp = CNFGglGetUniformLocation ( rendershader, "geotex" );
			if( temp >= 0 ) CNFGglUniform1i( temp, 0 );
			temp = CNFGglGetUniformLocation ( rendershader, "geowidth" );
			if( temp >= 0 ) CNFGglUniform1i( temp, texwid );
			temp = CNFGglGetUniformLocation ( rendershader, "geoheight" );
			if( temp >= 0 ) CNFGglUniform1i( temp, texhei );
			
			glBindTexture(GL_TEXTURE_2D, geotex );

			int ww = RENDERW;
			int wh = RENDERH;
			const float verts[] = {
				0,0, ww,0, ww,wh,
				0,0, ww,wh, 0,wh, };
			static const uint8_t colors[] = {
				0,0,   255,0,  255,255,
				0,0,  255,255, 0,255 };

			CNFGglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
			CNFGglVertexAttribPointer(1, 2, GL_UNSIGNED_BYTE, GL_TRUE, 0, colors);

			glDrawArrays( GL_TRIANGLES, 0, 6);
		}
		else
		{
			CNFGPenX = 1; CNFGPenY = 1;
			CNFGDrawText( "Compile Error", 2 );
		}
		
		uint32_t buffer[RENDERW*RENDERH*2];
		pingpong = (pingpong + 1)%2;
		uint32_t * tbuf = &buffer[RENDERW*RENDERH*pingpong];
		// Tricky: glReadPixels operates from bottom-left
		glReadPixels( 0, h-RENDERH, RENDERW, RENDERH, GL_RGBA, GL_UNSIGNED_BYTE, tbuf );
		while( frame_to_process != 0 ); // Wait before overwriting process.
		frame_to_process = tbuf;
		
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
