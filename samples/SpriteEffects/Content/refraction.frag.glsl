#version 300 es
precision mediump float;

in vec2 TexCoord;
in vec4 Color;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D DisplacementSampler;
uniform vec2 DisplacementScroll;

void main()
{
    vec2 displacement = texture(DisplacementSampler, DisplacementScroll + TexCoord / 3.0).xy;
    vec2 uv = TexCoord + displacement * 0.2 - 0.15;
    FragColor = texture(texture1, uv) * Color;
}
