
struct VSOutput
{
	float4 myPos:SV_POSITION;
	float4 colorVarying:COLOR0;
};

float4 main(VSOutput input):SV_TARGET0
{
	return input.colorVarying;
}
