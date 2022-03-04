Shader "Unlit/rtrti-base"
{
	Properties
	{
		_MainTex ("Texture", 2D) = "white" {}
		_BumpMap("Normal Map", 2D) = "bump" {}
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

			sampler2D _MainTex;
			sampler2D _BumpMap;

			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = v.uv;
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
	            half3 worldNormal;
                worldNormal.x = dot(i.tspace0, tnormal);
                worldNormal.y = dot(i.tspace1, tnormal);
                worldNormal.z = dot(i.tspace2, tnormal);

				float3 worldViewDir = normalize(UnityWorldSpaceViewDir(i.worldPos));
				float3 worldRefl = reflect(-worldViewDir, worldNormal);
				
				fixed4 col = tex2D(_MainTex, i.uv);
				
				col.rgb = worldRefl;
				UNITY_APPLY_FOG(i.fogCoord, col);
				return col;
			}
			ENDCG
		}
	}
}
