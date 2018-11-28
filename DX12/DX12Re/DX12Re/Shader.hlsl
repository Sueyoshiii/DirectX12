Texture2D<float4> tex : register(t0);

SamplerState smp : register(s0);

cbuffer Mat : register(b0)
{
    matrix world;
    matrix wvp;
}

struct Out
{
    float4 svpos : SV_POSITION;
    float4 pos : POSITION;
    float4 normal : NORMAL;
    //float2 uv : TEXCOORD;

};

Out BasicVS(float4 pos : POSITION, float4 normal : NORMAL/*, float2 uv : TEXCOORD*/)
{
    Out o;
    o.svpos = mul(wvp, pos);
    o.pos = pos;
    o.normal = mul(world, normal);
    //o.uv = uv;

    return o;
}

float4 BasicPS(Out o) : SV_Target
{
    float3 light = float3(-1, -1, -1);
    light = normalize(light);
    float brightness = dot(o.normal.xyz, light);

    //return float4(tex.Sample(smp, o.uv).rgb, 1);
    return float4(brightness, brightness, brightness, 1);
}
