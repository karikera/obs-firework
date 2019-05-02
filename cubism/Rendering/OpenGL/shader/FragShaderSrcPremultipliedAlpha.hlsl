
struct VSOutput
{
	float4 myPos:SV_POSITION;
	float2 texCoord:TEXCOORD0;
};

Texture2D s_texture0; //_MainTex
SamplerState s_sampler;

cbuffer data
{
	float4 u_baseColor; //v2f.color
};

float4 main(VSOutput input):SV_TARGET0
{
	return s_texture0.Sample(s_sampler, input.texCoord) * u_baseColor;
}
