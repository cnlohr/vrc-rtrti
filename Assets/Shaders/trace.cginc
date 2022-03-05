sampler2D _GeoTex;
float4 _GeoTex_TexelSize;
sampler2D _EmissionTex;

float4 CoreTrace( float3 eye, float3 dir )
{
	dir = normalize( dir );
	float4 col = 0.;
	float2 ptr = float2( 0, 0 );
	float4 value;
	float4 truefalse;
	int i = 0;
	float minz = 1e20;
	[loop]
	do
	{
		value = tex2D( _GeoTex, ptr );
		truefalse = tex2D( _GeoTex, ptr + float2( _GeoTex_TexelSize.x, 0 ) );
		
		// Ray-Sphere Intersection, but with normalized dir, and 0 offset.
		value.xyz = value.xyz - eye;
		float b = dot( dir, value.xyz ); // Actually would be *2, but we can algebra that out.
		float c = dot( value.xyz, value.xyz ) - value.w;
		float det = b * b - c; // a = 1; b is 1/2 "b"
		
		float radical = sqrt( det ); //Actually 1/2 radical
		float2 ts = .5*b + float2(radical,-radical);
		// T is the computed distance to sphere surface.
		
		//XXX TODO: Catch if ball is behind us. 
		// XXX TODO Check math (maybe it's right? maybe I algebrad wrong.
		det = all( ts < 0 )?-1:det;
		det = all( ts > minz)?-1:det;

		if( det > 0 )
		{
			// Intersection
			
			col += .001;
			
			if( truefalse.x < 0 )
			{
				// Do triangle intersection. If not, set to no intersection.
				float3 N =  tex2D( _GeoTex, ptr + float2( _GeoTex_TexelSize.x * 2, 0.0 ) );
				float4 v0 = tex2D( _GeoTex, ptr + float2( _GeoTex_TexelSize.x * 3, 0.0 ) );
				float4 v1 = tex2D( _GeoTex, ptr + float2( _GeoTex_TexelSize.x * 5, 0.0 ) );
				float4 v2 = tex2D( _GeoTex, ptr + float2( _GeoTex_TexelSize.x * 7, 0.0 ) );
				
				// Compute t and barycentric coordinates using Moller-Trumbore
				// https://tr.inf.unibe.ch/pdf/iam-04-004.pdf
				float3 E1 = v1 - v0;
				float3 E2 = v2 - v0;
				float3 T = eye - v0;
				float3 Q = cross( T, E1 );
				float3 P = cross( dir, E2 );
				float4 tbary = 
					float4(
						1.0/dot(P,E1)*float3(
							dot( Q, E2 ),
							dot( P, T ),
							dot( Q, dir ) ), 0.0 );
				tbary.w = 1.0 - tbary.y - tbary.z;

				if( all( tbary.xyzw >= 0 ) && tbary.x < minz )
				{
					float4 n0 = tex2D( _GeoTex, ptr + float2( _GeoTex_TexelSize.x * 4, 0.0 ) );
					float4 n1 = tex2D( _GeoTex, ptr + float2( _GeoTex_TexelSize.x * 6, 0.0 ) );
					float4 n2 = tex2D( _GeoTex, ptr + float2( _GeoTex_TexelSize.x * 8, 0.0 ) );
					
					float2 uv0 = float2( v0.w, n0.x );
					float2 uv1 = float2( v1.w, n1.x );
					float2 uv2 = float2( v2.w, n2.x );
					float2 uv = uv0 * tbary.w + uv1 * tbary.y + uv2 * tbary.z;
					float3 norm = n0.yzw * tbary.w + n1.yzw * tbary.y + n2.yzw * tbary.z;
					//return float4( tbary.yzw, 1.0 );
					//return float4( norm, 1.0 );
					minz = tbary.x;
					col = tex2D( _EmissionTex, uv );
					
				}
				
				ptr = truefalse.zw;
				
			}
			else
			{
				ptr = truefalse.xy;
			}
		}
		else
		{
			// No intersection
			ptr = truefalse.zw;
		}
	}
	while( i++ < 4096 && ptr.x >= 0 );
	return col;
}
