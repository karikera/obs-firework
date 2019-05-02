
struct VSInput
{
	float4 position : POSITION;
};

struct VSOutput
{
	float4 myPos:SV_POSITION;
	float4 colorVarying:COLOR0;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.myPos = input.position;
	float4 diffuseColor = float4(0.0, 0.0 , 1.0, 0.5);
	output.colorVarying = diffuseColor;
	return output;
}
