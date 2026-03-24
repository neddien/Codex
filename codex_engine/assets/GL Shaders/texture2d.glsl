#shader_type vertex
#version 330 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec4 a_Colour;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexID;
layout(location = 4) in vec2 a_TexDim;

out vec4 o_Colour;
out vec2 o_TexCoord;
out float o_TexID;
out float o_EntityID;

uniform mat4 u_View;
uniform mat4 u_Proj;

void main()
{
	o_Colour = a_Colour;
	o_TexCoord = vec2(a_TexCoord.x / a_TexDim.x, 1.0 - (a_TexCoord.y / a_TexDim.y));
	o_TexID = a_TexID;
	//o_EntityID = a_EntityID;
	gl_Position = u_Proj * u_View * vec4(a_Pos, 1.0);
}

#shader_type fragment
#version 330 core

in vec4 o_Colour;
in vec2 o_TexCoord;
in float o_TexID;

layout(location = 0) out int o_EntityID;

uniform sampler2D u_Textures[32];

out vec4 FragColour;

void main()
{
	vec4 tex_colour = vec4(1.0, 1.0, 1.0, 1.0);
	int tex_id = int(o_TexID - 1);
	if (o_TexID > 0)
	{
		tex_colour = texture(u_Textures[tex_id], o_TexCoord) * o_Colour;
	}
	else
	{
		FragColour = o_Colour;
	}
	
	if (tex_colour.a < 0.5) 
	{
		discard;
	}
	
	//o_EntityID = 50;
	//FragColour = vec4(o_EntityID, o_EntityID, o_EntityID, 1.0);
}
