#version 450

in vec2 tc;
out vec4 fragcolor;

uniform sampler2D tex;


void main() {
	int x, y;
	vec4 sum = vec4( 0.0 );
//	for( y = 0; y < 256; y++ )
//	for( x = 0; x < 512; x++ )
//	{
//		//sum += texelFetch( tex, ivec2( x, y ), 0 ).rgba;	
//	}

	float intensity = 1.0 / (2.0*3.14159) * exp( -( dot(tc.xy,tc.xy)*10.0 ) );
	sum.r = intensity*.1;
	sum.a = 0.0;
	fragcolor = sum;
}
