Shader "Unlit/CombinedReflect"
{
	Properties
	{
		_Tex0("Texture 0", 2D) = "white" {}
		_Tex1("Texture 1", 2D) = "white" {}
		_Tex2("Texture 2", 2D) = "white" {}
		_Tex3("Texture 3", 2D) = "white" {}
		_Tex4("Texture 4", 2D) = "white" {}
		_Tex5("Texture 5", 2D) = "white" {}
		_Tex6("Texture 6", 2D) = "white" {}
		_Tex7("Texture 7", 2D) = "white" {}
	 }

	 SubShader
	 {
		Lighting Off
		Blend One Zero

		Pass
		{
			CGPROGRAM
			#include "UnityCustomRenderTexture.cginc"
			#pragma vertex CustomRenderTextureVertexShader
			#pragma fragment frag
			#pragma target 3.0

			float4	  _Color;
			sampler2D   _Tex0;
			sampler2D   _Tex1;
			sampler2D   _Tex2;
			sampler2D   _Tex3;
			sampler2D   _Tex4;
			sampler2D   _Tex5;
			sampler2D   _Tex6;
			sampler2D   _Tex7;

			float4 frag(v2f_customrendertexture IN) : COLOR
			{
				float2 nruv = float2( IN.localTexcoord.xy * float2( 8, 1 ) );
				int select = floor( nruv.x );
				nruv = frac( nruv );
				switch( select )
				{
					case 0: return tex2D(_Tex0, nruv );
					case 1: return tex2D(_Tex1, nruv );
					case 2: return tex2D(_Tex2, nruv );
					case 3: return tex2D(_Tex3, nruv );
					case 4: return tex2D(_Tex4, nruv );
					case 5: return tex2D(_Tex5, nruv );
					case 6: return tex2D(_Tex6, nruv );
					case 7: return tex2D(_Tex7, nruv );
				}
				return 0;
			}
			ENDCG
			}
	}
}