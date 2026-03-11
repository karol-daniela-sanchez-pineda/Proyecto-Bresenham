#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in float aAngle; // per-vertex static rotation (radians)

out vec4 vColor;


uniform float uAspect; // width / height

void main()
{
	
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
}