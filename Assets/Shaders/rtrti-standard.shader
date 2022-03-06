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
		_Flip("Flip Mirror Enable", float ) =0.0
		[HDR] _Ambient("Ambient Color", Color) = (0.1,0.1,0.1,1.0)
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
		float _Flip;
		float4 _Ambient;

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

				
			// Test if we need to reverse-cast through a mirror.
			if( _Flip > 0.5 )
			{
				float3 mirror_pos = float3( -12, 1.5, 0 );
				float3 mirror_size = float3( 0, 3, 9 );
				float3 mirror_n = float3( 1, 0, 0 );

				float3 revray = float3( -worldRefl.x, worldRefl.yz );
				float3 revpos = IN.worldPos;
				
				// Make sure this ray intersects the mirror.
				float mirrort = dot( mirror_pos - IN.worldPos, mirror_n ) / dot( worldRefl, mirror_n );
				float3 mirrorp = IN.worldPos + worldRefl * mirrort;
				if( all( abs( mirrorp.yz - mirror_pos.yz ) - mirror_size.yz/2 < 0 ) )
				{
				
					revpos.x = -(revpos.x - mirror_pos) + mirror_pos;

					float z2;
					float4 c2 = CoreTrace( revpos, revray, z2, uvo ) * _MediaBrightness;
					if( z2 < z )
					{
						col = c2;
						z = z2;
					}
				}
			}
				
			if( z > 1e10 )
				col = UNITY_SAMPLE_TEXCUBE(unity_SpecCube0, worldNormal )*.5;
			
			float rough = 1.0-tex2D(_Roughness, IN.uv_MainTex)*_RoughnessIntensity + _RoughnessShift;
			rough *= _RoughAdj;
			rough = saturate( rough );
			col = (1.-rough)*col;

			//c = 1.0;
            o.Albedo = c.rgb*(_DiffuseUse);
            o.Metallic = tex2D (_Metallicity, IN.uv_MainTex);
            o.Smoothness = tex2D (_Roughness, IN.uv_MainTex);
			o.Emission = max(col,0) + c.rgb * _Ambient;
            o.Alpha = c.a;
        }
        ENDCG
    }
    FallBack "Diffuse"
}
