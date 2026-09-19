#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;

layout(push_constant) uniform PushConstants {
    vec2 viewportSize;
    int useTexture;
} pushConstants;

layout(set = 0, binding = 0) uniform sampler2D fontTexture;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) flat out int fragUseTexture;

void main()
{
    vec2 ndc = (position / pushConstants.viewportSize) * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    fragUV = uv;
    fragColor = color;
    fragUseTexture = pushConstants.useTexture;
}
