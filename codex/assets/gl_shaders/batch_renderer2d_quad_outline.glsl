#shader_type vertex

layout (location = 0) in vec4 a_MatCol0;
layout (location = 1) in vec4 a_MatCol1;
layout (location = 2) in vec4 a_MatCol2;
layout (location = 3) in vec4 a_MatCol3;
layout (location = 4) in vec4 a_Vertex;
layout (location = 5) in vec4 a_Colour;
layout (location = 6) in vec2 a_TexCoord;
layout (location = 7) in vec2 a_TexDim;
layout (location = 8) in int a_TexId;
layout (location = 9) in int a_EntityId;

out vec4 o_Colour;
flat out int o_EntityId;

uniform mat4 u_View;
uniform mat4 u_Proj;
uniform float u_outline_size;
uniform vec4 u_outline_colour;

void main()
{
	mat4 model = mat4(
		a_MatCol0,
		a_MatCol1,
		a_MatCol2,
		a_MatCol3
    );

	o_Colour = u_outline_colour;
	o_EntityId = a_EntityId;
	gl_Position = u_Proj * u_View * model * (a_Vertex * vec4(1.0 + u_outline_size, 1.0 + u_outline_size, 1.0, 1.0));
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
