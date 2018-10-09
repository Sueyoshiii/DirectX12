Texture2D<float4> tex:register(t0);
SamplerState smp:register(s0);
matrix mat:register(b0);

struct Out {
    float4 pos : POSITION;
	float4 svpos : SV_POSITION;
	//float4 color : COLOR;
	//float2 uv : TEXCOORD;
    float4 normal : NORMAL;
};

//頂点シェーダ
Out vs( float4 pos : POSITION, /*float2 uv:TEXCOORD*/float4 normal : NORMAL )
{
	Out o;
	pos = mul(mat, pos);
    normal = mul(mat, normal);

    o.pos = pos;
	o.svpos = pos;
	//o.color = pos;
	//o.uv = uv;
    o.normal = normal;
	return o;
}

//ピクセルシェーダ
float4 ps(Out o) : SV_Target
{
	//return float4(o.uv,1,1);
	return float4( o.normal.rgb, 1);
}