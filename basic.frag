#version 150

in vec3 color_;
in vec2 texcoord_;

out vec4 outColor;

uniform sampler2D tex0;

void main()
{
    outColor = texture(tex0, texcoord_);
}

