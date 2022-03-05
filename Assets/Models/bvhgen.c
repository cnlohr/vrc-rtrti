// Execute with 
//   tcc -run bvhgen.c -lm

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "unitytexturewriter.h"
#include <math.h>

// TODO:
//  1) Fix bounding binding something's wrong with the bounding spheres.
//  2) Improve performance of pair finding.

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
	int vertex_count = 0;
	float * normals = 0;
	int normal_count = 0;
	float * tcoords = 0;
	int tcoord_count = 0;
	
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
				printf( "%p %p %d %p\n", vend, vertices, vertex_count, f );
				if( fscanf( f, "%f %f %f", vend, vend+1, vend+2 ) != 3 )
				{
					fprintf( stderr, "Error parsing vertices on line %d\n", line );
					goto fail;
				}
				
				//XXX WARNING: we invert X.
				*vend *= -1; 
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
				td[3+i*8] = tcoords[tno*2+0];
				td[4+i*8] = tcoords[tno*2+1];
				td[5+i*8] = normals[nno*3+0];
				td[6+i*8] = normals[nno*3+1];
				td[7+i*8] = normals[nno*3+2];
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
	int x, y; // Start
	int w, h; // Size (right now 2 for bv, 8 for triangle)
};


void cross3d( float * out, const float * a, const float * b )
{
	out[0] = a[1]*b[2] - a[2]*b[1];
	out[1] = a[2]*b[0] - a[0]*b[2];
	out[2] = a[0]*b[1] - a[1]*b[0];
}

float dist3d( const float * a, const float * b )
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

float mag3d( const float * a )
{
	return sqrt( a[0] * a[0] + a[1] * a[1] + a[2] * a[2] );
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
		float l0 = dist3d( m, tv+0 );
		float l1 = dist3d( m, tv+8 );
		float l2 = dist3d( m, tv+16 );
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
				float dist = dist3d( jp->xyzr, ip->xyzr ) + jp->xyzr[3] + ip->xyzr[3];
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
		float lenji = dist3d( jp->xyzr, ip->xyzr );
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
			//printf( "J %d (%f+%f+%f) %d %d <%f %f %f %f - %f %f %f %f>\n", nrpairs, edgej, lenji, edgei, besti, bestj, ip->xyzr[0], ip->xyzr[1], ip->xyzr[2], ip->xyzr[3], jp->xyzr[0], jp->xyzr[1], jp->xyzr[2], jp->xyzr[3] );
		}
		else if( edgej + lenji < edgei )
		{
			memcpy( xyzr, ip->xyzr, sizeof( ip->xyzr ) );
			//printf( "I %d (%f+%f+%f) %d %d <%f %f %f %f - %f %f %f %f>\n", nrpairs, edgej, lenji, edgei, besti, bestj, ip->xyzr[0], ip->xyzr[1], ip->xyzr[2], ip->xyzr[3], jp->xyzr[0], jp->xyzr[1], jp->xyzr[2], jp->xyzr[3] );
		}
		else
		{
			// The new center is between the two.
			float r1 = edgei;
			float r2 = edgej;
			float * c1 = ip->xyzr;
			float * c2 = jp->xyzr;
			float clen = lenji;
			float R = ( r1 + r2 + clen )/2;
			xyzr[0] = c1[0] + ( c2[0] - c1[0] ) * (R - r1) / clen;
			xyzr[1] = c1[1] + ( c2[1] - c1[1] ) * (R - r1) / clen;
			xyzr[2] = c1[2] + ( c2[2] - c1[2] ) * (R - r1) / clen;
			xyzr[3] = R;
			
#if 0
			float edgesize = (edgej + lenji + edgei)/2;
			float center = edgesize - edgei;
			center /= lenji;
			mul3d( vecji, center );
			float * ix = ip->xyzr;
			//add3d( xyzr, vecji, ix );
			xyzr[0] = vecji[0] + ix[0];
			xyzr[1] = vecji[1] + ix[1];
			xyzr[2] = vecji[2] + ix[2];
			xyzr[3] = edgesize*10;
			printf( "B %d %f %f (%f+%f+%f) %d %d <%f %f %f %f - %f %f %f %f> = %f %f %f %f\n", nrpairs, center, edgesize, edgej, lenji, edgei, besti, bestj, ip->xyzr[0], ip->xyzr[1], ip->xyzr[2], ip->xyzr[3], jp->xyzr[0], jp->xyzr[1], jp->xyzr[2], jp->xyzr[3],
				xyzr[0], xyzr[1], xyzr[2], xyzr[3] );
#endif
		}
		
		// XXX TODO: OPTIMIZATION: Greedily find new center.
		
		jp->parent = parent;
		ip->parent = parent;
		nrpairs++;
	} while( 1 );
	printf( "Done\n" );
	return pairs + nrpairs - 1;
}

#define TEXW 128
#define TEXH 128
float asset2d[TEXH][TEXW][4];
int lineallocations[TEXH];
int totalallocations;
int trianglecount;
int bvhcount;

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
			totalallocations += pixels;
			return 0;
		}
	}
	fprintf( stderr, "No room left in geometry map\n" );
	return -1;
}

int AllocateBVH( struct BVHPair * tt )
{
	tt->h = 1;
	tt->w = (tt->triangle_number<0)?2:9;

	if( Allocate( tt->w, &tt->x, &tt->y ) < 0 )
		return -1;

	trianglecount += (tt->triangle_number<0)?0:1;
	bvhcount ++;
	
	if( tt->a ) 
		if( AllocateBVH( tt->a ) < 0 )
			return -1;
	if( tt->b )
		if( AllocateBVH( tt->b ) < 0 )
			return -1;
	return 0;
}

// Get the "next" node if this node is false.
struct BVHPair * FindFallBVH( struct BVHPair * tt )
{
	// If root of tree, we are freeeee
	if( !tt->parent ) return 0;
	
	if( tt->parent->b != tt )
		return tt->parent->b;
	
	return FindFallBVH( tt->parent );
}



int WriteInBVH( struct BVHPair * tt, float * triangles )
{
	// BVH
	if( tt->a )
		WriteInBVH( tt->a, triangles );
	if( tt->b )
		WriteInBVH( tt->b, triangles );

	// Fill our "hit" in to be 
	int x = tt->x;
	int y = tt->y;
	
	int j;
	float * hitmiss = asset2d[y][x+1];
	if( !tt->a )
	{
		//XXXX TOOD If we have a "HIT" on a leaf node, what does that mean?
		hitmiss[0] = -1;
		hitmiss[1] = -1;
	}
	else
	{
		hitmiss[0] = tt->a->x / (float)TEXW;
		hitmiss[1] = tt->a->y / (float)TEXH;
	}

	struct BVHPair * next = FindFallBVH( tt );
	if( next )
	{
		hitmiss[2] = next->x / (float)TEXW;
		hitmiss[3] = next->y / (float)TEXH;
	}
	else
	{
		hitmiss[2] = -1;
		hitmiss[3] = -1;
	}

	memcpy( asset2d[y][x], tt->xyzr, sizeof( float ) * 4 );
	asset2d[y][x][3] = asset2d[y][x][3] * asset2d[y][x][3];// Tricky: We do r^2 because that makes the math work out better in the shader.

#if 0
	if( tt->triangle_number >= 0 )
	{
		for( j = 2; j < tt->w; j++ )
		{
			if( tt->w < 3 )
			{
				asset2d[y][x+j][0] = 1.0;
				asset2d[y][x+j][1] = 0.0;
				asset2d[y][x+j][2] = 1.0;
				asset2d[y][x+j][3] = 1.0;
			}
			else
			{
				asset2d[y][x+j][0] = 0.0;
				asset2d[y][x+j][1] = 1.0;
				asset2d[y][x+j][2] = 0.0;
				asset2d[y][x+j][3] = 1.0;
			}
		}
	}

#endif
	printf( "%d %d %d\n", tt->triangle_number, x, y );
	if( tt->triangle_number >= 0 )
	{
		// Just FYI for this hitmiss[0] / 1 will be negative
		float * this_tri = triangles + tt->triangle_number * 24;
		memcpy( asset2d[y][x+3], this_tri, sizeof( float ) * 24 );

		// Compute the normal to the surface of this triangle.
		float dA[3] = { this_tri[8] - this_tri[0], this_tri[9] - this_tri[1], this_tri[10] - this_tri[2] };
		float dB[3] = { this_tri[16] - this_tri[0], this_tri[17] - this_tri[1], this_tri[18] - this_tri[2] };
		float norm[3];
		cross3d( norm, dA, dB );
		mul3d( norm, 1.0/mag3d( norm ) );
		memcpy( asset2d[y][x+2], norm, sizeof(float)*3 );
	}

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

	if( AllocateBVH( root ) < 0 )
		return -1;	

	WriteInBVH( root, tridata );

	WriteUnityImageAsset( "geometryimage.asset", asset2d, sizeof(asset2d), TEXW, TEXH, 0, UTE_RGBA_FLOAT );

	printf( "Usage: %d / %d (%3.2f%%)\n", totalallocations, TEXW*TEXH, ((float)totalallocations)/(TEXW*TEXH)*100. );
	printf( "Triangles: %d\n", trianglecount );
	printf( "BVH Count: %d\n", bvhcount );
}
