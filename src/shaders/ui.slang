struct VSInput {
    float2 inPos;
    float3 inColor;
    float2 inTexCoord;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float3 fragColor;
    float2 fragTexCoord;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
ConstantBuffer<UniformBuffer> ubo;

struct PushConstants
{
    float2 scale;
    float2 translate;
    float time;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.pos = float4(input.inPos * pc.scale + pc.translate, 0.0, 1.0);
    // output.pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.inPos, 0.0, 1.0))));
    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}

Sampler2D texture;
[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    float2 center = float2(0.5, 0.5);
    float2 delta = vertIn.fragTexCoord - center;
    float dist = length(delta);
    float2 dir = delta / max(dist, 1e-5); // 避免除以零

    float frequency = 30.0; // 波纹密度
    float speed = 5.0;      // 传播速度
    float amplitude = 0.02; // UV 偏移幅度

    float wave = sin(dist * frequency - pc.time * speed);

    float attenuation = exp(-dist * 3.0);
    wave *= attenuation;

    float2 uv = vertIn.fragTexCoord + dir * wave * amplitude;


    return texture.Sample(uv);
}
