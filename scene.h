#pragma once

#include "math.h"
#include "common.h"

#include <glew/glew.h>
#include <gl/GL.h>
#include <vector>
#include <unordered_map>

struct camera
{
	vec3 position;
	float pitch;
	float yaw;

	float aspect;
	float verticalFOV;
	float nearPlane;
	float farPlane;
	mat4 view;
	mat4 proj;
	mat4 invProj;
};

struct point_light
{
	vec3 position;
	float radius;
	vec3 color;

	point_light(const vec3& pos, float radius, const vec3& color)
		: position(pos), radius(radius), color(color) {}
};

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

enum shader_type
{
	SHADER_GEOMETRY,
	SHADER_MATERIAL,
	SHADER_SSR,

	SHADER_COUNT,
};

enum model_name
{
	MESH_STALL,
	MESH_PLANE,
	MESH_SPHERE,

	MESH_COUNT,
};

enum texture_name
{
	TEXTURE_STALL,
	TEXTURE_GROUND,

	TEXTURE_COUNT
};

struct entity
{
	model_name meshID;
	texture_name textureID;
	SQT transform;
	float shininess;

	entity(model_name meshID, texture_name textureID, const SQT& sqt, float shininess)
		: meshID(meshID), textureID(textureID), transform(sqt), shininess(shininess) {}
};

struct material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

#define MAX_POINT_LIGHTS 11

struct scene_state
{
	camera cam;

	opengl_fbo frontFaceBuffer;
	opengl_fbo backFaceBuffer;

	opengl_shader shaders[SHADER_COUNT];
	opengl_texture textures[TEXTURE_COUNT];
	opengl_mesh meshes[MESH_COUNT];

	std::vector<opengl_mesh> staticGeometry;
	std::vector<material> staticGeometryMaterials;

	std::vector<entity> entities;
	std::vector<point_light> pointLights;

	// shader uniforms
	GLuint geometry_MVP, geometry_MV, geometry_numberOfPointLights, geometry_shininess;
	GLuint geometry_pl_position[MAX_POINT_LIGHTS], geometry_pl_radius[MAX_POINT_LIGHTS], geometry_pl_color[MAX_POINT_LIGHTS];

	GLuint material_MVP, material_MV, material_numberOfPointLights, material_ambient, material_diffuse, material_specular, material_shininess;
	GLuint material_pl_position[MAX_POINT_LIGHTS], material_pl_radius[MAX_POINT_LIGHTS], material_pl_color[MAX_POINT_LIGHTS];

	GLuint ssr_screenDim, ssr_proj, ssr_invProj, ssr_clippingPlanes;
};

enum kb_button
{
	KB_0, KB_1, KB_2, KB_3, KB_4, KB_5, KB_6, KB_7, KB_8, KB_9,
	KB_A, KB_B, KB_C, KB_D, KB_E, KB_F, KB_G, KB_H, KB_I, KB_J,
	KB_K, KB_L, KB_M, KB_N, KB_O, KB_P, KB_Q, KB_R, KB_S, KB_T,
	KB_U, KB_V, KB_W, KB_X, KB_Y, KB_Z, KB_SPACE, KB_ENTER,
	KB_SHIFT, KB_ALT, KB_TAB, KB_CTRL, KB_ESC, KB_UP, KB_DOWN,
	KB_LEFT, KB_RIGHT,

	KB_BUTTONCOUNT,

	KB_UNKNOWN_BUTTON
};

struct button
{
	bool isDown;
	bool wasDown;
};

struct keyboard_input
{
	button buttons[KB_BUTTONCOUNT];
};

struct mouse_input
{
	button left;
	button right;
	button middle;
	float scroll;

	int x;
	int y;

	float relX;
	float relY;

	int dx;
	int dy;

	float reldx;
	float reldy;
};

struct raw_input
{
	keyboard_input keyboard;
	mouse_input mouse;
};

inline bool isDown(raw_input& input, kb_button buttonID)
{
	return input.keyboard.buttons[buttonID].isDown;
}

inline bool isUp(raw_input& input, kb_button buttonID)
{
	return !input.keyboard.buttons[buttonID].isDown;
}

inline bool buttonDownEvent(button& button)
{
	return button.isDown && !button.wasDown;
}

inline bool buttonUpEvent(button& button)
{
	return !button.isDown && button.wasDown;
}

inline bool buttonDownEvent(raw_input& input, kb_button buttonID)
{
	return buttonDownEvent(input.keyboard.buttons[buttonID]);
}

inline bool buttonUpEvent(raw_input& input, kb_button buttonID)
{
	return buttonUpEvent(input.keyboard.buttons[buttonID]);
}

inline kb_button charToButton(char c)
{
	kb_button result = KB_UNKNOWN_BUTTON;
	if (c >= 'a' && c <= 'z')
		result = (kb_button)(c - 'a' + KB_A);
	else if (c >= 'A' && c <= 'Z')
		result = (kb_button)(c - 'A' + KB_A);
	else if (c >= '0' && c <= '9')
		result = (kb_button)(c - '0');
	else
	{
		switch (c)
		{
			case 32: result = KB_SPACE; break;
			case 27: result = KB_ESC; break;
			case 9: result = KB_TAB; break;
				// TODO: complete this..
		}
	}

	return result;
}

inline bool buttonDownEvent(raw_input& input, char c)
{
	kb_button b = charToButton(c);
	if (b != KB_UNKNOWN_BUTTON)
		return buttonDownEvent(input.keyboard.buttons[b]);
	return false;
}

inline bool buttonUpEvent(raw_input& input, char c)
{
	kb_button b = charToButton(c);
	if (b != KB_UNKNOWN_BUTTON)
		return buttonUpEvent(input.keyboard.buttons[b]);
	return false;
}

void initializeScene(scene_state& scene, uint32 screenWidth, uint32 screenHeight);
void updateScene(scene_state& scene, raw_input& input, float dt);
void renderScene(scene_state& scene, uint32 screenWidth, uint32 screenHeight);
void cleanupScene(scene_state& scene);