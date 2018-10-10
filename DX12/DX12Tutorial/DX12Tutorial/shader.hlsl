Texture2D<float4> tex:register(t0);
SamplerState smp:register(s0);
//matrix mat:register(b0);

cbuffer WVP : register(b0)
{
    matrix world;
    matrix wvp;
}

struct Out {
    //float4 pos : POSITION;
	float4 svpos : SV_POSITION;
	//float4 color : COLOR;
	//float2 uv : TEXCOORD;
    float4 normal : NORMAL;
};

//頂点シェーダ
Out vs( float4 pos : POSITION, /*float2 uv:TEXCOORD*/float4 normal : NORMAL )
{
	Out o;
    
	pos = mul(wvp, pos);
    
	o.svpos = pos;
	//o.color = pos;
	//o.uv = uv;
    o.normal = mul(world, normal);
	return o;
}

//ピクセルシェーダ
float4 ps(Out o) : SV_Target
{
    float3 light = float3(-1, -1, -1);
    //normalizeしないと正確な値が取れない
    light = normalize(light);
    float brightness = saturate(dot(light, o.normal.xyz)) + 0.2f;
	return float4( brightness, brightness, brightness, 1);
}