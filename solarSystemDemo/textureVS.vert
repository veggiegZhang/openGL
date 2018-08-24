#version 330 core

in vec3 aPosition;
in vec2 aTexCoord;

uniform mat4 uModelViewProjectionMatrix;


out vec2 vTexCoord;

void main()
{
    gl_Position = uModelViewProjectionMatrix * vec4(aPosition, 1.0);
	vTexCoord = aTexCoord;
}

