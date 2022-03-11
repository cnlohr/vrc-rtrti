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
		_DiffuseShift( "Diffuse Shift", float) = 0.0
		_MediaBrightness( "Media Brightness", float ) = 1.2
		_RoughnessIntensity( "Roughness Intensity", float ) = 3.0
		_RoughnessShift( "Roughness Shift", float ) = 0.0
		_NormalizeValue("Normalize Value", float) = 0.0
		_RoughAdj("Roughness Adjust", float) = 0.3
		_Flip("Flip Mirror Enable", float ) =0.0
		_MirrorPlace("Mirror Place", Vector ) = ( 0, 0, 0, 0)
		_MirrorScale("Mirror Enable", Vector ) = ( 0, 0, 0, 0)
		_MirrorRotation("Mirror Rotation", Vector ) = ( 0, 0, 0, 0)
		_OverrideReflection( "Override Reflection", float)=0.0
		_SkyboxBrightness("Skybox Brightness", float) = 1.0
		_AlbedoBoost("Albedo Boost", float) = 1.0
		_MetallicMux("Metallic Mux", float) = 1.0
		_MetallicShift("Metallic Shift", float) = 0.0
		_SmoothnessMux("Smooth Mux", float) = 1.0
		_SmoothnessShift("Smooth Shift", float) = 0.0
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
		float _DiffuseUse, _DiffuseShift, _MediaBrightness;
		float _RoughnessIntensity, _RoughnessShift;
		float _NormalizeValue;
		float _RoughAdj;
		float _Flip;
		float4 _Ambient;
		float3 _MirrorPlace;
		float3 _MirrorScale;
		float4 _MirrorRotation;
		float _OverrideReflection;
		float _SkyboxBrightness;
		float _AlbedoBoost;
		float _MetallicMux;
		float _MetallicShift;
		float _SmoothnessMux;
		float _SmoothnessShift;

		#include "trace.cginc"

        struct Input
        {
            float2 uv_MainTex;
			float3 worldPos;
			float3 worldNormal;
			float3 worldRefl;
			INTERNAL_DATA
        };

		//https://community.khronos.org/t/quaternion-functions-for-glsl/50140/2
		float3 qtransform( in float4 q, in float3 v )
		{
			return v + 2.0*cross(cross(v, q.xyz ) + q.w*v, q.xyz);
		}
		

		// Next two https://gist.github.com/mattatz/40a91588d5fb38240403f198a938a593
		float4 q_conj(float4 q)
		{
			return float4(-q.x, -q.y, -q.z, q.w);
		}

		// https://jp.mathworks.com/help/aeroblks/quaternioninverse.html
		float4 q_inverse(float4 q)
		{
			float4 conj = q_conj(q);
			return conj / (q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
		}
		
        void surf (Input IN, inout SurfaceOutputStandard o)
        {
            // Albedo comes from a texture tinted by color
            fixed4 c = tex2D (_MainTex, IN.uv_MainTex) * _AlbedoBoost;
			o.Normal = UnpackNormal (tex2D (_BumpMap, IN.uv_MainTex));
			o.Normal.z+=_NormalizeValue;
			o.Normal = normalize(o.Normal);
			float3 worldNormal = WorldNormalVector (IN, o.Normal);

			float3 worldViewDir = normalize(UnityWorldSpaceViewDir(IN.worldPos));
			float3 worldRefl = reflect(-worldViewDir, worldNormal);
			float4 col = 0.;

			float z;
			float2 uvo = 0.0;
			float epsilon = 0.00;
			col = CoreTrace( IN.worldPos+worldRefl*epsilon, worldRefl, z, uvo ) * _MediaBrightness;
			if( uvo.x > 1.0 ) col = 0.0;

			float3 debug = 0.0;
			// Test if we need to reverse-cast through a mirror.
			if( _Flip > 0.5 ) {
			//if( 0 ) {
			//if( 1 ){
				debug = 0;
				float3 mirror_pos = _MirrorPlace;//float3( -12, 1.5, 0 );
				float3 mirror_size = qtransform( q_inverse(_MirrorRotation), _MirrorScale );
				float3 mirror_n = qtransform( q_inverse(_MirrorRotation), float3( 0, 0, -1 ) );

				float3 revray = reflect( worldRefl, mirror_n ); //float3( -worldRefl.x, worldRefl.yz );
				float3 revpos = IN.worldPos;
				
				// Make sure this ray intersects the mirror.
				float mirrort = dot( mirror_pos - IN.worldPos, mirror_n ) / dot( worldRefl, mirror_n );
				float3 mirrorp = IN.worldPos + worldRefl * mirrort;
				
				float3 relative_intersection = qtransform( q_inverse(_MirrorRotation),(mirrorp));
				if( _OverrideReflection > 0.5 || all( abs( relative_intersection.xy ) - mirror_size.zy/2 < 0 ) )
				{
					//revpos.x = -(revpos.x - mirror_pos) + mirror_pos;
					revpos = mirror_pos+reflect( revpos - mirror_pos, mirror_n );

					float z2;
					uvo = 0.0;
					float4 c2 = CoreTrace( revpos+revray*epsilon, revray, z2, uvo ) * _MediaBrightness;
					if( uvo.x > 1.0 ) c2 = 0.0;
					if( z2 < z )
					{
						col = c2;
						z = z2;
					}
				}
			}
				
			if( z > 1e10 )
				col = UNITY_SAMPLE_TEXCUBE(unity_SpecCube0, worldRefl )*_SkyboxBrightness;
			
			if( length( debug )> 0.0 ) col.rgb = debug;			
			
			float rough = 1.0-tex2D(_Roughness, IN.uv_MainTex)*_RoughnessIntensity + _RoughnessShift;
			rough *= _RoughAdj;
			rough = saturate( rough );
			col = (1.-rough)*col;

			//c = 1.0;
            o.Albedo = (c.rgb-_DiffuseShift)*(_DiffuseUse);
            o.Metallic = tex2D (_Metallicity, IN.uv_MainTex) * _MetallicMux + _MetallicShift;
            o.Smoothness = tex2D (_Roughness, IN.uv_MainTex) * _SmoothnessMux + _SmoothnessShift;
			o.Emission = max(col,0) + c.rgb * _Ambient;
            o.Alpha = c.a;
        }
        ENDCG
    }
    FallBack "Diffuse"
}
