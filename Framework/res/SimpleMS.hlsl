//////////////////////////////////////////////////////////////////////////////////////////////////////////
// MSInput structure
//////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MSInput
{
    float3 Position; // position coordinates
    float4 Color; // color of vertices
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// MSOutput structure
//////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MSOutput
{
    float4 Position : SV_POSITION; // position coordinates
    float4 Color : COLOR; // color of vertices
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// TransformParam structure
//////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TransformParam
{
    float4x4 World; // world matrix
    float4x4 View; // view matrix
    float4x4 Proj; // projection matrix
};

//--------------------------------------------------------------------------------------------------------
// Resources
//--------------------------------------------------------------------------------------------------------
StructuredBuffer<MSInput> Vertices : register(t0);
StructuredBuffer<uint3> Indices : register(t1);
ConstantBuffer<TransformParam> Transform : register(b0);

//--------------------------------------------------------------------------------------------------------
//   main entry point of meshshader
//--------------------------------------------------------------------------------------------------------
[numthreads(64, 1, 1)]
[outputtopology("triangle")]
void main
(
    uint3 threadIndex : SV_DispatchThreadID,
    out vertices MSOutput verts[3],
    out indices uint3 tris[1]
)
{
    // set the number of vertices and primitives of thread group
    SetMeshOutputCounts(3, 1);
    
    // set vertex index
    if (threadIndex.x < 1)
    {
        tris[threadIndex.x] = Indices[threadIndex.x];
    }
    
    // set vertex data
    if (threadIndex.x < 3)
    {
        MSOutput output = (MSOutput) 0;
        
        float4 localPos = float4(Vertices[threadIndex.x].Position, 1.f);
        float4 worldPos = mul(Transform.World, localPos);
        float4 viewPos = mul(Transform.View, worldPos);
        float4 projPos = mul(Transform.Proj, viewPos);
        
        output.Position = projPos;
        output.Color = Vertices[threadIndex.x].Color;
        
        verts[threadIndex.x] = output;
    }
}