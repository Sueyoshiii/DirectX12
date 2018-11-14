struct Out
{
    float4 svpos : SV_POSITION;
    float4 pos : POSITION;
    float2 uv : TEXCOORD;
};

Out BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Out o;
    o.svpos = pos;
    o.pos = pos;
    o.uv = uv;

    return o;
}

float4 BasicPS(Out o) : SV_Target
{
    return float4(o.uv, 1, 1);
}
