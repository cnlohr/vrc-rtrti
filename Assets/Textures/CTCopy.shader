Shader "CustomRenderTexture/CTCopy"
{
    Properties
    {
        _Color ("Color", Color) = (1,1,1,1)
        _Tex("InputTex", 2D) = "white" {}
		[ToggleUI] _IsAVProInput("Is AVPro", float) = 0
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

			float _IsAVProInput;
            float4      _Color;
            sampler2D   _Tex;

            float4 frag(v2f_customrendertexture IN) : COLOR
            {
				float2 uv = IN.localTexcoord.xy;
				float3 col;

				if( _IsAVProInput )
				{
					uv.y = 1.0 - uv.y;
					col = tex2D(_Tex, uv);
					col = GammaToLinearSpace( col );
					//col.rgb = pow(col.rgb,1.4);
				}
				else
				{
					col = tex2D(_Tex, uv);
				}
				return _Color * float4( col, 1.0 );
            }
            ENDCG
            }
    }
}