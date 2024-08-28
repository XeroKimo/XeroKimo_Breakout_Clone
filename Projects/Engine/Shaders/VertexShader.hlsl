struct VS_IN
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer PerFrame : register(b0)
{
    
}

cbuffer PerCamera : register(b1)
{
    
}

cbuffer PerMaterial : register(b2)
{
    
}

cbuffer PerObject : register(b3)
{
    float4x4 objectTransform;
}

VS_OUT main(VS_IN input)
{
    VS_OUT output;
    output.position = mul(objectTransform, float4(input.position, 1));
    output.uv = input.uv;
	return output;
}