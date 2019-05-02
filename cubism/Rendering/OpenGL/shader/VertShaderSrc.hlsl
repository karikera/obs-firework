

struct VSInput
{
	float4 position : POSITION;
	float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
	float4 myPos:SV_POSITION;
	float2 texCoord:TEXCOORD0;
};

cbuffer data
{
	float4x4 u_matrix;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.myPos = mul(u_matrix, input.position);
	output.texCoord = input.texCoord;
	output.texCoord.y = 1.0 - input.texCoord.y;
	return output;
}
