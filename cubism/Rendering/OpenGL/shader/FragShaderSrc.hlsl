
struct VSOutput
{
	float4 myPos:SV_POSITION;
	float2 texCoord:TEXCOORD0;
};

cbuffer data
{
	float4 u_baseColor;
};

Texture2D s_texture0; //_MainTex
SamplerState s_sampler;

float4 main(VSOutput input):SV_TARGET0
{
	float4 color = s_texture0.Sample(s_sampler, input.texCoord) * u_baseColor;
	return float4(color.rgb * color.a,  color.a);
}
