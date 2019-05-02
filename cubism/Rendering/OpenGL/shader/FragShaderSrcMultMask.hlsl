
struct VSOutput
{
	float4 myPos:SV_POSITION;
	float2 texCoord:TEXCOORD0;
	float4 clipPos:TEXCOORD1;
};

Texture2D s_texture0;
Texture2D s_texture1;
SamplerState s_sampler;

cbuffer data
{
	float4 u_channelFlag;
	float4 u_baseColor;
};


float4 main(VSOutput input):SV_TARGET0
{
	float4 col_formask = s_texture0.Sample(s_sampler, input.texCoord) * u_baseColor;
	col_formask.rgb = col_formask.rgb  * col_formask.a ;
	float4 clipMask = (1.0 - s_texture1.Sample(s_sampler, input.clipPos.xy / input.clipPos.w)) * u_channelFlag;
	float maskVal = clipMask.r + clipMask.g + clipMask.b + clipMask.a;
	col_formask = col_formask * maskVal;
	return col_formask;
}
