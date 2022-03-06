Shader "Unlit/rtrti-base"
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
		_NormalizeValue("Normalize Value", float) = 0.0
		_Flip ( "Flip Mirror Enable", float ) = 0.5
		_RoughAdj("Roughness Adjust", float) = 0.3
	}
	SubShader
	{
	
	
	
		Tags { "RenderType"="Opaque" }
		LOD 100

		Pass
		{
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			// make fog work
			#pragma multi_compile_fog

			#include "UnityCG.cginc"

			#include "trace.cginc"
			
			struct appdata
			{
				float4 vertex : POSITION;
				float2 uv : TEXCOORD0;
				float3 normal : NORMAL;
				float4 tangent : TANGENT;
			};

			struct v2f
			{
				float2 uv : TEXCOORD0;
				UNITY_FOG_COORDS(1)
				float4 vertex : SV_POSITION;
				float3 worldPos : TEXCOORD2;
				
				
				float3 tspace0 : TEXCOORD3; // tangent.x, bitangent.x, normal.x
				float3 tspace1 : TEXCOORD4; // tangent.y, bitangent.y, normal.y
				float3 tspace2 : TEXCOORD5; // tangent.z, bitangent.z, normal.z
			};

			sampler2D _BumpMap, _MainTex, _Roughness, _Metallicity;
			float _DiffuseUse, _MediaBrightness;
			float4 _UVScale;
			float _RoughnessIntensity;
			float _NormalizeValue;
			float _RoughAdj;
			float _Flip;

			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = v.uv * _UVScale;
				o.worldPos = mul(unity_ObjectToWorld, v.vertex).xyz;

                half3 wNormal = UnityObjectToWorldNormal(v.normal);
				
				float3 wTangent = UnityObjectToWorldDir(v.tangent.xyz);
				// compute bitangent from cross product of normal and tangent
				float3 tangentSign = v.tangent.w * unity_WorldTransformParams.w;
				float3 wBitangent = cross(wNormal, wTangent) * tangentSign;
				// output the tangent space matrix
				o.tspace0 = float3(wTangent.x, wBitangent.x, wNormal.x);
				o.tspace1 = float3(wTangent.y, wBitangent.y, wNormal.y);
				o.tspace2 = float3(wTangent.z, wBitangent.z, wNormal.z);
				
				UNITY_TRANSFER_FOG(o,o.vertex);
				return o;
			}

			fixed4 frag (v2f i) : SV_Target
			{
				float3 tnormal = UnpackNormal(tex2D(_BumpMap, i.uv));
				tnormal.z+=_NormalizeValue;
				tnormal = normalize(tnormal);
	            half3 worldNormal;
                worldNormal.x = dot(i.tspace0, tnormal);
                worldNormal.y = dot(i.tspace1, tnormal);
                worldNormal.z = dot(i.tspace2, tnormal);

				float3 worldViewDir = normalize(UnityWorldSpaceViewDir(i.worldPos));
				float3 worldRefl = reflect(-worldViewDir, worldNormal);
				

				float z;
				float2 uvo;
				float4 col = CoreTrace( i.worldPos, worldRefl, z, uvo ) * _MediaBrightness;
				if( uvo.x > 1.0 ) col = 0.0;
				
				// Test if we need to reverse-cast through a mirror.
				if( _Flip > 0.5 )
				{
					float3 mirror_pos = float3( -12, 1.5, 0 );
					float3 mirror_size = float3( 0, 3, 9 );
					float3 mirror_n = float3( 1, 0, 0 );

					float3 revray = float3( -worldRefl.x, worldRefl.yz );
					float3 revpos = i.worldPos;
					
					// Make sure this ray intersects the mirror.
					float mirrort = dot( mirror_pos - i.worldPos, mirror_n ) / dot( worldRefl, mirror_n );
					float3 mirrorp = i.worldPos + worldRefl * mirrort;
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
				
				//col.rgb = float3( uvo.xy, 0.0 );

				if( z > 1e10 )
					col = UNITY_SAMPLE_TEXCUBE(unity_SpecCube0, worldNormal )*.5;
				
				float rough = 1.0-tex2D(_Roughness, i.uv)*_RoughnessIntensity;
				rough *= _RoughAdj;
				col = (1.-rough)*col + tex2D(_MainTex, i.uv)*_DiffuseUse*rough;
				
				UNITY_APPLY_FOG(i.fogCoord, col);
				return col;
			}
			ENDCG
		}
	}
}
