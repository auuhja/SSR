#include "scene.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <string>

static bool loadTexture(opengl_texture& texture, const std::string& filename);


#pragma pack(push, 1)
struct vertex3PTN
{
	vec3 pos;
	vec2 tex;
	vec3 nor;
};

struct vertex3PTNT
{
	vec3 pos;
	vec2 tex;
	vec3 nor;
	vec3 tan;
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

static void uploadVertexData(opengl_mesh& mesh, const std::vector<vertex3PTNT>& vertices, const std::vector<uint32>& indices)
{
	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex3PTNT), &vertices[0], GL_STATIC_DRAW);

	// positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex3PTNT), (void*)0);
	// texCoords
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex3PTNT), (void*)(3 * sizeof(GLfloat)));
	// normals
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex3PTNT), (void*)(5 * sizeof(GLfloat)));
	// tangents
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vertex3PTNT), (void*)(8 * sizeof(GLfloat)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &mesh.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32), &indices[0], GL_STATIC_DRAW);

	glBindVertexArray(0);
}

static material loadMaterial(aiMaterial* mat)
{
	material material = { 0 };
	aiString name;
	mat->Get(AI_MATKEY_NAME, name);

	aiColor3D color;
	mat->Get(AI_MATKEY_COLOR_AMBIENT, color);
	material.ambient = vec3(color.r, color.g, color.b);

	mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
	material.diffuse = vec3(color.r, color.g, color.b);

	mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
	material.specular = vec3(color.r, color.g, color.b);

	mat->Get(AI_MATKEY_COLOR_EMISSIVE, color);
	vec3 emissiveColor = vec3(color.r, color.g, color.b);
	if (sqlength(emissiveColor) > 0.f)
	{
		std::cout << name.C_Str() << " is emissive" << std::endl;
		material.emitting = true;
	}

	float shininess;
	mat->Get(AI_MATKEY_SHININESS, shininess);
	material.shininess = shininess / 4; // for some reason this has to be divided by 4
	std::cout << name.C_Str() << " has shininess " << material.shininess << std::endl;

	aiString texPath;
	if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == aiReturn_SUCCESS)
	{
		std::cout << name.C_Str() << " has diffuse: " << texPath.C_Str() << std::endl;
		material.hasDiffuseTexture = true;
		loadTexture(material.diffuseTexture, texPath.C_Str());
	}
	if (mat->GetTexture(aiTextureType_HEIGHT, 0, &texPath) == aiReturn_SUCCESS) // why is the normal map in aiTextureType_HEIGHT???
	{
		std::cout << name.C_Str() << " has normal: " << texPath.C_Str() << std::endl;
		material.hasNormalTexture = true;
		loadTexture(material.normalTexture, texPath.C_Str());
	}
	if (mat->GetTexture(aiTextureType_SPECULAR, 0, &texPath) == aiReturn_SUCCESS)
	{
		std::cout << name.C_Str() << " has specular: " << texPath.C_Str() << std::endl;
		material.hasSpecularTexture = true;
		loadTexture(material.specularTexture, texPath.C_Str());
	}

	return material;
}

// this is expected to be already at the desired world position
static bool loadStaticGeometry(std::vector<opengl_mesh>& meshes, std::vector<material>& materials, const std::string& filename)
{
	std::string filepath = std::string("res/models/") + filename;

	Assimp::Importer Importer;
	const aiScene* aiScene = Importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace
		| aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality);

	if (!aiScene) {
		std::cerr << "File " << filepath << " not found." << std::endl;
		return false;
	}

	uint32 numberOfMeshes = aiScene->mNumMeshes;
	for (uint32 m = 0; m < numberOfMeshes; ++m)
	{
		const aiMesh* aiMesh = aiScene->mMeshes[m];
		opengl_mesh mesh = { 0 };

		std::vector<uint32> indices;
		indices.reserve(aiMesh->mNumFaces * 3);

		uint32 vertexCount = aiMesh->mNumVertices;
		uint32 indexCount = aiMesh->mNumFaces * 3;
		mesh.indexCount = indexCount;

		// indices
		for (uint32 i = 0; i < aiMesh->mNumFaces; i++) {
			const aiFace &face = aiMesh->mFaces[i];
			assert(face.mNumIndices == 3);
			indices.push_back(face.mIndices[0]);
			indices.push_back(face.mIndices[1]);
			indices.push_back(face.mIndices[2]);
		}

		// material
		material material = loadMaterial(aiScene->mMaterials[aiMesh->mMaterialIndex]);

		// vertices
		if (material.hasNormalTexture)
		{
			std::vector<vertex3PTNT> vertices;
			vertices.reserve(aiMesh->mNumVertices);

			for (uint32 i = 0; i < aiMesh->mNumVertices; ++i) {

				const aiVector3D& pos = aiMesh->mVertices[i];
				const aiVector3D& nor = aiMesh->mNormals[i];
				const aiVector3D& tan = aiMesh->mTangents[i];

				vertex3PTNT vertex;
				vertex.pos = vec3(pos.x, pos.y, pos.z);
				vertex.tex = vec2(0, 0);
				if (aiMesh->HasTextureCoords(0))
					vertex.tex = vec2(aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y);

				vertex.nor = vec3(nor.x, nor.y, nor.z);
				vertex.tan = vec3(tan.x, tan.y, tan.z);
				vertices.push_back(vertex);
			}

			uploadVertexData(mesh, vertices, indices);
		}
		else
		{
			std::vector<vertex3PTN> vertices;
			vertices.reserve(aiMesh->mNumVertices);

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

			uploadVertexData(mesh, vertices, indices);
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

// returns start and end index of loaded meshes and materials
static std::pair<uint32, uint32> loadMesh(std::vector<opengl_mesh>& meshes, std::vector<material>& materials, const std::string& filename)
{
	std::string filepath = std::string("res/models/") + filename;

	Assimp::Importer Importer;
	const aiScene* aiScene = Importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_GenSmoothNormals
		| aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality);

	if (!aiScene) {
		std::cerr << "File " << filepath << " not found." << std::endl;
		return std::pair<uint32, uint32>(0, 0);
	}

	uint32 startIndex = (uint32)meshes.size();

	uint32 numberOfMeshes = aiScene->mNumMeshes;

	assert(numberOfMeshes > 0);

	for (uint32 m = 0; m < numberOfMeshes; ++m)
	{
		const aiMesh* aiMesh = aiScene->mMeshes[m];

		std::vector<vertex3PTN> vertices; // this does not support normal mapping for now
		std::vector<uint32> indices;

		material material = loadMaterial(aiScene->mMaterials[aiMesh->mMaterialIndex]);
		opengl_mesh mesh = { 0 };

		vertices.reserve(aiMesh->mNumVertices);
		indices.reserve(aiMesh->mNumFaces * 3);

		uint32 vertexCount = aiMesh->mNumVertices;
		uint32 indexCount = aiMesh->mNumFaces * 3;

		mesh.indexCount = indexCount;


		for (uint32 i = 0; i < aiMesh->mNumVertices; ++i) {

			const aiVector3D& pos = aiMesh->mVertices[i];
			const aiVector3D& nor = aiMesh->mNormals[i];
			const aiVector3D& tex = aiMesh->mTextureCoords[0][i];

			vertex3PTN vertex;
			vertex.pos = vec3(pos.x, pos.y, pos.z);
			vertex.nor = vec3(nor.x, nor.y, nor.z);
			vertex.tex = vec2(0, 0);
			if (aiMesh->HasTextureCoords(0))
				vertex.tex = vec2(aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y);

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

		meshes.push_back(mesh);
		materials.push_back(material);
	}

	uint32 endIndex = (uint32)meshes.size();

	return std::pair<uint32, uint32>(startIndex, endIndex);
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

static bool initializeFBOs(opengl_renderer& renderer)
{
	createFBO(renderer.frontFaceBuffer, renderer.width, renderer.height);
	attachColorAttachment(renderer.frontFaceBuffer, GL_RGB32F, GL_FLOAT);		// positions
	attachColorAttachment(renderer.frontFaceBuffer, GL_RGB16F, GL_FLOAT);		// normals
	attachColorAttachment(renderer.frontFaceBuffer, GL_RGB8, GL_UNSIGNED_BYTE);	// color
	attachColorAttachment(renderer.frontFaceBuffer, GL_R8, GL_UNSIGNED_BYTE);	// shininess
	attachDepthAttachment(renderer.frontFaceBuffer);
	bool frontFaceBufferSucess = finishFBO(renderer.frontFaceBuffer);

	createFBO(renderer.backFaceBuffer, renderer.width / 4, renderer.height / 4);
	attachDepthAttachment(renderer.backFaceBuffer);
	bool backFaceBufferSuccess = finishFBO(renderer.backFaceBuffer);

	createFBO(renderer.reflectionBuffer, renderer.width / 2, renderer.height / 2);
	attachColorAttachment(renderer.reflectionBuffer, GL_RGBA, GL_UNSIGNED_BYTE);
	bool reflectionBufferSuccess = finishFBO(renderer.reflectionBuffer);

	createFBO(renderer.lastFrameBuffer, renderer.width, renderer.height);
	attachColorAttachment(renderer.lastFrameBuffer, GL_RGBA8, GL_UNSIGNED_BYTE);
	bool lastFrameBufferSuccess = finishFBO(renderer.lastFrameBuffer);

	createFBO(renderer.tmpBuffer, renderer.width / 2, renderer.height / 2);
	attachColorAttachment(renderer.tmpBuffer, GL_RGBA, GL_UNSIGNED_BYTE);
	bool tmpBufferSuccess = finishFBO(renderer.tmpBuffer);

	bindDefaultFramebuffer(renderer.width, renderer.height);

	return frontFaceBufferSucess && backFaceBufferSuccess && reflectionBufferSuccess && lastFrameBufferSuccess && tmpBufferSuccess;
}

static void blitFrameBuffer(opengl_fbo& from, uint32 fromIndex, opengl_fbo& to, uint32 toIndex)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, from.fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + fromIndex);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, to.fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + toIndex);

	glBlitFramebuffer(0, 0, from.width, from.height,
		0, 0, to.width, to.height,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

static void blitFrameBuffer(opengl_fbo& from, uint32 fromIndex, opengl_fbo& to, uint32 toIndex, 
	uint32 topLeftX, uint32 topLeftY, uint32 bottomRightX, uint32 bottomRightY)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, from.fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + fromIndex);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, to.fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + toIndex);

	glBlitFramebuffer(0, 0, from.width, from.height,
		topLeftX, topLeftY, bottomRightX, bottomRightY,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

static void blitFrameBufferToScreen(opengl_fbo& from, uint32 fromIndex, uint32 width, uint32 height)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, from.fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + fromIndex);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glBlitFramebuffer(0, 0, from.width, from.height,
		0, 0, width, height,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

static void blitFrameBufferToScreen(opengl_fbo& from, uint32 fromIndex, 
	uint32 topLeftX, uint32 topLeftY, uint32 bottomRightX, uint32 bottomRightY)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, from.fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + fromIndex);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glBlitFramebuffer(0, 0, from.width, from.height,
		topLeftX, topLeftY, bottomRightX, bottomRightY,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

static bool loadAllShaders(opengl_renderer& renderer)
{
	bool reloaded = false;
	{
		opengl_shader& shader = renderer.shaders[SHADER_GEOMETRY];
		if (loadShader(shader, "geometry_shader.glsl"))
		{
			bindShader(shader);
			renderer.geometry_MVP = glGetUniformLocation(shader.programID, "MVP");
			renderer.geometry_MV = glGetUniformLocation(shader.programID, "MV");
			renderer.geometry_ambient = glGetUniformLocation(shader.programID, "ambient");
			renderer.geometry_diffuse = glGetUniformLocation(shader.programID, "diffuse");
			renderer.geometry_specular = glGetUniformLocation(shader.programID, "specular");
			renderer.geometry_shininess = glGetUniformLocation(shader.programID, "shininess");
			renderer.geometry_emitting = glGetUniformLocation(shader.programID, "emitting");
			renderer.geometry_hasDiffuseTexture = glGetUniformLocation(shader.programID, "hasDiffuseTexture");
			renderer.geometry_hasNormalTexture = glGetUniformLocation(shader.programID, "hasNormalTexture");
			renderer.geometry_hasSpecularTexture = glGetUniformLocation(shader.programID, "hasSpecularTexture");
			renderer.geometry_numberOfPointLights = glGetUniformLocation(shader.programID, "numberOfPointLights");

			for (uint32 i = 0; i < MAX_POINT_LIGHTS; ++i)
			{
				std::string uniformName = std::string("pointLights[") + std::to_string(i) + "].";
				renderer.geometry_pl_position[i] = glGetUniformLocation(shader.programID, (uniformName + "position").c_str());
				renderer.geometry_pl_radius[i] = glGetUniformLocation(shader.programID, (uniformName + "radius").c_str());
				renderer.geometry_pl_color[i] = glGetUniformLocation(shader.programID, (uniformName + "color").c_str());
			}

			glUniform1i(glGetUniformLocation(shader.programID, "diffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(shader.programID, "normalTexture"), 1);
			glUniform1i(glGetUniformLocation(shader.programID, "specularTexture"), 2);			

			reloaded = true;
		}
	}
	{
		opengl_shader& shader = renderer.shaders[SHADER_SSR];
		if (loadShader(shader, "ssr_shader.glsl"))
		{
			bindShader(shader);
			renderer.ssr_proj = glGetUniformLocation(shader.programID, "proj");
			renderer.ssr_toPrevFramePos = glGetUniformLocation(shader.programID, "toPrevFramePos");
			renderer.ssr_clippingPlanes = glGetUniformLocation(shader.programID, "clippingPlanes");
			
			glUniform1i(glGetUniformLocation(shader.programID, "positionTexture"), 0);
			glUniform1i(glGetUniformLocation(shader.programID, "normalTexture"), 1);
			glUniform1i(glGetUniformLocation(shader.programID, "lastFrameColorTexture"), 2);
			glUniform1i(glGetUniformLocation(shader.programID, "shininessTexture"), 3);
			glUniform1i(glGetUniformLocation(shader.programID, "depthTexture"), 4);
			glUniform1i(glGetUniformLocation(shader.programID, "backfaceDepthTexture"), 5);

			reloaded = true;
		}
	}
	{
		opengl_shader& shader = renderer.shaders[SHADER_BLUR];
		if (loadShader(shader, "blur_shader.glsl"))
		{
			bindShader(shader);

			renderer.blur_blurDirection = glGetUniformLocation(shader.programID, "blurDirection");

			glUniform1i(glGetUniformLocation(shader.programID, "inputTexture"), 0);

			reloaded = true;
		}
	}
	{
		opengl_shader& shader = renderer.shaders[SHADER_RESULT];
		if (loadShader(shader, "result_shader.glsl"))
		{
			bindShader(shader);

			glUniform1i(glGetUniformLocation(shader.programID, "colorTexture"), 0);
			glUniform1i(glGetUniformLocation(shader.programID, "reflectionTexture"), 1);

			reloaded = true;
		}
	}

	return reloaded;
}

void initializeRenderer(opengl_renderer& renderer, uint32 screenWidth, uint32 screenHeight)
{
	renderer.width = screenWidth;
	renderer.height = screenHeight;

	bool fboSuccess = initializeFBOs(renderer);
	if (!fboSuccess)
	{
		std::cout << "initializing FBOs failed!" << std::endl;
	}
	
	// shaders
	{
		for (uint32 i = 0; i < SHADER_COUNT; ++i)
			renderer.shaders[i].writeTime = 0;
		loadAllShaders(renderer);
	}

	loadMesh(renderer.plane, "plane.obj");
	loadMesh(renderer.sphere, "sphere.obj");

	glClearColor(0.18f, 0.35f, 0.5f, 1.0f);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
}

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

static void renderGeometry(opengl_renderer& renderer, scene_state& scene)
{
	opengl_shader& geometryShader = renderer.shaders[SHADER_GEOMETRY];
	bindShader(geometryShader);

	glUniform1i(renderer.geometry_numberOfPointLights, (int32)scene.pointLights.size());
	for (uint32 i = 0; i < min(scene.pointLights.size(), MAX_POINT_LIGHTS); ++i)
	{
		vec4 posVS = scene.cam.view * vec4(scene.pointLights[i].position, 1.f);
		glUniform3f(renderer.geometry_pl_position[i], posVS.x, posVS.y, posVS.z);
		glUniform1f(renderer.geometry_pl_radius[i], scene.pointLights[i].radius);
		glUniform3f(renderer.geometry_pl_color[i], scene.pointLights[i].color.x, scene.pointLights[i].color.y, scene.pointLights[i].color.z);
	}

	mat4 MV = scene.cam.view;
	mat4 MVP = scene.cam.proj * MV;

	for (uint32 i = 0; i < scene.staticGeometry.size(); ++i)
	{
		glUniformMatrix4fv(renderer.geometry_MV, 1, GL_FALSE, MV.data);
		glUniformMatrix4fv(renderer.geometry_MVP, 1, GL_FALSE, MVP.data);

		// material properties
		glUniform3f(renderer.geometry_ambient, scene.staticGeometryMaterials[i].ambient.x, scene.staticGeometryMaterials[i].ambient.y, scene.staticGeometryMaterials[i].ambient.z);
		glUniform3f(renderer.geometry_diffuse, scene.staticGeometryMaterials[i].diffuse.x, scene.staticGeometryMaterials[i].diffuse.y, scene.staticGeometryMaterials[i].diffuse.z);
		glUniform3f(renderer.geometry_specular, scene.staticGeometryMaterials[i].specular.x, scene.staticGeometryMaterials[i].specular.y, scene.staticGeometryMaterials[i].specular.z);
		glUniform1f(renderer.geometry_shininess, scene.staticGeometryMaterials[i].shininess);

		if (scene.staticGeometryMaterials[i].hasDiffuseTexture)
		{
			glUniform1i(renderer.geometry_hasDiffuseTexture, 1);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, scene.staticGeometryMaterials[i].diffuseTexture.textureID);
		}
		else
		{
			glUniform1i(renderer.geometry_hasDiffuseTexture, 0);
		}

		if (scene.staticGeometryMaterials[i].hasNormalTexture)
		{
			glUniform1i(renderer.geometry_hasNormalTexture, 1);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, scene.staticGeometryMaterials[i].normalTexture.textureID);
		}
		else
		{
			glUniform1i(renderer.geometry_hasNormalTexture, 0);
		}

		if (scene.staticGeometryMaterials[i].hasSpecularTexture)
		{
			glUniform1i(renderer.geometry_hasSpecularTexture, 1);

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, scene.staticGeometryMaterials[i].specularTexture.textureID);
		}
		else
		{
			glUniform1i(renderer.geometry_hasSpecularTexture, 0);
		}

		glUniform1i(renderer.geometry_emitting, scene.staticGeometryMaterials[i].emitting ? 1 : 0);

		bindAndDrawMesh(scene.staticGeometry[i]);
	}

	for (uint32 i = 0; i < scene.entities.size(); ++i)
	{
		entity& ent = scene.entities[i];
		mat4 MV = scene.cam.view * sqtToMat4(ent.position);
		mat4 MVP = scene.cam.proj * MV;

		glUniformMatrix4fv(renderer.geometry_MV, 1, GL_FALSE, MV.data);
		glUniformMatrix4fv(renderer.geometry_MVP, 1, GL_FALSE, MVP.data);

		for (uint32 m = ent.meshStartIndex; m < ent.meshEndIndex; ++m)
		{

			// material properties
			glUniform3f(renderer.geometry_ambient, scene.materials[m].ambient.x, scene.materials[m].ambient.y, scene.materials[m].ambient.z);
			glUniform3f(renderer.geometry_diffuse, scene.materials[m].diffuse.x, scene.materials[m].diffuse.y, scene.materials[m].diffuse.z);
			glUniform3f(renderer.geometry_specular, scene.materials[m].specular.x, scene.materials[m].specular.y, scene.materials[m].specular.z);
			glUniform1f(renderer.geometry_shininess, scene.materials[m].shininess);

			if (scene.materials[m].hasDiffuseTexture)
			{
				glUniform1i(renderer.geometry_hasDiffuseTexture, 1);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, scene.materials[m].diffuseTexture.textureID);
			}
			else
			{
				glUniform1i(renderer.geometry_hasDiffuseTexture, 0);
			}

			if (scene.materials[m].hasNormalTexture)
			{
				glUniform1i(renderer.geometry_hasNormalTexture, 1);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, scene.materials[m].normalTexture.textureID);
			}
			else
			{
				glUniform1i(renderer.geometry_hasNormalTexture, 0);
			}

			if (scene.materials[m].hasSpecularTexture)
			{
				glUniform1i(renderer.geometry_hasSpecularTexture, 1);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, scene.materials[m].specularTexture.textureID);
			}
			else
			{
				glUniform1i(renderer.geometry_hasSpecularTexture, 0);
			}

			glUniform1i(renderer.geometry_emitting, scene.materials[m].emitting ? 1 : 0);

			bindAndDrawMesh(scene.geometry[m]);
		}
	}

#if 0
	for (point_light& pl : scene.pointLights)
	{
		vec3 pos = pl.position;
		mat4 MV = scene.cam.view * createModelMatrix(pos, quat(), 1.f);
		mat4 MVP = scene.cam.proj * MV;
		glUniformMatrix4fv(renderer.geometry_MV, 1, GL_FALSE, MV.data);
		glUniformMatrix4fv(renderer.geometry_MVP, 1, GL_FALSE, MVP.data);

		bindAndDrawMesh(renderer.sphere);
	}
#endif
}

void renderScene(opengl_renderer& renderer, scene_state& scene, uint32 screenWidth, uint32 screenHeight, bool debugRendering)
{
	loadAllShaders(renderer);

	// adapt screen on resize
	if (screenWidth != renderer.width || screenHeight != renderer.height)
	{
		renderer.width = screenWidth;
		renderer.height = screenHeight;
		deleteFBO(renderer.frontFaceBuffer);
		deleteFBO(renderer.backFaceBuffer);
		deleteFBO(renderer.lastFrameBuffer);
		deleteFBO(renderer.reflectionBuffer);
		deleteFBO(renderer.tmpBuffer);
		initializeFBOs(renderer);
	}

	if (screenWidth != scene.cam.width || screenHeight != scene.cam.height)
	{
		scene.cam.width = screenWidth;
		scene.cam.height = screenHeight;
		float aspect = (float)screenWidth / (float)screenHeight;
		scene.cam.proj = createProjectionMatrix(scene.cam.verticalFOV, aspect, scene.cam.nearPlane, scene.cam.farPlane);
		scene.cam.toPrevFramePos = scene.cam.proj;
		std::cout << "resize" << std::endl;
	}

	// front faces
	bindFramebuffer(renderer.frontFaceBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderGeometry(renderer, scene);

	// back faces
	bindFramebuffer(renderer.backFaceBuffer);
	glClear(GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_FRONT);
	renderGeometry(renderer, scene);
	glCullFace(GL_BACK);
	
	// ssr
	bindFramebuffer(renderer.reflectionBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	opengl_shader& ssrShader = renderer.shaders[SHADER_SSR];
	bindShader(ssrShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderer.frontFaceBuffer.colorTextures[0]);	// position
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, renderer.frontFaceBuffer.colorTextures[1]);	// normal
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, renderer.lastFrameBuffer.colorTextures[0]);	// prev frame
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, renderer.frontFaceBuffer.colorTextures[3]);	// shininess
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, renderer.frontFaceBuffer.depthTexture);		// front face depth
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, renderer.backFaceBuffer.depthTexture);			// back face depth

	mat4 proj = createScaleMatrix(vec3((float)screenWidth, (float)screenHeight, 1.f)) * createModelMatrix(vec3(0.5f, 0.5f, 0.f), quat(), vec3(0.5f, 0.5f, 1.f)) * scene.cam.proj;

	glUniformMatrix4fv(renderer.ssr_proj, 1, GL_FALSE, proj.data);
	glUniformMatrix4fv(renderer.ssr_toPrevFramePos, 1, GL_FALSE, scene.cam.toPrevFramePos.data);
	
	glUniform2f(renderer.ssr_clippingPlanes, scene.cam.nearPlane, scene.cam.farPlane);

	bindAndDrawMesh(renderer.plane);

	if (debugRendering)
	{
		blitFrameBufferToScreen(renderer.frontFaceBuffer, 2, 0, screenHeight / 2, screenWidth / 2, screenHeight);			 // top left: image without reflections
		blitFrameBufferToScreen(renderer.reflectionBuffer, 0, screenWidth / 2, screenHeight / 2, screenWidth, screenHeight); // top right: reflection buffer
		blitFrameBufferToScreen(renderer.frontFaceBuffer, 3, 0, 0, screenWidth / 2, screenHeight / 2);						 // bottom left: shininess
	}

	// blur reflection buffer
	bindFramebuffer(renderer.tmpBuffer);
	glClear(GL_COLOR_BUFFER_BIT);
	opengl_shader& blurShader = renderer.shaders[SHADER_BLUR];
	bindShader(blurShader);
	glUniform2f(renderer.blur_blurDirection, 1, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderer.reflectionBuffer.colorTextures[0]);
	bindAndDrawMesh(renderer.plane);
	bindFramebuffer(renderer.reflectionBuffer);
	glClear(GL_COLOR_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderer.tmpBuffer.colorTextures[0]);
	glUniform2f(renderer.blur_blurDirection, 0, 1);
	bindAndDrawMesh(renderer.plane);

	// bring it together - save for next frame
	bindFramebuffer(renderer.lastFrameBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	opengl_shader& resultShader = renderer.shaders[SHADER_RESULT];
	bindShader(resultShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderer.frontFaceBuffer.colorTextures[2]);	// color
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, renderer.reflectionBuffer.colorTextures[0]);	// reflected color

	bindAndDrawMesh(renderer.plane);

	// blit to screen
	if (debugRendering)
	{
		blitFrameBufferToScreen(renderer.reflectionBuffer, 0, screenWidth / 2, 0, screenWidth, screenHeight / 2);
	}
	else
	{
		blitFrameBufferToScreen(renderer.lastFrameBuffer, 0, screenWidth, screenHeight);
	}

}

void cleanupRenderer(opengl_renderer& renderer)
{
	for (uint32 i = 0; i < SHADER_COUNT; ++i)
		deleteShader(renderer.shaders[i]);
	deleteFBO(renderer.frontFaceBuffer);
	deleteFBO(renderer.backFaceBuffer);
	deleteFBO(renderer.lastFrameBuffer);
	deleteFBO(renderer.reflectionBuffer);
	deleteFBO(renderer.tmpBuffer);
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