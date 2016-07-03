##GL_VERTEX_SHADER
#version 330

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texCoords;

out vec2 texCoords;

void main()
{
	texCoords = in_texCoords;
	gl_Position = vec4(in_position, 1.0);
}

##GL_FRAGMENT_SHADER
#version 330

in vec2 texCoords;

uniform sampler2D colorTexture;
uniform sampler2D reflectionTexture;

layout (location = 0) out vec4 out_color;

void main()
{
	vec4 color = texture2D(colorTexture, texCoords);
	vec4 reflectedColor = texture2D(reflectionTexture, texCoords);

	out_color = mix(color, reflectedColor, reflectedColor.a);
}