
struct VSOutput
{
	float4 myPos:SV_POSITION;
	float2 texCoord:TEXCOORD0;
};

cbuffer data
{
	float4 u_channelFlag;
	float4 u_baseColor;
};

Texture2D u_texture0;
SamplerState u_sampler;

float4 main(VSOutput input):SV_TARGET0
{
	float4 normpos = input.myPos / input.myPos.w;

	float isInside = step(u_baseColor.x, normpos.x) 
		* step(u_baseColor.y, normpos.y)
		* step(normpos.x, u_baseColor.z)
		* step(normpos.y, u_baseColor.w);

	return u_channelFlag * u_texture0.Sample(u_sampler, input.texCoord).a * isInside;
}
