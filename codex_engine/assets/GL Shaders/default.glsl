#type vertex
#version 330 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec4 a_Colour;

out vec4 f_Colour;

void main()
{
	f_Colour = a_Colour;
	gl_Position = vec4(a_Pos, 1.0f);
}

#type fragment
#version 330 core

in vec4 f_Colour;

out vec4 colour;

void main()
{
	colour = f_Colour;
}