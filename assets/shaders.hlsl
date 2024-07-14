cbuffer SceneConstantBuffer : register( b0 )
{
    float4x4 model;
    float4x4 viewProj;
    float4 padding[ 8 ];
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
    float2 uv       : TEXCOORD;
};

Texture2D g_texture : register( t0 );
SamplerState g_sampler : register( s0 );

PSInput VSMain( float4 position : POSITION, float4 color : COLOR, float4 uv : TEXCOORD )
{
    PSInput result;
    
    float4x4 modelViewProj = mul( model, viewProj );
    
    result.position = mul( float4( position.xyz, 1.0 ), modelViewProj );
    result.color = color;
    result.uv = uv;
    
    return result;
}

float4 PSMain( PSInput input ) : SV_Target
{
    return input.color * g_texture.Sample( g_sampler, input.uv );
}
