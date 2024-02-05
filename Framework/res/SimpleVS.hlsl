struct VSInput
{
    float3 Position : POSITION; // position coordinates
    float4 Color : COLOR; // color of vertex
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

cbuffer Transform : register(b0)
{
    float4x4 World : packoffset(c0); // world matrix
    float4x4 View : packoffset(c4); // view matrix
    float4x4 Proj : packoffset(c8); // projection matrix
};

//--------------------------------------------------------------------------------------------------------
// main entry point of vertex shader
//--------------------------------------------------------------------------------------------------------
VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    
    float4 localPos = float4(input.Position, 1.f);
    float4 worldPos = mul(World, localPos);
    float4 viewPos = mul(View, worldPos);
    float4 projPos = mul(Proj, viewPos);
    
    output.Position = projPos;
    output.Color = input.Color;
    
    return output;
}