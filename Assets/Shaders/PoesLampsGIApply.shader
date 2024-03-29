// Unity built-in shader source. Copyright (c) 2016 Unity Technologies. MIT license (see license.txt)

Shader "Custom/PoesGIApply"
{
    Properties
    {
        _MainTex ("Texture Reg", 2D) = "white" {}
		_GIAlbedoTex ("Texture GI Albedo", 2D) = "white" {}
		_GIEmissionTex ("Texture GI Emission", 2D) = "white" {}
		_GIEmissionMux ("GI Emission Mux", float) = 10.0 
		_GIAlbedoMux ("GI Emission Mux", float) = 10.0 
    }

    CGINCLUDE
        #define UNITY_SETUP_BRDF_INPUT MetallicSetup
    ENDCG

    SubShader
    {
        Tags { "RenderType"="Opaque" "PerformanceChecks"="False" }
        LOD 100

        Pass
        {
			AlphaToMask True 
            CGPROGRAM


            #pragma vertex vert
            #pragma fragment frag
            // make fog work
            #pragma multi_compile_fog
            
            #include "UnityCG.cginc"

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
                float2 uv1 : TEXCOORD0;
                float2 uv2 : TEXCOORD1;
                float2 uv3 : TEXCOORD2;
                float2 uv4 : TEXCOORD3;
            };

            struct v2f
            {
                float2 uv : TEXCOORD0;
				float4 color : TEXCOORD1;
                UNITY_FOG_COORDS(1)
                float4 vertex : SV_POSITION;
            };

            sampler2D _MainTex;
            float4 _MainTex_ST;
            sampler2D _GIAlbedoTex;
			sampler2D _GIEmissionTex;
            fixed4 _GIAlbedoTex_ST;
            float _GIAlbedoMux;
			float _GIEmissionMux;

            v2f vert (appdata v)
            {
                v2f o;
				float4 worldspace = mul( unity_ObjectToWorld, v.vertex);
                o.vertex = UnityObjectToClipPos(v.vertex);
                o.uv = TRANSFORM_TEX(v.uv, _MainTex);
				float3 origpos = float3( v.uv3, v.uv4.x );
				origpos = origpos.xyz;
				o.color = float4( worldspace.xyz, 1 ); //Not actually used, since they're a static mesh.
                UNITY_TRANSFER_FOG(o,o.vertex);
				//o.vertex = 0;
                return o;
            }
            
            fixed4 frag (v2f i) : SV_Target
            {

                // sample the texture
                fixed4 col = tex2D(_GIEmissionTex, i.uv);
				//col = float4( 1.-i.uv, 0., 1. );
				col.a = 1.;
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

            #include "UnityStandardMeta.cginc"
			
            sampler2D _GIAlbedoTex;
			sampler2D _GIEmissionTex;
            fixed4 _GIAlbedoTex_ST;
            float _GIAlbedoMux;
			float _GIEmissionMux;
			
			
						
			float4 frag_meta2 (v2f_meta i) : SV_Target
			{
				// we're interested in diffuse & specular colors,
				// and surface roughness to produce final albedo.
				FragmentCommonData data = UNITY_SETUP_BRDF_INPUT (i.uv);

				UnityMetaInput o;
				UNITY_INITIALIZE_OUTPUT(UnityMetaInput, o);

               fixed4 c = tex2D (_GIAlbedoTex, i.uv);
                o.Albedo = fixed3(c.rgb * _GIAlbedoMux);

				c = tex2D(_GIEmissionTex, i.uv);
                o.Emission = fixed3(c.rgb * _GIEmissionMux);

				return UnityMetaFragment(o);
			}

            ENDCG
        }
    }

}
