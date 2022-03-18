#version 400

uniform vec4 xfrm;
attribute vec3 a0;
attribute vec4 a1;
out vec2 tc;

void main()
{
	gl_Position = vec4( a0.xy*xfrm.xy+xfrm.zw, a0.z, 0.5 );
	tc = a1.xy;
}
