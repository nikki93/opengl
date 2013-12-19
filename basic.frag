#version 150

in vec3 color_;
in vec2 texcoord_;

out vec4 outColor;

uniform sampler2D tex0;
uniform sampler2D tex1;

void main()
{
    vec4 col0 = texture(tex0, texcoord_);
    vec4 col1 = texture(tex1, texcoord_);
    outColor = mix(col0, col1, 0.5);
}

