struct VSInput {
    float2 inPos;
    float4 inColor;
    float2 inTexCoord;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float4 fragColor;
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
    return vertIn.fragColor * texture.Sample(vertIn.fragTexCoord);
}
