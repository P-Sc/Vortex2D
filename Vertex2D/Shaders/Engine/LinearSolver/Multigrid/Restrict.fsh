#version 150

precision highp float;

in vec2 v_texCoord;

uniform sampler2D u_texture;

out vec4 colour_out;

void main()
{
    float p = texture(u_texture, v_texCoord).y;

    colour_out = vec4(p, 0.0, 0.0, 0.0);
}