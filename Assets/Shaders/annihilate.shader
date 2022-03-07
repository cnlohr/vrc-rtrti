Shader "Unlit/annihilate"
{
    Properties
    {
        _MainTex ("Texture", 2D) = "white" {}
		_MainIntensity ("Intensity", float ) = 1.5

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


            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag

            #include "UnityCG.cginc"

            struct appdata
            {
                float4 vertex : POSITION;
            };

            struct v2f
            {
                float4 vertex : SV_POSITION;
            };

			float _MainIntensity;
            sampler2D _MainTex;
            float4 _MainTex_ST;

            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = 0.;
                return o;
            }

            fixed4 frag (v2f i) : SV_Target
            {
                return 0;
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

			float _GIEmissionMux;
			
			float4 frag_meta2 (v2f_meta i) : SV_Target
			{
				// we're interested in diffuse & specular colors,
				// and surface roughness to produce final albedo.
				FragmentCommonData data = UNITY_SETUP_BRDF_INPUT (i.uv);

				UnityMetaInput o;
				UNITY_INITIALIZE_OUTPUT(UnityMetaInput, o);

	
				float4 c = tex2D(_GIEmissionTex, i.uv);
                o.Emission = fixed3(c.rgb * _GIEmissionMux);
				//o.Albedo = o.Emission = 0.;
				return UnityMetaFragment(o);
			}

            ENDCG
        }
    }
}
