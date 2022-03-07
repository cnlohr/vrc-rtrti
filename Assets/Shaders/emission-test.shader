Shader "Unlit/emission-test"
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
                float2 uv : TEXCOORD0;
                UNITY_FOG_COORDS(1)
                float4 vertex : SV_POSITION;
            };

			float _MainIntensity;
            sampler2D _MainTex;
            float4 _MainTex_ST;

            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = UnityObjectToClipPos(v.vertex);
                o.uv = TRANSFORM_TEX(v.uv, _MainTex);
                UNITY_TRANSFER_FOG(o,o.vertex);
                return o;
            }

            fixed4 frag (v2f i) : SV_Target
            {
                // sample the texture
                fixed4 col = tex2D(_MainTex, i.uv)*_MainIntensity;
                // apply fog
                UNITY_APPLY_FOG(i.fogCoord, col);
                return col;
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
