#pragma once

#include "math.h"
#include "common.h"
#include "renderer.h"

#include <vector>

enum scene_name
{
	SCENE_HALLWAY,
	SCENE_STREET,

	SCENE_COUNT,
};

struct entity
{
	uint32 meshStartIndex;
	uint32 meshEndIndex;
	SQT position;

	entity(uint32 startIndex, uint32 endIndex, const SQT& position)
		: meshStartIndex(startIndex), meshEndIndex(endIndex), position(position) {}
};

struct scene_state
{
	camera cam;

	std::vector<opengl_mesh> staticGeometry;
	std::vector<material> staticGeometryMaterials;

	std::vector<opengl_mesh> geometry;
	std::vector<material> materials;
	std::vector<entity> entities;

	std::vector<point_light> pointLights;
};


void initializeScene(scene_state& scene, scene_name name, uint32 screenWidth, uint32 screenHeight);
void updateScene(scene_state& scene, raw_input& input, float dt);
void cleanupScene(scene_state& scene);