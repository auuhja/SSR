##GL_VERTEX_SHADER
#version 330

layout (location = 0) in vec3 in_position;
layout (location = 2) in vec3 in_normal;

uniform mat4 MV;
uniform mat4 MVP;

out vec3 position;
out vec3 normal;

void main()
{
	vec4 pos = vec4(in_position, 1.0);
	position = (MV * pos).xyz;
	normal = (MV * vec4(in_normal, 0.0)).xyz;
	gl_Position = MVP * pos;
}



##GL_FRAGMENT_SHADER
#version 330

in vec3 position;
in vec3 normal;

layout (location = 0) out vec3 out_position;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec4 out_color;


void main()
{
	out_position.xyz = position;
	vec3 N = normalize(normal);
	out_normal.xyz = N;
	out_color = vec4(1.0, 0, 0, 1);
}
