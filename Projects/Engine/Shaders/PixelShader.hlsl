struct VS_OUT
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

float4 main(VS_OUT input) : SV_TARGET
{
	return float4(1.0f, 0.0f, 0.0f, 0.0f);
}