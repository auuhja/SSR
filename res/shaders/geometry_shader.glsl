##GL_VERTEX_SHADER
#version 330

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texCoords;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_tangent;

uniform mat4 MV;
uniform mat4 MVP;

uniform int hasNormalTexture;

out vec3 position;

out vec3 normal;
out vec3 tangent;
out vec3 bitangent;

out vec2 texCoords;


void main()
{
	vec4 pos = vec4(in_position, 1.0);

	position = (MV * pos).xyz;
	texCoords = in_texCoords;

	normal = normalize((MV * vec4(in_normal, 0.0)).xyz);

	if (hasNormalTexture == 1)
	{
		tangent = normalize((MV * vec4(in_tangent, 0.f)).xyz);
		bitangent = cross(tangent, normal);
	}

	gl_Position = MVP * pos;
}



##GL_FRAGMENT_SHADER
#version 330

in vec3 position;

in vec3 normal;
in vec3 tangent;
in vec3 bitangent;

in vec2 texCoords;

// material properties
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

uniform int hasDiffuseTexture;
uniform sampler2D diffuseTexture;

uniform int hasNormalTexture;
uniform sampler2D normalTexture;

struct point_light
{
	vec3 position;
	vec3 color;
	float radius;
};

#define MAX_POINT_LIGHTS 11

uniform point_light pointLights[MAX_POINT_LIGHTS];
uniform int numberOfPointLights;

layout (location = 0) out vec3 out_position;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec3 out_color;
layout (location = 3) out float out_shininess;


void main()
{
	vec3 N = normalize(normal);

	if (hasNormalTexture == 1)
	{
		mat3 TBN = mat3(
			normalize(tangent),
			normalize(bitangent),
			N
		);
		
		N = normalize(TBN * texture2D(normalTexture, texCoords).xyz);
	}

	vec3 ambientColor = vec3(0.0);
	vec3 diffuseColor = vec3(0.0);
	vec3 specularColor = vec3(0.0);
	
	vec3 diffuseTexColor = diffuse;
	if (hasDiffuseTexture == 1)
		diffuseTexColor *= texture2D(diffuseTexture, texCoords).rgb;
		
	vec3 E = normalize(-position);
	for (int i = 0; i < min(MAX_POINT_LIGHTS, numberOfPointLights); ++i)
	{
		vec3 L = pointLights[i].position - position;
		float dist = length(L);
		L /= dist;

		float diffuseFactor = clamp(dot(L, N), 0.0, 1.0);

		float normDist = clamp(dist / pointLights[i].radius, 0.0, 1.0);
		float attenuation = 1.0 - normDist * normDist;

		vec3 R = normalize(reflect(-L, N));
		float specularFactor = pow(clamp(dot(E, R), 0.f, 1.f), shininess);

		ambientColor += ambient * pointLights[i].color * attenuation;
		diffuseColor += diffuseFactor * pointLights[i].color * diffuseTexColor * attenuation;
		specularColor += specularFactor * pointLights[i].color * attenuation;
	}
	
	out_position.xyz = position;
	out_normal.xyz = N;
	out_color = diffuseColor + ambientColor + specularColor;
	out_shininess = shininess;
}
