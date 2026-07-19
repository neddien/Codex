#shader_type vertex

layout (location = 0) in vec4 a_Vertex;
layout (location = 1) in vec4 a_Colour;
layout (location = 2) in vec3 a_Centre;
layout (location = 3) in vec2 a_TexCoord;
layout (location = 4) in int a_TexId;
layout (location = 5) in int a_EntityId;

out vec4 o_Colour;
flat out int o_EntityId;

uniform mat4 u_View;
uniform mat4 u_Proj;
uniform float u_outline_size;
uniform vec4 u_outline_colour;

void main()
{

	o_Colour = u_outline_colour;
	o_EntityId = a_EntityId;
	gl_Position = u_Proj * u_View * vec4(a_Centre + (a_Vertex.xyz - a_Centre) * (1.0 + u_outline_size), 1.0);
}

#shader_type fragment

in vec4 o_Colour;
flat in int o_EntityId;

layout (location = 0) out vec4 FragColour;
layout (location = 1) out int EntityId;

void main()
{
	EntityId = o_EntityId;
	FragColour = o_Colour;
}
