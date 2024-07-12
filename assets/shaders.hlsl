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
    
    result.position = position;
    result.color = color;
    result.uv = uv;
    
    return result;
}

float4 PSMain( PSInput input ) : SV_Target
{
    return input.color * g_texture.Sample( g_sampler, input.uv );
}
