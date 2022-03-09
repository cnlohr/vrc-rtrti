﻿Shader "Unlit/emission-test"
{
	Properties
	{
		// Put one frame latent texture here.
		[ToggleUI] _IsAVProInput("Is AVPro", Int) = 0

		// Put your REAL texture here.  This needs to be asap
		_GIEmissionTex ("Texture GI Emission", 2D) = "white" {}
		_GIEmissionMux ("GI Emission Mux", float) = 10.0 
	}

	CGINCLUDE
		#define UNITY_SETUP_BRDF_INPUT MetallicSetup
	ENDCG

	SubShader
	{

		Tags { "RenderType"="Opaque" "PerformanceChecks"="False"  "LightMode"="ForwardBase"}

		Pass
		{
			AlphaToMask True 
			Cull Off
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			// make fog work
			#pragma multi_compile_fog nometa

			#include "UnityCG.cginc"

			struct appdata
			{
				float4 vertex : POSITION;
				float2 uv : TEXCOORD0;
			};

			struct v2f
			{
				float4 vertex : SV_POSITION;
			};

			int _IsAVProInput;

			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = 0;
				return o;
			}

			fixed4 frag (v2f i) : SV_Target
			{
				return 0.;
			}
			ENDCG
		}
		
		
		

		// ------------------------------------------------------------------
		// Extracts information for lightmapping, GI (emission, albedo, ...)
		// This pass it not used during regular rendering.
		Pass
		{
			Name "META"
			Tags { "LightMode"="Meta" }

			Cull Off

			CGPROGRAM
			#pragma vertex vert_meta
			#pragma fragment frag_meta2

			#pragma shader_feature _EMISSION
			#pragma shader_feature EDITOR_VISUALIZATION
			#pragma shader_feature ___ _DETAIL_MULX2
			
			
			#include "UnityStandardMeta.cginc"
			
			sampler2D _GIEmissionTex;
			int _IsAVProInput;

			float _GIEmissionMux;
			
			float4 frag_meta2 (v2f_meta i) : SV_Target
			{
				// we're interested in diffuse & specular colors,
				// and surface roughness to produce final albedo.
				FragmentCommonData data = UNITY_SETUP_BRDF_INPUT (i.uv);

				UnityMetaInput o;
				UNITY_INITIALIZE_OUTPUT(UnityMetaInput, o);

	
				//float4 c = tex2D(_GIEmissionTex, i.uv);
				float3 col;
				float2 uv = i.uv;
				
				if( _IsAVProInput )
				{
					uv.y = 1.0 - uv.y;
					col = tex2D(_GIEmissionTex, uv);
					col = GammaToLinearSpace( col );
					//e.rgb = pow(e.rgb,2.2);
				}
				else
				{
					col = tex2D(_GIEmissionTex, uv);
				}
				
				
				o.Emission = fixed3(col.rgb * _GIEmissionMux);
				o.Albedo = float4(0,0,0,1);
				return UnityMetaFragment(o);
			}

			ENDCG
		}
	}
}