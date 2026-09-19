#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) flat in int fragUseTexture;

layout(set = 0, binding = 0) uniform sampler2D fontTexture;

layout(location = 0) out vec4 outColor;

void main()
{
    if (fragUseTexture != 0)
    {
        vec4 texColor = texture(fontTexture, fragUV);
        outColor = vec4(fragColor.rgb, fragColor.a * texColor.r);
    }
    else
    {
        outColor = fragColor;
    }
}
