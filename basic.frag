#version 150

in vec2 texcoord_;

uniform sampler2D tex0;

out vec4 outColor;

void main()
{
    outColor = texture(tex0, texcoord_);
}

