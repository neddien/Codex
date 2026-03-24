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
out vec2 o_TexCoord;
flat out int o_TexId;
flat out int o_EntityId;

uniform mat4 u_View;
uniform mat4 u_Proj;

void main()
{
	mat4 model = mat4(
		a_MatCol0,
		a_MatCol1,
		a_MatCol2,
		a_MatCol3
);
	o_Colour = a_Colour;
	//o_TexCoord = vec2(a_TexCoord.x / a_TexDim.x, 1.0 - (a_TexCoord.y / a_TexDim.y));
	o_TexCoord = vec2(a_TexCoord.x / a_TexDim.x, a_TexCoord.y / a_TexDim.y);
    o_TexId = a_TexId;
	o_EntityId = a_EntityId;
	gl_Position = u_Proj * u_View * model * a_Vertex;
}

#shader_type fragment

in vec4 o_Colour;
in vec2 o_TexCoord;
flat in int o_TexId;
flat in int o_EntityId;

layout (location = 0) out vec4 FragColour;
layout (location = 1) out int EntityId;

uniform sampler2D u_Textures[CX_MAX_SLOT_COUNT];

void main()
{
	EntityId = o_EntityId;
	int tex_id = o_TexId - 1;
	if (o_TexId > 0)
	{
		switch (tex_id) {
			case 0:
				FragColour = texture(u_Textures[0], o_TexCoord) * o_Colour;
				break;
			case 1:
				FragColour = texture(u_Textures[1], o_TexCoord) * o_Colour;
				break;
			case 2:
				FragColour = texture(u_Textures[2], o_TexCoord) * o_Colour;
				break;
			case 3:
				FragColour = texture(u_Textures[3], o_TexCoord) * o_Colour;
				break;
			case 4:
				FragColour = texture(u_Textures[4], o_TexCoord) * o_Colour;
				break;
			case 5:
				FragColour = texture(u_Textures[5], o_TexCoord) * o_Colour;
				break;
			case 6:
				FragColour = texture(u_Textures[6], o_TexCoord) * o_Colour;
				break;
			case 7:
				FragColour = texture(u_Textures[7], o_TexCoord) * o_Colour;
				break;
#if CX_MAX_SLOT_COUNT >= 16
			case 8:
				FragColour = texture(u_Textures[8], o_TexCoord) * o_Colour;
				break;
			case 9:
				FragColour = texture(u_Textures[9], o_TexCoord) * o_Colour;
				break;
			case 10:
				FragColour = texture(u_Textures[10], o_TexCoord) * o_Colour;
				break;
			case 11:
				FragColour = texture(u_Textures[11], o_TexCoord) * o_Colour;
				break;
			case 12:
				FragColour = texture(u_Textures[12], o_TexCoord) * o_Colour;
				break;
			case 13:
				FragColour = texture(u_Textures[13], o_TexCoord) * o_Colour;
				break;
			case 14:
				FragColour = texture(u_Textures[14], o_TexCoord) * o_Colour;
				break;
			case 15:
				FragColour = texture(u_Textures[15], o_TexCoord) * o_Colour;
				break;
#endif
#if CX_MAX_SLOT_COUNT >= 32
			case 16:
				FragColour = texture(u_Textures[16], o_TexCoord) * o_Colour;
				break;
			case 17:
				FragColour = texture(u_Textures[17], o_TexCoord) * o_Colour;
				break;
			case 18:
				FragColour = texture(u_Textures[18], o_TexCoord) * o_Colour;
				break;
			case 19:
				FragColour = texture(u_Textures[19], o_TexCoord) * o_Colour;
				break;
			case 20:
				FragColour = texture(u_Textures[20], o_TexCoord) * o_Colour;
				break;
			case 21:
				FragColour = texture(u_Textures[21], o_TexCoord) * o_Colour;
				break;
			case 22:
				FragColour = texture(u_Textures[22], o_TexCoord) * o_Colour;
				break;
			case 23:
				FragColour = texture(u_Textures[23], o_TexCoord) * o_Colour;
				break;
			case 24:
				FragColour = texture(u_Textures[24], o_TexCoord) * o_Colour;
				break;
			case 25:
				FragColour = texture(u_Textures[25], o_TexCoord) * o_Colour;
				break;
			case 26:
				FragColour = texture(u_Textures[26], o_TexCoord) * o_Colour;
				break;
			case 27:
				FragColour = texture(u_Textures[27], o_TexCoord) * o_Colour;
				break;
			case 28:
				FragColour = texture(u_Textures[28], o_TexCoord) * o_Colour;
				break;
			case 29:
				FragColour = texture(u_Textures[29], o_TexCoord) * o_Colour;
				break;
			case 30:
				FragColour = texture(u_Textures[30], o_TexCoord) * o_Colour;
				break;
			case 31:
				FragColour = texture(u_Textures[31], o_TexCoord) * o_Colour;
				break;
#endif
		}
	}
	else
		FragColour = o_Colour;
}
