#version 300 es
precision mediump float;

in vec2 TexCoord;
in vec4 Color;

out vec4 FragColor;

uniform sampler2D texture1;

void main()
{
    vec4 tex = texture(texture1, TexCoord);
    float greyscale = dot(tex.rgb, vec3(0.3, 0.59, 0.11));
    tex.rgb = mix(vec3(greyscale), tex.rgb, Color.a * 4.0);
    FragColor = tex;
}
