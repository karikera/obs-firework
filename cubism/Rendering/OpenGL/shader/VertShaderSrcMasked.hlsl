
struct VSInput
{
	float4 position : POSITION;
	float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
	float4 myPos:SV_POSITION;
	float2 texCoord:TEXCOORD0;
	float4 clipPos:TEXCOORD1;
};

cbuffer data
{
	float4x4 u_matrix;
	float4x4 u_clipMatrix;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.myPos = mul(u_matrix, input.position);
	output.clipPos = mul(u_clipMatrix, input.position);
	output.texCoord.x = input.texCoord.x;
	output.texCoord.y = 1.0 - input.texCoord.y;
	return output;
}
