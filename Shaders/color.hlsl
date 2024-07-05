
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldProjMat;
}

struct VertexIn
{
    float3 posL : POSITION;
    float4 color : COLOR;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float4 color : COLOR;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    vout.posH = mul(float4(vin.posL, 1.0f), gWorldProjMat);
    
    vout.color = vin.color;
    
    return vout;
}

float4 PS(VertexOut vin) : SV_Target
{
    return vin.color;
}