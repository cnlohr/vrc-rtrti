#version 450

uniform vec4 xfrm;
in vec3 a0;
in vec4 a1;
out vec2 tc;


uniform sampler2D tex;

void main()
{
	int id = int(a0.z);
	int idx = id%256;
	int idy = id/256;
	vec4 fetch = texelFetch( tex, ivec2( idx, idy ), 0 ).rgba;
	if( fetch.b > 0 )
	{
		gl_Position = vec4( 0, 0, 0, 0 );
		tc = a0.xy*xfrm.xy+xfrm.zw;
	}
	else
	{
		vec2 rcoord = (a0.xy*xfrm.xy+xfrm.zw)*.16 + (fetch.xy-vec2(0.5));
		gl_Position = vec4( rcoord, 0, 0.5 );
		tc = a0.xy*xfrm.xy+xfrm.zw;
	}
}
