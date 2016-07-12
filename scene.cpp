#include "scene.h"

#include <string>



void initializeScene(scene_state& scene, scene_name name, uint32 screenWidth, uint32 screenHeight)
{
	// meshes
	if (name == SCENE_HALLWAY)
	{
		loadStaticGeometry(scene.staticGeometry, scene.staticGeometryMaterials, "hallway/space_station_interior.obj");
	
		float lightHeight = 7.5f;
		float radius = 15.f;
		float lightDistance = 9.1f;

		for (uint32 i = 0; i < 10; ++i)
		{
			scene.pointLights.push_back(point_light(vec3(-41.f + i * lightDistance, lightHeight, 0.f), radius, vec3(1.f, 1.f, 1.f)));
		}

		scene.pointLights.push_back(point_light(vec3(18.2f, lightHeight-2.f, -13.8f), radius, vec3(1.f, 1.f, 1.f)));
	}
	else if (name == SCENE_STREET)
	{
		loadStaticGeometry(scene.staticGeometry, scene.staticGeometryMaterials, "street/street.obj");

		std::pair<uint32, uint32> lampIndices = loadMesh(scene.geometry, scene.materials, "street/lamp.obj");
		float lightHeight = 5.f;
		float radius = 30.f;
		vec3 color(0.7f, 0.53f, 0.36f);

		scene.entities.push_back(entity(lampIndices.first, lampIndices.second, SQT(vec3(8.f, 0.f, 1.f), quat(), 0.3f)));
		scene.pointLights.push_back(point_light(vec3(-8.f, lightHeight, 1.f), radius, color));

		scene.entities.push_back(entity(lampIndices.first, lampIndices.second, SQT(vec3(0.f, 0.f, 5.f), quat(), 0.3f)));
		scene.pointLights.push_back(point_light(vec3(0.f, lightHeight, 5.f), radius, color));

		scene.entities.push_back(entity(lampIndices.first, lampIndices.second, SQT(vec3(-10.f, 0.f, 5.f), quat(), 0.3f)));
		scene.pointLights.push_back(point_light(vec3(-10.f, lightHeight, 5.f), radius, color));

		scene.entities.push_back(entity(lampIndices.first, lampIndices.second, SQT(vec3(-20.f, 0.f, 5.f), quat(), 0.3f)));
		scene.pointLights.push_back(point_light(vec3(-20.f, lightHeight, 5.f), radius, color));

		scene.entities.push_back(entity(lampIndices.first, lampIndices.second, SQT(vec3(-30.f, 0.f, 2.f), quat(), 0.3f)));
		scene.pointLights.push_back(point_light(vec3(-30.f, lightHeight, 2.f), radius, color));

		scene.entities.push_back(entity(lampIndices.first, lampIndices.second, SQT(vec3(-40.f, 0.f, -1.f), quat(), 0.3f)));
		scene.pointLights.push_back(point_light(vec3(-40.f, lightHeight, -1.f), radius, color));


		std::pair<uint32, uint32> wallLampIndices = loadMesh(scene.geometry, scene.materials, "street/wall_lamp.obj");
		scene.entities.push_back(entity(wallLampIndices.first, wallLampIndices.second, SQT(vec3(-28.5f, 6.f, 17.6f), quat(vec3(0.f, 1.f, 0.f), degreesToRadians(180.f)), 5.f)));

	}

	// camera
	{
		scene.cam.nearPlane = 0.1f;
		scene.cam.farPlane = 1000.f;
		scene.cam.position = vec3(0.f, 2.f, 0.f);
		scene.cam.pitch = 0.f;
		scene.cam.yaw = 0.f;
		scene.cam.verticalFOV = degreesToRadians(70.f);
		scene.cam.width = screenWidth;
		scene.cam.height = screenHeight;
		float aspect = (float)screenWidth / (float)screenHeight;
		scene.cam.proj = createProjectionMatrix(scene.cam.verticalFOV, aspect, scene.cam.nearPlane, scene.cam.farPlane);
		scene.cam.view = createViewMatrix(scene.cam.position, scene.cam.pitch, scene.cam.yaw);
		scene.cam.toPrevFramePos = scene.cam.proj;
	}
}

void updateScene(scene_state& scene, raw_input& input, float dt)
{
	mat4 prevView = scene.cam.view;

	if (buttonDownEvent(input, KB_ESC))
		exit(0);

	const float movementSpeed = 10.f;
	const float rotationSpeed = 2.f;

	vec3 positionChange(0.f);
	if (isDown(input, KB_W)) positionChange += vec3(0.f, 0.f, -1.f);
	if (isDown(input, KB_S)) positionChange += vec3(0.f, 0.f, 1.f);
	if (isDown(input, KB_D)) positionChange += vec3(1.f, 0.f, 0.f);
	if (isDown(input, KB_A)) positionChange += vec3(-1.f, 0.f, 0.f);

	if (input.mouse.left.isDown)
	{
		scene.cam.pitch = fmodf(scene.cam.pitch + input.mouse.reldy * rotationSpeed, 2.f * M_PI);
		scene.cam.yaw = fmodf(scene.cam.yaw - input.mouse.reldx * rotationSpeed, 2.f * M_PI);
	}

	quat rotation = quat(vec3(0.f, 1.f, 0.f), scene.cam.yaw) * quat(vec3(1.f, 0.f, 0.f), scene.cam.pitch);
	scene.cam.position += (rotation * positionChange) * movementSpeed * dt;

	scene.cam.view = createViewMatrix(scene.cam.position, scene.cam.pitch, scene.cam.yaw);
	scene.cam.toPrevFramePos = scene.cam.proj * prevView * inverted(scene.cam.view); // I think this only works with non-moving geometry!

	//std::cout << scene.cam.position << std::endl;
}

void cleanupScene(scene_state& scene)
{
	for (opengl_mesh& mesh : scene.staticGeometry)
		deleteMesh(mesh);
	for (material& mat : scene.staticGeometryMaterials)
	{
		deleteTexture(mat.diffuseTexture);
		deleteTexture(mat.normalTexture);
	}

	for (opengl_mesh& mesh : scene.geometry)
		deleteMesh(mesh);
	for (material& mat : scene.materials)
	{
		deleteTexture(mat.diffuseTexture);
		deleteTexture(mat.normalTexture);
	}
}