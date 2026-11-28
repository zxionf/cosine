struct VSInput {
    float2 inPos;
    float4 inColor;
    float2 inTexCoord;
    // 解析形状：half = 未含圆角的外半尺寸；extra = (radius, thickness)。
    float2 inHalf;
    float2 inExtra;
    // 图元种类：0 sprite/text，1 圆角矩形，2 圆角环，3 MTSDF 图集。
    nointerpolation uint inKind;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float4 fragColor;
    float2 fragTexCoord;
    float2 fragHalf;
    float2 fragExtra;
    nointerpolation uint fragKind;
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
    // 1 / ui_scale：把 1 物理像素换成逻辑像素（SDF 的 AA 宽度下限）。
    float inv_ui_scale;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.pos = float4(input.inPos * pc.scale + pc.translate, 0.0, 1.0);
    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    output.fragHalf = input.inHalf;
    output.fragExtra = input.inExtra;
    output.fragKind = input.inKind;
    return output;
}

Sampler2D texture;

// ── SDF 图元 ──────────────────────────────────────────────────────
//
// 与 CPU 侧 box 几何一一对应：圆角矩形 / 圆角环 / MTSDF 图集。
// 覆盖率都按「1 物理像素」过渡：fwidth 给的是每屏幕像素的距离变化，
// 顶点已经是逻辑坐标，所以换 DPI 不需要额外参数（只有兜底用 inv_ui_scale）。

float sd_rounded_box(float2 p, float2 half, float r) {
    float2 q = abs(p) - half + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

// 三通道取中位数（MSDF 重建）；HLSL 没有内置 median。
float msdf_median(float3 c) {
    return max(min(c.r, c.g), min(max(c.r, c.g), c.b));
}

// 预乘管线：覆盖率要同时乘进 rgb 与 a，否则半透明边缘会发亮。
float4 premultiply_coverage(float4 color, float coverage) {
    return float4(color.rgb * coverage, color.a * coverage);
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    if (vertIn.fragKind == 0u) {
        return vertIn.fragColor * texture.Sample(vertIn.fragTexCoord);
    }

    // MTSDF 图集：uv 采样 → median 重建距离 → 1px 过渡。
    if (vertIn.fragKind == 3u) {
        float4 tex = texture.Sample(vertIn.fragTexCoord);
        float sd = msdf_median(tex.rgb);
        float w = max(fwidth(sd), 1e-5);
        float coverage = smoothstep(0.5 - w, 0.5 + w, sd);
        return premultiply_coverage(vertIn.fragColor, coverage);
    }

    // 解析形状：uv 是局部坐标（像素），half/radius/thickness 来自顶点。
    float2 p = vertIn.fragTexCoord;
    float d = sd_rounded_box(p, vertIn.fragHalf, vertIn.fragExtra.x);
    if (vertIn.fragKind == 2u) {
        float thickness = max(vertIn.fragExtra.y, 0.0);
        float2 inner_half = vertIn.fragHalf - thickness;
        float inner_radius = max(vertIn.fragExtra.x - thickness, 0.0);
        float inner = sd_rounded_box(p, inner_half, inner_radius);
        d = max(d, -inner);
    }

    float width = max(fwidth(d), 0.5 * pc.inv_ui_scale);
    float coverage = saturate(0.5 - d / width);
    return premultiply_coverage(vertIn.fragColor, coverage);
}
