#version 300 es
precision mediump float;

in vec2 TexCoord;
in vec4 Color;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D NormalSampler;
uniform vec3 LightDirection;

const vec3 LightColor = vec3(1.5, 1.5, 1.5);
const vec3 AmbientColor = vec3(0.0, 0.0, 0.0);

void main()
{
    vec4 tex = texture(texture1, TexCoord);
    vec3 normal = texture(NormalSampler, TexCoord).rgb;
    float lightAmount = max(dot(normal, LightDirection), 0.0);
    vec4 col = Color;
    col.rgb *= AmbientColor + lightAmount * LightColor;
    FragColor = tex * col;
}
