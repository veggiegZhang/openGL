#version 330 core


in vec2 vTexCoord;
uniform sampler2D uTextureSampler;
uniform float uAlpha;

out vec3 fColor;

void main()
{
	fColor = texture(uTextureSampler, vTexCoord).rgb * uAlpha;
}