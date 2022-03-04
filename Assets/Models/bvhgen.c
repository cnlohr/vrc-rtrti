// Execute with 
//   tcc -run bvhgen.c -lm

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "unitytexturewriter.h"
#include <math.h>

int OpenOBJ( const char * name, float ** tridata, int * tricount )
{
	//float * tridata; // x, y, z, tcx, tcy, nx, ny, nz
	//int tricount;

	FILE * f = fopen( "TestScene.obj", "r" );
	if( !f || ferror( f ) )
	{
		fprintf( stderr, "Error: could not open model file.\n" );
		return -1;
	}
	
	float * vertices = 0;
	int vertex_count;
	float * normals = 0;
	int normal_count;
	float * tcoords = 0;
	int tcoord_count;
	
	int c;
	int line = 0;

	while( !feof( f ) )
	{
		line++;
		int c = fgetc( f );
		if( c == 'v' )
		{
			c = fgetc( f );
			if( c == ' ' )
			{
				vertex_count++;
				vertices = realloc( vertices, vertex_count * 3 * sizeof( float ) );
				float * vend = vertices + (vertex_count - 1) * 3;
				if( fscanf( f, "%f %f %f", vend, vend+1, vend+2 ) != 3 )
				{
					fprintf( stderr, "Error parsing vertices on line %d\n", line );
					goto fail;
				}
			}
			else if( c == 'n' )
			{
				c = fgetc( f );
				normal_count++;
				normals = realloc( normals, normal_count * 3 * sizeof( float ) );
				float * vend = normals + (normal_count - 1) * 3;
				if( fscanf( f, "%f %f %f", vend, vend+1, vend+2 ) != 3 )
				{
					fprintf( stderr, "Error parsing normals on line %d\n", line );
					goto fail;
				}
			}
			else if( c == 't' )
			{
				c = fgetc( f );
				tcoord_count++;
				tcoords = realloc( tcoords, tcoord_count * 2 * sizeof( float ) );
				float * vend = tcoords + (tcoord_count - 1) * 2;
				if( fscanf( f, "%f %f", vend, vend+1 ) != 2 )
				{
					fprintf( stderr, "Error parsing tcs on line %d\n", line );
					goto fail;
				}
			}
		}
		else if( c == 'f' )
		{
			char vpt[3][128];
			c = fgetc( f );
			if( fscanf( f, "%127s %127s %127s", vpt[0], vpt[1], vpt[2] ) != 3 )
			{
				fprintf( stderr, "Error: Bad face reading on line %d\n", line );
				goto fail;
			}
			int otc = *tricount;
			(*tricount) ++;
			*tridata = realloc( *tridata, (*tricount) * 8*3 * sizeof( float ) );
			float * td = (*tridata) + otc * 8*3;
			int i;
			for( i = 0; i < 3; i++ )
			{
				int j;
				int len = strlen( vpt[i] );
				for( j = 0; j < len; j++ ) if( vpt[i][j] == '/' ) vpt[i][j] = ' ';
				int vno;
				int tno;
				int nno;
				int k = sscanf( vpt[i], "%d %d %d", &vno, &tno, &nno );
				if( k != 3 )
				{
					fprintf( stderr, "Error on line %d\n", line );
					goto fail;
				}
				vno--;
				tno--;
				nno--;
				if( vno > vertex_count || vno < 0 ) { fprintf( stderr, "Error: vertex count on line %d bad\n", line ); goto fail; }
				if( tno > tcoord_count || tno < 0 ) { fprintf( stderr, "Error: texture count on line %d bad\n", line ); goto fail; }
				if( nno > normal_count || nno < 0 ) { fprintf( stderr, "Error: normal count on line %d bad\n", line ); goto fail; }
				td[0+i*8] = vertices[vno*3+0];
				td[1+i*8] = vertices[vno*3+1];
				td[2+i*8] = vertices[vno*3+2];
				td[3+i*8] = tcoords[vno*2+0];
				td[4+i*8] = tcoords[vno*2+1];
				td[5+i*8] = normals[vno*3+0];
				td[6+i*8] = normals[vno*3+1];
				td[7+i*8] = normals[vno*3+2];
			}
		}
		while( ( c = fgetc( f ) ) != '\n' ) if( c == EOF ) break;
	}
	goto finish;
fail:
	*tricount = -1;
finish:
	free( vertices );
	free( normals );
	free( tcoords );
	return *tricount;
}

#define MAX_BVPER 4
#define MAX_TRIPER 2

struct BVHPair
{
	struct BVHPair * a;
	struct BVHPair * b;
	struct BVHPair * parent;
	float xyzr[4];
	int triangle_number; /// If -1 is a inner node.
};

float len3d( const float * a, const float * b )
{
	float del[3];
	del[0] = a[0] - b[0];
	del[0] *= del[0];
	del[1] = a[1] - b[1];
	del[1] *= del[1];
	del[2] = a[2] - b[2];
	del[2] *= del[2];
	return sqrt( del[0] + del[1] + del[2] );
}

void mul3d( float * val, float mag)
{
	val[0] = val[0] * mag;
	val[1] = val[1] * mag;
	val[2] = val[2] * mag;
}

struct BVHPair * BuildBVH( struct BVHPair * pairs, float * tridata, int tricount )
{
	memset( pairs, 0, sizeof( pairs ) );
	int nrpairs;
	float * trimetadata = malloc( tricount * 4 * sizeof( float ) );
	int i;
	for( i = 0; i < tricount; i++ )
	{
		float * tv = tridata + i*24;
		float * m = pairs[i].xyzr;
		m[0] = (tv[0] + tv[8] + tv[16])/3;
		m[1] = (tv[1] + tv[9] + tv[17])/3;
		m[2] = (tv[2] + tv[10] + tv[18])/3;
		float l0 = len3d( m, tv+0 );
		float l1 = len3d( m, tv+8 );
		float l2 = len3d( m, tv+16 );
		m[3] = (l0>l1)?(l0>l2)?l0:l2:(l1>l2)?l1:l2;
		printf( "%d / %f %f %f / %f\n", i, tv[0], tv[8], tv[16], m[3] );
		pairs[i].triangle_number = i;
	}
	
	nrpairs = i;

	// Now, pairs from 0..tricount are leaf (Triangle) nodes on up.
	// Building a BVH this way isn't perfectly optimal but for a binary BVH, it's really good.
	int any_left = 0;
	do
	{
		any_left = 0;
		int besti, bestj;
		float smallestr = 1e20;
		int i, j;
		for( j = 0; j < nrpairs; j++ )
		{
			struct BVHPair * jp = pairs+j;
			if( jp->parent ) continue; // Already inside a tree.
			for( i = 0; i < nrpairs; i++ )
			{
				if( i == j ) continue;
				struct BVHPair * ip = pairs+i;
				if( ip->parent ) continue; // Already inside a tree.
				any_left = 1;
				float dist = len3d( jp->xyzr, ip->xyzr ) + jp->xyzr[3] + ip->xyzr[3];
				if( dist < smallestr )
				{
					smallestr = dist;
					besti = i;
					bestj = j;
				}
			}
		}
		if (!any_left) break;
		// Pair them up.
		struct BVHPair * jp = pairs+bestj;
		struct BVHPair * ip = pairs+besti;	
		struct BVHPair * parent = pairs + nrpairs;
		parent->a = jp;
		parent->b = ip;
		parent->triangle_number = -1;
		// Tricky - joining two spheres. 
		float * xyzr = parent->xyzr;
		float vecji[3] = { jp->xyzr[0] - ip->xyzr[0], jp->xyzr[1] - ip->xyzr[1], jp->xyzr[2] - ip->xyzr[2] };
		float lenji = len3d( jp->xyzr, ip->xyzr );
		if( lenji > 0.001 )
			mul3d( vecji, 1.0/lenji );
		else
			mul3d( vecji, 0.0 );
		// edgej / lenji / edgei
		// Special case: If one bv completely contains another.
		//  i.e. edgej = 10, lenji = 1, edgei = 1
		float edgej = jp->xyzr[3];
		float edgei = ip->xyzr[3];
		if( edgej > lenji + edgei )
		{
			memcpy( xyzr, jp->xyzr, sizeof( jp->xyzr ) );
			printf( "J %d (%f+%f+%f) %d %d <%f %f %f %f - %f %f %f %f>\n", nrpairs, edgej, lenji, edgei, besti, bestj, ip->xyzr[0], ip->xyzr[1], ip->xyzr[2], ip->xyzr[3], jp->xyzr[0], jp->xyzr[1], jp->xyzr[2], jp->xyzr[3] );
		}
		else if( edgej + lenji < edgei )
		{
			memcpy( xyzr, ip->xyzr, sizeof( ip->xyzr ) );
			printf( "I %d (%f+%f+%f) %d %d <%f %f %f %f - %f %f %f %f>\n", nrpairs, edgej, lenji, edgei, besti, bestj, ip->xyzr[0], ip->xyzr[1], ip->xyzr[2], ip->xyzr[3], jp->xyzr[0], jp->xyzr[1], jp->xyzr[2], jp->xyzr[3] );
		}
		else
		{
			// The new center is between the two.
			float edgesize = (edgej + lenji + edgei)/2;
			float center = edgesize - edgei;
			center /= lenji;
			mul3d( vecji, center );
			float * ix = ip->xyzr;
			//add3d( xyzr, vecji, ix );
			xyzr[0] = vecji[0] + ix[0];
			xyzr[1] = vecji[1] + ix[1];
			xyzr[2] = vecji[2] + ix[2];
			xyzr[3] = edgesize;
			printf( "B %d %f %f (%f+%f+%f) %d %d <%f %f %f %f - %f %f %f %f> = %f %f %f %f\n", nrpairs, center, edgesize, edgej, lenji, edgei, besti, bestj, ip->xyzr[0], ip->xyzr[1], ip->xyzr[2], ip->xyzr[3], jp->xyzr[0], jp->xyzr[1], jp->xyzr[2], jp->xyzr[3],
				xyzr[0], xyzr[1], xyzr[2], xyzr[3] );
		}
		
		// XXX TODO: OPTIMIZATION: Greedily find new center.
		
		jp->parent = parent;
		ip->parent = parent;
		nrpairs++;
	} while( 1 );
	printf( "Done\n" );
	return pairs + nrpairs - 1;
}

#define TEXW 512
#define TEXH 512
float asset2d[TEXW][TEXH][4];
int lineallocations[TEXH];



int Allocate( int pixels, int * x, int * y )
{
	int i;
	for( i = 0; i < TEXH; i++ )
	{
		if( TEXW - lineallocations[i] > pixels )
		{
			*x = lineallocations[i];
			*y = i;
			lineallocations[i] += pixels;
			return 0;
		}
	}
	fprintf( stderr, "No room left in geometry map\n" );
	return -1;
}

int WriteInBVH( struct BVHPair * tt, int * x, int * y, float * triangles )
{
	// Texture format:
	//  XYZR / HIT MISS
	//  If sign(R) < 0 check intersect with triangle, else go to miss.
	//  TTRRRIIIAANNGGLLLEEEEE
	//
	// This handles taking the tree and flattening the traversal logic into a very, very simple jumpmap.

	//XXX CONSDER
	if( Allocate( (tt->triangle_number<0)?2:6, *x, *y ) < 0 )
	{
		return -1;
	}
	
	if( tt->triangle_number < 0 )
	{
		// BVH
		int x1, y1, x2, y2;
		WriteInBVH( tt->a, &x1, &y1 );

		// Fill our "hit" in to be 
		float * hitmiss = asset2d[*x+1][*y];
		hitmiss[0] = x1 / (float)TEXW;
		hitmiss[1] = y1 / (float)TEXW;

		WriteInBVH( tt->b, &x2, &y2 );
		memcpy( asset2d[*x][*y], tt->xyzr, sizeof( float ) * 4 );
		
		// We have to write in A's miss cell to point into B's cell.
		float * ahitmiss = asset2d[x1][y1];
		hitmiss[2] = x2 / (float)TEXW;
		hitmiss[3] = y2 / (float)TEXW;
		
		// Now, we need to update our miss cell --- and --- we have to update B's miss cell.
		// It's like pretend tail recursion.
		// XXX TODO
	}
	else
	{
		// Triangles
		memcpy( asset2d[*x][*y], triangles + tt->triangle_number * 24, sizeof( float ) * 24 );
	}
	

/*
struct BVHPair
{
	struct BVHPair * a;
	struct BVHPair * b;
	struct BVHPair * parent;
	float xyzr[4];
	int triangle_number; /// If -1 is a inner node.
};*/

	return 0;
}

int main( )
{
	float * tridata = 0;
	int tricount = 0;
	if( OpenOBJ( "TestScene.obj", &tridata, &tricount ) <= 0 )
	{
		fprintf( stderr, "Error: couldn't open OBJ file\n" );
		return -1;
	}
	
	struct BVHPair * allpairs = calloc( sizeof( struct BVHPair ), tricount*2 );
	struct BVHPair * root = BuildBVH( allpairs, tridata, tricount );

	float x, y;
	WriteInBVH( root );

	WriteUnityImageAsset( "geometryimage.asset", asset2d, sizeof(asset2d), TEXW, TEXH, 0, UTE_RGBA_FLOAT );
}
