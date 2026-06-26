#version 300 es
precision mediump float;

in vec2 TexCoord;
in vec4 Color;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D OverlaySampler;
uniform vec2 OverlayScroll;

void main()
{
    vec4 tex = texture(texture1, TexCoord);
    float fadeSpeed = texture(OverlaySampler, OverlayScroll + TexCoord).x;
    tex *= clamp((Color.a - fadeSpeed) * 2.5 + 1.0, 0.0, 1.0);
    FragColor = tex;
}
