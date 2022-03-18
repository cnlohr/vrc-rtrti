#version 450

in vec2 tc;
out vec4 fragcolor;

uniform highp sampler2D  geotex;
uniform int geoheight;
uniform int geowidth;

ivec2 txvf( vec2 fv )
{
	return ivec2( fv.xy*vec2(geowidth, geoheight) );
}

#define tex2Dlod( tex, v ) \
	texture( tex, v.xy )
//	texelFetch( tex, txvf(v.xy), 0 )

#define _GeoTex geotex

float max_component( vec3 v )
{
	return max (max (v.x, v.y), v.z);
}

float min_component( vec3 v )
{
	return min(min(v.x, v.y), v.z);
}

vec4 CoreTrace( vec3 eye, vec3 dir )
{
	vec4 _GeoTex_TexelSize = vec4( 1.0/geowidth, 1.0/geoheight, geowidth, geoheight );
	dir = normalize( dir );

	vec2 ptr = vec2( 0, 0 );
	vec2 ptrhit = vec2( -1 );
	vec4 mincorner;
	vec4 maxcorner;
	vec4 truefalse;
	int i = 0;
	float minz = 1.0e20;
	vec2 uvo = vec2( -1.0 );
	vec3 invd = 1.0 / dir;
	int tricheck = 0;

	{
		// Pick the most optimal stepping order.

		vec3 dira = abs( dir );
		int axisno = (dira.x>dira.y)? ( (dira.x > dira.z)?0:2 ) : ( (dira.y > dira.z)?1:2 );
		int dire = (dir[axisno]>0)?1:0;
		ptr = tex2Dlod( _GeoTex, vec4( (axisno * 2 + dire)*_GeoTex_TexelSize.x, 0, 0, 0) ).xy*_GeoTex_TexelSize.xy;
	}

	while( i++ < 255 && ptr.y > 0 )
	{
		mincorner = tex2Dlod( _GeoTex, vec4( ptr, 0, 0) );
		maxcorner = tex2Dlod( _GeoTex, vec4( ptr + vec2( _GeoTex_TexelSize.x*1, _GeoTex_TexelSize.y*0 ), 0, 0 ) );
		truefalse = tex2Dlod( _GeoTex, vec4( ptr + vec2( _GeoTex_TexelSize.x*0, _GeoTex_TexelSize.y*1 ), 0, 0 ) )*_GeoTex_TexelSize.xyxy;
		// minmaxcorner:  [ x y z ] [ radius ]
		// minmaxcorner:  [ x y z ] [ 1 if triangle, 0 otherwise ]

		return vec4(truefalse.xyx*500.0, 1.0);

		//XXX XXX OPTIMIZATION OPPORTUNITY:  We can know which ones are min/max rel.
	
		// Effectively "slabs" function from: https://jcgt.org/published/0007/03/04/paper-lowres.pdf
		vec3 minrel = mincorner.xyz - eye;
		vec3 maxrel = maxcorner.xyz - eye;
		vec3 t0 = minrel * invd;
		vec3 t1 = maxrel * invd;
		vec3 tmin = min(t0,t1), tmax = max(t0,t1);

		// This can be as simple as dividing the axis-value in dir by the distance to the object.
		float r = mincorner.a;
		vec3 center = (minrel + maxrel)/2;
		float dr = dot( center, dir );
		
		bool hit = all( lessThan (
			vec3( max_component(tmin), 0, dr-r )
			,
			vec3( min_component(tmax), dr+r, minz ) ) );

		// Does intersect, AND is it in front of us, and behind the closest hit object?
		if( hit && maxcorner.a > 0)
		{
			// Do triangle intersection. If not, set to no intersection.
			//vec3 N =  tex2Dlod( _GeoTex, vec4( ptr + vec2( _GeoTex_TexelSize.x * 1, _GeoTex_TexelSize.y ), 0.0, 0.0 ) );
			vec4 v0 = tex2Dlod( _GeoTex, vec4( ptr + vec2( _GeoTex_TexelSize.x * 1, _GeoTex_TexelSize.y*1 ), 0.0, 0.0 ) );
			vec4 v1 = tex2Dlod( _GeoTex, vec4( ptr + vec2( _GeoTex_TexelSize.x * 2, _GeoTex_TexelSize.y*0 ), 0.0, 0.0 ) );
			vec4 v2 = tex2Dlod( _GeoTex, vec4( ptr + vec2( _GeoTex_TexelSize.x * 2, _GeoTex_TexelSize.y*1 ), 0.0, 0.0 ) );
			
			// Compute t and barycentric coordinates using Moller-Trumbore
			// https://tr.inf.unibe.ch/pdf/iam-04-004.pdf
			vec3 E1 = v1.xyz; // Implicitly is - v0
			vec3 E2 = v2.xyz; // Implicitly is - v0
			vec3 T = eye - v0.xyz;
			vec3 Q = cross( T, E1 );
			vec3 P = cross( dir, E2 );
			vec4 tbary = 
				vec4(
					1.0/dot(P,E1)*vec3(
						dot( Q, E2 ),
						dot( P, T ),
						dot( Q, dir ) ), 0.0 );
			tbary.w = 1.0 - tbary.y - tbary.z;
#ifdef DEBUG_TRACE
			tricheck++;
#endif
			// Note: Normally, to not hit backfaces, we do dot( N, dir ) > 0, but
			// I think Q and dir is ok.
			if( all( greaterThanEqual( tbary.xyzw, vec4( 0 ) ) ) && tbary.x < minz && dot( Q, dir ) < 0 )
			{
				minz = tbary.x;
				ptrhit = vec2( v0.a, v1.a )*_GeoTex_TexelSize.xy;
			}
		}
		ptr = hit?truefalse.xy:truefalse.zw;
	}
	return vec4( i/40., 0, 0, 1);
	return vec4( ptrhit, minz,  i + tricheck * 1000 );
}


float GetTriDataFromPtr( vec3 eye, vec3 dir, vec2 ptr, out vec2 uvo, out vec3 hitnorm )
{
	vec4 _GeoTex_TexelSize = vec4( 1.0/geowidth, 1.0/geoheight, geowidth, geoheight );
	// Do triangle intersection. If not, set to no intersection.
	vec4 v0 = tex2Dlod( _GeoTex, vec4( ptr + vec2( _GeoTex_TexelSize.x * 0, _GeoTex_TexelSize.y*0 ), 0.0, 0.0 ) );
	vec4 v1 = tex2Dlod( _GeoTex, vec4( ptr + vec2( _GeoTex_TexelSize.x * 1, _GeoTex_TexelSize.y*0 ), 0.0, 0.0 ) );
	vec4 v2 = tex2Dlod( _GeoTex, vec4( ptr + vec2( _GeoTex_TexelSize.x * 2, _GeoTex_TexelSize.y*0 ), 0.0, 0.0 ) );
	vec4 n0 = tex2Dlod( _GeoTex, vec4( ptr + vec2( _GeoTex_TexelSize.x * 0, _GeoTex_TexelSize.y*1 ), 0.0, 0.0 ) );
	vec4 n1 = tex2Dlod( _GeoTex, vec4( ptr + vec2( _GeoTex_TexelSize.x * 1, _GeoTex_TexelSize.y*1 ), 0.0, 0.0 ) );
	vec4 n2 = tex2Dlod( _GeoTex, vec4( ptr + vec2( _GeoTex_TexelSize.x * 2, _GeoTex_TexelSize.y*1 ), 0.0, 0.0 ) );

	// Compute t and barycentric coordinates using Moller-Trumbore
	// https://tr.inf.unibe.ch/pdf/iam-04-004.pdf
	vec3 E1 = v1.xyz; // Implicitly is - v0
	vec3 E2 = v2.xyz; // Implicitly is - v0
	vec3 T = eye - v0.xyz;
	vec3 Q = cross( T, E1 );
	vec3 P = cross( dir, E2 );
	vec4 tbary = 
		vec4(
			1.0/dot(P,E1)*vec3(
				dot( Q, E2 ),
				dot( P, T ),
				dot( Q, dir ) ), 0.0 );
	tbary.w = 1.0 - tbary.y - tbary.z;
	
	vec2 uv0 = vec2( v0.w, n0.x );
	vec2 uv1 = vec2( v1.w, n1.x );
	vec2 uv2 = vec2( v2.w, n2.x );
	vec2 uv = uv0 * tbary.w + uv1 * tbary.y + uv2 * tbary.z;
	hitnorm = n0.yzw * tbary.w + n1.yzw * tbary.y + n2.yzw * tbary.z;
	uvo = uv;
	return tbary.x;
}


void main()
{
	vec3 eye = vec3( 0, 0, 20. );
	vec3 dir = normalize(vec3( tc.x-0.5, tc.y-0.5, 0.4 ));
	vec4 ctdata = CoreTrace( eye, dir );
	fragcolor = ctdata;

	//vec4 _GeoTex_TexelSize = vec4( 1.0/geowidth, 1.0/geoheight, geowidth, geoheight );
	//fragcolor = vec4(tex2Dlod( _GeoTex, vec4( tc.xy*_GeoTex_TexelSize.xy*512/20, 0, 0 ) ).xyz, 1.0);
	//fragcolor = vec4( _GeoTex_TexelSize.xy*1024, 0, 1 );
}



