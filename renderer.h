#pragma once

#include "common.h"
#include "math.h"
#include <vector>

#include <glew/glew.h>
#include <gl/GL.h>

struct opengl_shader
{
	GLuint vs_ID;
	GLuint gs_ID;
	GLuint fs_ID;
	GLuint programID;

	uint64 writeTime;
};

struct opengl_texture
{
	GLuint textureID;
};

struct opengl_mesh
{
	GLuint vao;
	GLuint vbo;
	GLuint ibo;
	uint32 indexCount;
};

struct opengl_fbo
{
	GLuint fbo;

	uint32 width, height;
	bool usesDepth;

	std::vector<GLuint> colorTextures;
	GLuint depthTexture;
};


struct camera
{
	vec3 position;
	float pitch;
	float yaw;

	uint32 width, height;
	float verticalFOV;
	float nearPlane;
	float farPlane;
	mat4 view;
	mat4 proj;
	mat4 toPrevFramePos;
};

struct point_light
{
	vec3 position;
	float radius;
	vec3 color;

	point_light(const vec3& pos, float radius, const vec3& color)
		: position(pos), radius(radius), color(color) {}
};

struct material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;

	bool hasDiffuseTexture;
	bool hasNormalTexture;
	bool hasSpecularTexture;
	bool emitting;

	opengl_texture diffuseTexture;
	opengl_texture normalTexture;
	opengl_texture specularTexture;
};

#define MAX_POINT_LIGHTS 11


enum shader_type
{
	SHADER_GEOMETRY,
	SHADER_SSR,
	SHADER_BLUR,
	SHADER_RESULT,

	SHADER_COUNT,
};

struct opengl_renderer
{
	uint32 width, height;

	opengl_fbo frontFaceBuffer;		// positions, normals, colors, depth
	opengl_fbo backFaceBuffer;		// backface depth
	opengl_fbo lastFrameBuffer;		// color info of prev frame
	opengl_fbo reflectionBuffer;	// reflection color, reflection mask, this will get slightly blurred
	opengl_fbo tmpBuffer;			// used for blurring

	opengl_mesh plane;
	opengl_mesh sphere;

	union
	{
		struct
		{
			opengl_shader geometryShader;
			opengl_shader ssrShader;
			opengl_shader blurShader;
			opengl_shader resultShader;
		};

		opengl_shader shaders[SHADER_COUNT];
	};

	// shader uniforms
	GLuint geometry_MVP, geometry_MV, geometry_ambient, geometry_diffuse, geometry_specular, geometry_shininess, geometry_emitting;
	GLuint geometry_hasDiffuseTexture, geometry_hasNormalTexture, geometry_hasSpecularTexture;

	GLuint geometry_numberOfPointLights, geometry_pl_position[MAX_POINT_LIGHTS], geometry_pl_color[MAX_POINT_LIGHTS], geometry_pl_radius[MAX_POINT_LIGHTS];

	GLuint ssr_proj, ssr_toPrevFramePos, ssr_clippingPlanes;

	GLuint blur_blurDirection;
};

void initializeRenderer(opengl_renderer& renderer, uint32 screenWidth, uint32 screenHeight);
void renderScene(opengl_renderer& renderer, struct scene_state& scene, uint32 screenWidth, uint32 screenHeight, bool debugRendering = false);
void cleanupRenderer(opengl_renderer& renderer);

bool loadStaticGeometry(std::vector<opengl_mesh>& meshes, std::vector<material>& materials, const std::string& filename);
bool loadMesh(opengl_mesh& mesh, const std::string& filename);
std::pair<uint32, uint32> loadMesh(std::vector<opengl_mesh>& meshes, std::vector<material>& materials, const std::string& filename);

void deleteMesh(opengl_mesh& mesh);
void deleteTexture(opengl_texture& texture);
