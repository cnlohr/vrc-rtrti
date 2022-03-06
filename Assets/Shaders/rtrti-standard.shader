Shader "Custom/rtrti-standard"
{
    Properties
    {
		_EmissionTex ("Emission Texture", 2D) = "white" {}
		_MainTex("Main Texture", 2D) = "white" {}
		_Roughness("Roughness", 2D) = "white" {}
		_Metallicity("Metallicity", 2D) = "white" {}
		_BumpMap("Normal Map", 2D) = "bump" {}
		_GeoTex ("Geometry", 2D) = "black" {}
		_DiffuseUse( "Diffuse Use", float ) = .1
		_MediaBrightness( "Media Brightness", float ) = 1.2
		_UVScale(  "UV Scale", Vector ) = ( 2, 2, 0, 0 )
		_RoughnessIntensity( "Roughness Intensity", float ) = 3.0
		_RoughnessShift( "Roughness Shift", float ) = 0.0
		_NormalizeValue("Normalize Value", float) = 0.0
		_RoughAdj("Roughness Adjust", float) = 0.3
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 200

        CGPROGRAM
        // Physically based Standard lighting model, and enable shadows on all light types
        #pragma surface surf Standard

        // Use shader model 3.0 target, to get nicer looking lighting
        #pragma target 5.0
		

		sampler2D _BumpMap, _MainTex, _Roughness, _Metallicity;
		float _DiffuseUse, _MediaBrightness;
		float4 _UVScale;
		float _RoughnessIntensity, _RoughnessShift;
		float _NormalizeValue;
		float _RoughAdj;

		#include "trace.cginc"

        struct Input
        {
            float2 uv_MainTex;
			float3 worldPos;
			float3 worldNormal;
			float3 worldRefl;
			INTERNAL_DATA
        };

        void surf (Input IN, inout SurfaceOutputStandard o)
        {
            // Albedo comes from a texture tinted by color
            fixed4 c = tex2D (_MainTex, IN.uv_MainTex);
			o.Normal = UnpackNormal (tex2D (_BumpMap, IN.uv_MainTex));
			o.Normal.z+=_NormalizeValue;
			o.Normal = normalize(o.Normal);
			float3 worldNormal = WorldNormalVector (IN, o.Normal);

			float3 worldViewDir = normalize(UnityWorldSpaceViewDir(IN.worldPos));
			float3 worldRefl = reflect(-worldViewDir, worldNormal);


			float z;
			float2 uvo;
			float4 col = CoreTrace( IN.worldPos, worldRefl, z, uvo ) * _MediaBrightness;
			if( uvo.x > 1.0 ) col = 0.0;
			
//			if( z > 1e10 )
//				col = UNITY_SAMPLE_TEXCUBE(unity_SpecCube0, worldNormal )*.5;
			
			float rough = 1.0-tex2D(_Roughness, IN.uv_MainTex)*_RoughnessIntensity + _RoughnessShift;
			rough *= _RoughAdj;
			col = (1.-rough)*col;

			//c = 1.0;
            o.Albedo = c.rgb*_DiffuseUse;
            o.Metallic = tex2D (_Metallicity, IN.uv_MainTex);
            o.Smoothness = tex2D (_Roughness, IN.uv_MainTex);
			o.Emission = col;
            o.Alpha = c.a;
        }
        ENDCG
    }
    FallBack "Diffuse"
}
