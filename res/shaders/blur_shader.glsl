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

uniform sampler2D inputTexture;
uniform vec2 blurDirection; // [1, 0] or [0, 1]

const float gaussWeights[5] = 
{
	0.06136, 0.24477, 0.38774, 0.24477, 0.06136
};

layout (location = 0) out vec4 out_color;

void main()
{
	vec2 texSize = textureSize(inputTexture, 0);
	vec2 texelSize = vec2(1.0 / texSize.x, 1.0 / texSize.y);
	vec2 texelOffset = texelSize * blurDirection;

	vec4 blurred = vec4(0.0);

	for (float i = -2; i <= 2; i += 1.0)
	{
		blurred += texture2D(inputTexture, texCoords + i * texelOffset) * gaussWeights[int(i) + 2];
	}

	out_color = blurred;
}