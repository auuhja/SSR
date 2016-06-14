#include "scene.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <string>

static bool loadTexture(opengl_texture& texture, const std::string& filename);


opengl_shader scene_state::shaders[SHADER_COUNT];

#pragma pack(push, 1)
struct vertex3PTN
{
	vec3 pos;
	vec2 tex;
	vec3 nor;
};
#pragma pack(pop)

static void uploadVertexData(opengl_mesh& mesh, const std::vector<vertex3PTN>& vertices, const std::vector<uint32>& indices)
{
	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex3PTN), &vertices[0], GL_STATIC_DRAW);

	// positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex3PTN), (void*)0);
	// texCoords
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex3PTN), (void*)(3 * sizeof(GLfloat)));
	// normals
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex3PTN), (void*)(5 * sizeof(GLfloat)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &mesh.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32), &indices[0], GL_STATIC_DRAW);

	glBindVertexArray(0);
}

// this is expected to be already at the desired world position
static bool loadStaticGeometry(std::vector<opengl_mesh>& meshes, std::vector<material>& materials, const std::string& filename)
{
	std::string filepath = std::string("res/models/") + filename;

	Assimp::Importer Importer;
	const aiScene* aiScene = Importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_GenSmoothNormals
		| aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality);

	if (!aiScene) {
		std::cerr << "File " << filepath << " not found." << std::endl;
		return false;
	}

	uint32 numberOfMeshes = aiScene->mNumMeshes;
	for (uint32 m = 0; m < numberOfMeshes; ++m)
	{
		const aiMesh* aiMesh = aiScene->mMeshes[m];
		opengl_mesh mesh;

		std::vector<vertex3PTN> vertices;
		std::vector<uint32> indices;

		vertices.reserve(aiMesh->mNumVertices);
		indices.reserve(aiMesh->mNumFaces * 3);

		uint32 vertexCount = aiMesh->mNumVertices;
		uint32 indexCount = aiMesh->mNumFaces * 3;

		mesh.indexCount = indexCount;

		for (uint32 i = 0; i < aiMesh->mNumVertices; ++i) {

			const aiVector3D& pos = aiMesh->mVertices[i];
			const aiVector3D& nor = aiMesh->mNormals[i];

			vertex3PTN vertex;
			vertex.pos = vec3(pos.x, pos.y, pos.z);
			vertex.tex = vec2(0, 0);
			if (aiMesh->HasTextureCoords(0))
				vertex.tex = vec2(aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y);

			vertex.nor = vec3(nor.x, nor.y, nor.z);
			vertices.push_back(vertex);
		}

		for (uint32 i = 0; i < aiMesh->mNumFaces; i++) {
			const aiFace &face = aiMesh->mFaces[i];
			assert(face.mNumIndices == 3);
			indices.push_back(face.mIndices[0]);
			indices.push_back(face.mIndices[1]);
			indices.push_back(face.mIndices[2]);
		}

		uploadVertexData(mesh, vertices, indices);

		material material = { 0 };
		aiMaterial* mat = aiScene->mMaterials[aiMesh->mMaterialIndex];
		aiString name;
		mat->Get(AI_MATKEY_NAME, name);

		aiColor3D color;
		mat->Get(AI_MATKEY_COLOR_AMBIENT, color);
		material.ambient = vec3(color.r, color.g, color.b);

		mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		material.diffuse = vec3(color.r, color.g, color.b);

		mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
		material.specular = vec3(color.r, color.g, color.b);

		float shininess;
		mat->Get(AI_MATKEY_SHININESS, shininess);
		material.shininess = shininess;

		aiString texPath;
		if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == aiReturn_SUCCESS)
		{
			std::cout << name.C_Str() << " has diffuse: " << texPath.C_Str() << std::endl;
			material.hasDiffuseTexture = true;
			loadTexture(material.diffuseTexture, texPath.C_Str());
		}
		if (mat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == aiReturn_SUCCESS)
		{
			std::cout << name.C_Str() << " has normal: " << texPath.C_Str() << std::endl;
			material.hasNormalTexture = true;
			loadTexture(material.normalTexture, texPath.C_Str());
		}


		meshes.push_back(mesh);
		materials.push_back(material);
	}

	return true;
}

static bool loadMesh(opengl_mesh& mesh, const std::string& filename)
{
	std::string filepath = std::string("res/models/") + filename;

	Assimp::Importer Importer;
	const aiScene* aiScene = Importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_GenSmoothNormals
		| aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality);

	if (!aiScene) {
		std::cerr << "File " << filepath << " not found." << std::endl;
		return false;
	}

	// for now
	assert(aiScene->mNumMeshes == 1);

	const aiMesh* aiMesh = aiScene->mMeshes[0];

	std::vector<vertex3PTN> vertices;
	std::vector<uint32> indices;

	vertices.reserve(aiMesh->mNumVertices);
	indices.reserve(aiMesh->mNumFaces * 3);

	uint32 vertexCount = aiMesh->mNumVertices;
	uint32 indexCount = aiMesh->mNumFaces * 3;

	mesh.indexCount = indexCount;

	assert(aiMesh->HasPositions());
	assert(aiMesh->HasTextureCoords(0));
	assert(aiMesh->HasNormals());

	for (uint32 i = 0; i < aiMesh->mNumVertices; ++i) {

		const aiVector3D& pos = aiMesh->mVertices[i];
		const aiVector3D& nor = aiMesh->mNormals[i];
		const aiVector3D& tex = aiMesh->mTextureCoords[0][i];

		vertex3PTN vertex;
		vertex.pos = vec3(pos.x, pos.y, pos.z);
		vertex.tex = vec2(tex.x, tex.y);
		vertex.nor = vec3(nor.x, nor.y, nor.z);
		vertices.push_back(vertex);
	}

	for (uint32 i = 0; i < aiMesh->mNumFaces; i++) {
		const aiFace &face = aiMesh->mFaces[i];
		assert(face.mNumIndices == 3);
		indices.push_back(face.mIndices[0]);
		indices.push_back(face.mIndices[1]);
		indices.push_back(face.mIndices[2]);
	}

	uploadVertexData(mesh, vertices, indices);

	return true;
}

static void deleteMesh(opengl_mesh& mesh)
{
	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(1, &mesh.vbo);
	glDeleteBuffers(1, &mesh.ibo);
}

static inline void bindAndDrawMesh(opengl_mesh& mesh)
{
	glBindVertexArray(mesh.vao);
	glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

static bool loadTexture(opengl_texture& texture, const std::string& filename)
{
	std::string filepath = "res/textures/" + filename;

	int32 width, height, comp;
	unsigned char *data = stbi_load(filepath.c_str(), &width, &height, &comp, 4);
	if (!data)
	{
		std::cerr << "File " << filepath << " not found." << std::endl;
		return false;
	}

	glGenTextures(1, &texture.textureID);
	glBindTexture(GL_TEXTURE_2D, texture.textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0);

	// anisotropic filtering
	float supported = 0.f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &supported);
	if (supported != 0.f)
	{
		float amount = min(4.f, supported);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, amount);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(data);

	return true;
}

static void deleteTexture(opengl_texture& texture)
{
	glDeleteTextures(1, &texture.textureID);
}

static GLuint loadShaderComponent(const std::string& filename, GLenum glType)
{
	GLuint shaderID = glCreateShader(glType);
	if (!shaderID)
	{
		std::cerr << "shader component creation failed" << std::endl;
	}

	std::string path = getPath(filename);

	input_file openFiles[5];
	openFiles[0] = readFile(filename.c_str());
	uint32 numberOfOpenFiles = 1;
	uint32 returnTo[5] = { 0 };

	if (!openFiles[0].contents)
	{
		std::cerr << "shader file reading failed" << std::endl;
	}

	char* shaderPrefix;
	switch (glType)
	{
		case GL_VERTEX_SHADER: { shaderPrefix = "##GL_VERTEX_SHADER\n"; } break;
		case GL_GEOMETRY_SHADER: { shaderPrefix = "##GL_GEOMETRY_SHADER\n"; } break;
		case GL_FRAGMENT_SHADER: { shaderPrefix = "##GL_FRAGMENT_SHADER\n"; } break;
	}
	uint64 prefixLength = strlen(shaderPrefix);



	uint64 indexStack[5] = { 0 };

	char* shaderSources[10] = { 0 };
	shaderSources[0] = (char*)openFiles[0].contents;
	GLint shaderLengths[10] = { 0 };

	for (indexStack[0] = 0; indexStack[0] < openFiles[0].size; ++indexStack[0])
	{
		if (strncmp(shaderSources[0] + indexStack[0], shaderPrefix, prefixLength - 1) == 0)
		{
			shaderSources[0] = shaderSources[0] + indexStack[0] + prefixLength;
			break;
		}
	}

	indexStack[0] += prefixLength;

	const char* include = "#include \"";
	size_t includeLength = strlen(include);

	uint32 filePointer = 0;
	uint64 currentIndex = 0;
	uint32 currentWrite = 0;
	while (true)
	{
		if (filePointer == 0 && (indexStack[0] + currentIndex >= openFiles[0].size || strncmp(shaderSources[currentWrite] + currentIndex, "##", 2) == 0))
		{
			shaderLengths[currentWrite] = (GLint)currentIndex;
			break;
		}
		else if (strncmp(shaderSources[currentWrite] + currentIndex, include, includeLength) == 0)
		{
			char includeFileName[50];
			uint32 fileNameLength = 0;
			for (; shaderSources[currentWrite][currentIndex + includeLength + fileNameLength] != '\"'; ++fileNameLength)
			{
				includeFileName[fileNameLength] = shaderSources[currentWrite][currentIndex + includeLength + fileNameLength];
			}
			includeFileName[fileNameLength] = '\0';
			std::string includeFilePath = path + "/" + includeFileName;
			std::cout << openFiles[filePointer].filename << " includes " << includeFilePath << std::endl;

			openFiles[numberOfOpenFiles++] = readFile(includeFilePath.c_str());

			shaderLengths[currentWrite] = (GLint)currentIndex;
			indexStack[filePointer] = indexStack[filePointer] + currentIndex + includeLength + fileNameLength + 1;

			uint32 myFilePointer = filePointer;
			filePointer = numberOfOpenFiles - 1;

			++currentWrite;
			returnTo[currentWrite] = myFilePointer;

			shaderSources[currentWrite] = (char*)openFiles[filePointer].contents;

			currentIndex = 0;
		}
		else if (indexStack[filePointer] + currentIndex >= openFiles[filePointer].size)
		{
			shaderLengths[currentWrite] = (GLint)currentIndex;

			filePointer = returnTo[filePointer];
			++currentWrite;

			shaderSources[currentWrite] = (char*)openFiles[filePointer].contents + indexStack[filePointer];
			currentIndex = 0;
		}
		else
		{
			++currentIndex;
		}
	}

	uint32 numberOfStrings = currentWrite + 1;
	

	glShaderSource(shaderID, numberOfStrings, shaderSources, shaderLengths);
	glCompileShader(shaderID);

	GLint success;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[1024];
		glGetShaderInfoLog(shaderID, 1024, NULL, infoLog);

		std::cerr << "Error compiling shader " << openFiles[0].filename << ":\n" << infoLog << std::endl;
	}

	for (uint32 i = 0; i < currentWrite; ++i)
		freeFile(openFiles[i]);

	return shaderID;
}

static bool loadShader(opengl_shader& shader, const std::string& filename)
{
	std::string path = "res/shaders/";
	std::string filepath = path + filename;

	uint64 writeTime = getFileWriteTime(filepath.c_str());
	if (writeTime <= shader.writeTime)
	{
		//std::cout << "already loaded" << std::endl;

		shader.writeTime = writeTime;
		return false;
	}

	shader.writeTime = writeTime;

	std::cout << "reloading" << std::endl;

	shader.programID = glCreateProgram();
	if (!shader.programID)
	{
		std::cerr << "program creation failed" << std::endl;
		return false;
	}

	shader.vs_ID = loadShaderComponent(filepath, GL_VERTEX_SHADER);
	shader.fs_ID = loadShaderComponent(filepath, GL_FRAGMENT_SHADER);
	glAttachShader(shader.programID, shader.vs_ID);
	glAttachShader(shader.programID, shader.fs_ID);
	glLinkProgram(shader.programID);
	GLint success;
	glGetProgramiv(shader.programID, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[1024];
		glGetProgramInfoLog(shader.programID, 1024, NULL, infoLog);
		std::cerr << "error linking shader" << filename <<  ":\n" << infoLog << std::endl;
		return false;
	}

	glValidateProgram(shader.programID);
	glGetProgramiv(shader.programID, GL_VALIDATE_STATUS, &success);
	if (!success)
	{
		std::cerr << "error validating shader" << std::endl;
		return false;
	}

	return true;
}

static inline void bindShader(opengl_shader& shader)
{
	glUseProgram(shader.programID);
}

static inline void unbindShader()
{
	glUseProgram(0);
}

static void deleteShader(opengl_shader& shader)
{
	glDetachShader(shader.programID, shader.vs_ID);
	glDetachShader(shader.programID, shader.gs_ID);
	glDetachShader(shader.programID, shader.fs_ID);
	glDeleteShader(shader.vs_ID);
	glDeleteShader(shader.gs_ID);
	glDeleteShader(shader.fs_ID);
	glDeleteProgram(shader.programID);
}

static void createFBO(opengl_fbo& fbo, uint32 width, uint32 height)
{
	glGenFramebuffers(1, &fbo.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);

	fbo.width = width;
	fbo.height = height;
	fbo.usesDepth = false;
}

static void attachColorAttachment(opengl_fbo& fbo, GLint internalformat, GLint format)
{
	GLuint texture;
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, fbo.width, fbo.height, 0, GL_RGB, format, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (int32)fbo.colorTextures.size(), GL_TEXTURE_2D, texture, 0);

	fbo.colorTextures.push_back(texture);
}

static void attachDepthAttachment(opengl_fbo& fbo)
{
	if (!fbo.usesDepth)
	{
		glGenTextures(1, &fbo.depthTexture);

		glBindTexture(GL_TEXTURE_2D, fbo.depthTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, fbo.width, fbo.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glBindTexture(GL_TEXTURE_2D, 0);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo.depthTexture, 0);

		fbo.usesDepth = true;
	}
}

static bool finishFBO(opengl_fbo& fbo)
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "FB error, status: " << status << std::endl;
		return false;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

static void deleteFBO(opengl_fbo& fbo)
{
	glDeleteFramebuffers(1, &fbo.fbo);
	if (fbo.colorTextures.size() > 0)
		glDeleteTextures((GLsizei)fbo.colorTextures.size(), &fbo.colorTextures[0]);
	glDeleteTextures(1, &fbo.depthTexture);

	fbo.colorTextures.clear();
	fbo.usesDepth = false;
}

static inline void bindFramebuffer(opengl_fbo& fbo)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
	glViewport(0, 0, fbo.width, fbo.height);
	if (fbo.colorTextures.size() > 0)
	{
		std::vector<GLenum> drawBuffers(fbo.colorTextures.size());
		for (uint32 i = 0; i < fbo.colorTextures.size(); ++i)
			drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
		glDrawBuffers((GLsizei)fbo.colorTextures.size(), &drawBuffers[0]);
	}
	else
	{
		glDrawBuffer(GL_NONE);
	}
}

static inline void bindDefaultFramebuffer(uint32 width, uint32 height)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
}

static bool initializeFBOs(opengl_fbo& frontFaceBuffer, opengl_fbo& backFaceBuffer, uint32 width, uint32 height)
{
	createFBO(frontFaceBuffer, width, height);
	attachColorAttachment(frontFaceBuffer, GL_RGB32F, GL_FLOAT);		// positions
	attachColorAttachment(frontFaceBuffer, GL_RGB16F, GL_FLOAT);		// normals
	attachColorAttachment(frontFaceBuffer, GL_RGBA8, GL_UNSIGNED_BYTE); // color
	attachDepthAttachment(frontFaceBuffer);
	bool frontFaceBufferSucess = finishFBO(frontFaceBuffer);

	createFBO(backFaceBuffer, width / 4, height / 4);
	attachDepthAttachment(backFaceBuffer);
	bool backFaceBufferSuccess = finishFBO(backFaceBuffer);

	bindDefaultFramebuffer(width, height);

	return frontFaceBufferSucess && backFaceBufferSuccess;
}

static bool loadAllShaders(scene_state& scene)
{
	bool reloaded = false;
	{
		opengl_shader& shader = scene.shaders[SHADER_GEOMETRY];
		if (loadShader(shader, "geometry_shader.glsl"))
		{
			bindShader(shader);
			scene.geometry_MVP = glGetUniformLocation(shader.programID, "MVP");
			scene.geometry_MV = glGetUniformLocation(shader.programID, "MV");
			scene.geometry_shininess = glGetUniformLocation(shader.programID, "shininess");
			scene.geometry_numberOfPointLights = glGetUniformLocation(shader.programID, "numberOfPointLights");

			for (uint32 i = 0; i < MAX_POINT_LIGHTS; ++i)
			{
				std::string uniformName = std::string("pointLights[") + std::to_string(i) + "].";
				scene.geometry_pl_position[i] = glGetUniformLocation(shader.programID, (uniformName + "position").c_str());
				scene.geometry_pl_radius[i] = glGetUniformLocation(shader.programID, (uniformName + "radius").c_str());
				scene.geometry_pl_color[i] = glGetUniformLocation(shader.programID, (uniformName + "color").c_str());
			}

			glUniform1i(glGetUniformLocation(shader.programID, "texture"), 0);

			reloaded = true;
		}
	}
	{
		opengl_shader& shader = scene.shaders[SHADER_MATERIAL];
		if (loadShader(shader, "material_shader.glsl"))
		{
			bindShader(shader);
			scene.material_MVP = glGetUniformLocation(shader.programID, "MVP");
			scene.material_MV = glGetUniformLocation(shader.programID, "MV");
			scene.material_numberOfPointLights = glGetUniformLocation(shader.programID, "numberOfPointLights");
			scene.material_ambient = glGetUniformLocation(shader.programID, "ambient");
			scene.material_diffuse = glGetUniformLocation(shader.programID, "diffuse");
			scene.material_specular = glGetUniformLocation(shader.programID, "specular");
			scene.material_shininess = glGetUniformLocation(shader.programID, "shininess");
			scene.material_hasDiffuseTexture = glGetUniformLocation(shader.programID, "hasDiffuseTexture");

			for (uint32 i = 0; i < MAX_POINT_LIGHTS; ++i)
			{
				std::string uniformName = std::string("pointLights[") + std::to_string(i) + "].";
				scene.material_pl_position[i] = glGetUniformLocation(shader.programID, (uniformName + "position").c_str());
				scene.material_pl_radius[i] = glGetUniformLocation(shader.programID, (uniformName + "radius").c_str());
				scene.material_pl_color[i] = glGetUniformLocation(shader.programID, (uniformName + "color").c_str());
			}

			glUniform1i(glGetUniformLocation(shader.programID, "diffuseTexture"), 0);

			reloaded = true;
		}
	}
	{
		opengl_shader& shader = scene.shaders[SHADER_SSR];
		if (loadShader(shader, "ssr_shader.glsl"))
		{
			bindShader(shader);
			scene.ssr_screenDim = glGetUniformLocation(shader.programID, "screenDim");
			scene.ssr_proj = glGetUniformLocation(shader.programID, "proj");
			scene.ssr_invProj = glGetUniformLocation(shader.programID, "invProj");
			scene.ssr_clippingPlanes = glGetUniformLocation(shader.programID, "clippingPlanes");
			
			glUniform1i(glGetUniformLocation(shader.programID, "positionTexture"), 0);
			glUniform1i(glGetUniformLocation(shader.programID, "normalTexture"), 1);
			glUniform1i(glGetUniformLocation(shader.programID, "colorTexture"), 2);
			glUniform1i(glGetUniformLocation(shader.programID, "depthTexture"), 3);
			glUniform1i(glGetUniformLocation(shader.programID, "backfaceDepthTexture"), 4);

			reloaded = true;
		}
	}

	return reloaded;
}

void initializeScene(scene_state& scene, scene_name name, uint32 screenWidth, uint32 screenHeight)
{
	initializeFBOs(scene.frontFaceBuffer, scene.backFaceBuffer, screenWidth, screenHeight);

	// shaders
	{
		for (uint32 i = 0; i < SHADER_COUNT; ++i)
			scene.shaders[i].writeTime = 0;
		loadAllShaders(scene);
	}

	loadMesh(scene.plane, "plane.obj");

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

		// TODO: right light positions
		float lightHeight = 7.5f;
		float radius = 15.f;
		float lightDistance = 9.1f;
		for (uint32 i = 0; i < 10; ++i)
		{
			scene.pointLights.push_back(point_light(vec3(-41.f + i * lightDistance, lightHeight, 0.f), radius, vec3(1.f, 1.f, 1.f)));
		}
	}

	// camera
	{
		scene.cam.nearPlane = 0.1f;
		scene.cam.farPlane = 1000.f;
		scene.cam.position = vec3(0.f, 2.f, 0.f);
		scene.cam.pitch = 0.f;
		scene.cam.yaw = 0.f;
		scene.cam.verticalFOV = degreesToRadians(70.f);
		scene.cam.aspect = (float)screenWidth / (float)screenHeight;
		scene.cam.proj = createProjectionMatrix(scene.cam.verticalFOV, scene.cam.aspect, scene.cam.nearPlane, scene.cam.farPlane);
		scene.cam.invProj = inverted(scene.cam.proj);
		scene.cam.view = createViewMatrix(scene.cam.position, scene.cam.pitch, scene.cam.yaw);
	}


	glClearColor(0.18f, 0.35f, 0.5f, 1.0f);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
}

void updateScene(scene_state& scene, raw_input& input, float dt)
{
	//scene.entities[0].transform.rotation = quat(vec3(0, 1, 0), 0.001f) * scene.entities[0].transform.rotation;

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
}

static void renderGeometry(scene_state& scene)
{
	opengl_shader& materialShader = scene.shaders[SHADER_MATERIAL];
	bindShader(materialShader);

	glUniform1i(scene.material_numberOfPointLights, (int32)scene.pointLights.size());
	for (uint32 i = 0; i < min(scene.pointLights.size(), MAX_POINT_LIGHTS); ++i)
	{
		vec4 posVS = scene.cam.view * vec4(scene.pointLights[i].position, 1.f);
		glUniform3f(scene.material_pl_position[i], posVS.x, posVS.y, posVS.z);
		glUniform1f(scene.material_pl_radius[i], scene.pointLights[i].radius);
		glUniform3f(scene.material_pl_color[i], scene.pointLights[i].color.x, scene.pointLights[i].color.y, scene.pointLights[i].color.z);
	}
	for (uint32 i = 0; i < scene.staticGeometry.size(); ++i)
	{
		mat4 MV = scene.cam.view;
		mat4 MVP = scene.cam.proj * MV;

		glUniformMatrix4fv(scene.material_MV, 1, GL_FALSE, MV.data);
		glUniformMatrix4fv(scene.material_MVP, 1, GL_FALSE, MVP.data);

		// material properties
		glUniform3f(scene.material_ambient, scene.staticGeometryMaterials[i].ambient.x, scene.staticGeometryMaterials[i].ambient.y, scene.staticGeometryMaterials[i].ambient.z);
		glUniform3f(scene.material_diffuse, scene.staticGeometryMaterials[i].diffuse.x, scene.staticGeometryMaterials[i].diffuse.y, scene.staticGeometryMaterials[i].diffuse.z);
		glUniform3f(scene.material_specular, scene.staticGeometryMaterials[i].specular.x, scene.staticGeometryMaterials[i].specular.y, scene.staticGeometryMaterials[i].specular.z);
		glUniform1f(scene.material_shininess, scene.staticGeometryMaterials[i].shininess);

		if (scene.staticGeometryMaterials[i].hasDiffuseTexture)
		{
			glUniform1i(scene.material_hasDiffuseTexture, 1);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, scene.staticGeometryMaterials[i].diffuseTexture.textureID);
		}
		else
		{
			glUniform1i(scene.material_hasDiffuseTexture, 0);
		}

		bindAndDrawMesh(scene.staticGeometry[i]);
	}

	// draw spheres at light positions
	/*bindShader(geometryShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, scene.textures[TEXTURE_GROUND].textureID);
	for (uint32 i = 0; i < min(scene.pointLights.size(), MAX_POINT_LIGHTS); ++i)
	{
	vec3 pos = scene.pointLights[i].position;
	mat4 MV = scene.cam.view * createModelMatrix(pos, quat(), 2.f);
	mat4 MVP = scene.cam.proj * MV;
	glUniformMatrix4fv(scene.material_MV, 1, GL_FALSE, MV.data);
	glUniformMatrix4fv(scene.material_MVP, 1, GL_FALSE, MVP.data);

	bindAndDrawMesh(scene.meshes[MESH_SPHERE]);
	}*/
}

void renderScene(scene_state& scene, uint32 screenWidth, uint32 screenHeight)
{
	loadAllShaders(scene);
	float aspect = (float)screenWidth / (float)screenHeight;
	if (aspect != scene.cam.aspect)
	{
		deleteFBO(scene.frontFaceBuffer);
		deleteFBO(scene.backFaceBuffer);
		initializeFBOs(scene.frontFaceBuffer, scene.backFaceBuffer, screenWidth, screenHeight);

		scene.cam.aspect = aspect;
		scene.cam.proj = createProjectionMatrix(scene.cam.verticalFOV, scene.cam.aspect, scene.cam.nearPlane, scene.cam.farPlane);
		scene.cam.invProj = inverted(scene.cam.proj);

		std::cout << "resize" << std::endl;
	}

	// front faces
	bindFramebuffer(scene.frontFaceBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderGeometry(scene);

	// back faces
	bindFramebuffer(scene.backFaceBuffer);
	glClear(GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_FRONT);
	renderGeometry(scene);
	glCullFace(GL_BACK);
	


	// ssr
	bindDefaultFramebuffer(screenWidth, screenHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	opengl_shader& ssrShader = scene.shaders[SHADER_SSR];
	bindShader(ssrShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, scene.frontFaceBuffer.colorTextures[0]);	// position
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, scene.frontFaceBuffer.colorTextures[1]);	// normal
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, scene.frontFaceBuffer.colorTextures[2]);	// color
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, scene.frontFaceBuffer.depthTexture);		// front face depth
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, scene.backFaceBuffer.depthTexture);		// back face depth

	glUniform2f(scene.ssr_screenDim, (GLfloat)screenWidth, (GLfloat)screenHeight);

	mat4 proj = createScaleMatrix(vec3((float)screenWidth, (float)screenHeight, 1.f)) * createModelMatrix(vec3(0.5f, 0.5f, 0.f), quat(), vec3(0.5f, 0.5f, 1.f)) * scene.cam.proj;

	glUniformMatrix4fv(scene.ssr_proj, 1, GL_FALSE, proj.data);
	glUniformMatrix4fv(scene.ssr_invProj, 1, GL_FALSE, scene.cam.invProj.data);
	
	glUniform2f(scene.ssr_clippingPlanes, scene.cam.nearPlane, scene.cam.farPlane);

	bindAndDrawMesh(scene.plane);
}

void cleanupScene(scene_state& scene)
{
	for (uint32 i = 0; i < SHADER_COUNT; ++i)
		deleteShader(scene.shaders[i]);

	for (opengl_mesh& mesh : scene.staticGeometry)
		deleteMesh(mesh);

	deleteFBO(scene.frontFaceBuffer);
	deleteFBO(scene.backFaceBuffer);
}