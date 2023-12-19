#ifndef MODEL_H
#define MODEL_H

#include <iomanip>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <mesh.h>
#include <shader.h>
#include <vector>
#include <string>
#include <stb_image.h>

unsigned int TextureFromFile(const char* path, const string& directory);
unsigned int TextureFromMemory(aiTexel* data, unsigned int len, int *channels);

class Model {
public:
	Model(string path) {
		loadModel(path);
	}
	void Draw(unsigned int shaderID);
	Texture getTexture(int num);
	unsigned int getTextureNum();
	vector<glm::vec3> getAllVertices();
	void deleteModel();
private:
	const aiScene *scene;
	vector<Mesh> meshes;
	string directory;
	vector<Texture> textures_loaded;
	const aiTexture* embeddedTextures;

	void loadModel(string path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	vector<Texture> loadMaterialtextures(aiMaterial* material, aiTextureType type, string typeName);
};
void Model::Draw(unsigned int shaderID) {
	for (int i = 0; i < this->meshes.size(); i++) {
		this->meshes[i].Draw(shaderID);
	}
}
Texture Model::getTexture(int num) {
	return this->textures_loaded[num];
}
unsigned int Model::getTextureNum() {
	return this->textures_loaded.size();
}
vector<glm::vec3> Model::getAllVertices() {
	vector<glm::vec3> allVertices;
	for (int i = 0; i < this->meshes.size(); i++) {
		for (int j = 0; j < this->meshes[i].vertices.size(); j++) {
			allVertices.push_back(this->meshes[i].vertices[j].Position);
		}
	}
	return allVertices;
}
void Model::loadModel(string path) {
	cout << "Starting model loading...\n";
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(
		path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
	);
	this->scene = scene;
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		cout << "ERROR::ASSIMP" << import.GetErrorString() << endl;
		return;
	}
	cout << "File reading done\n";
	this->directory = path.substr(0, path.find_last_of('/'));
	cout << "Processing Node..\n";
	processNode(scene->mRootNode, scene); //process all of the scene's nodes recursively
	cout << "Processing done from " << path << "\n";
}
void Model::processNode(aiNode* node, const aiScene* scene) {
	//cout << "processNode: " << node->mName.C_Str() << endl;
	for (unsigned int i = 0; i < node->mNumMeshes; i++) { //for all the meshes included in the node
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		this->meshes.push_back(processMesh(mesh, scene));
	}
	//cout<<"processNode_pushed all the meshes of: "<< node->mName.C_Str() << endl;
	//same for its children
	//cout << "Call processNode for children of: " << node->mName.C_Str() << endl;
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processNode(node->mChildren[i], scene);
	}
	//cout << "processNode_Done: " << node->mName.C_Str() << "------------" << endl;
}
Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<Texture> textures;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		Vertex vertex;
		glm::vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.Position = vector;
		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.Normal = vector;
		vector.x = mesh->mTangents[i].x;
		vector.y = mesh->mTangents[i].y;
		vector.z = mesh->mTangents[i].z;
		vertex.Tangent = vector;
		if (mesh->mTextureCoords[0]) {
			glm::vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoords = vec;
		}
		else {
			vertex.TexCoords = glm::vec2(0.0f, 0.0f);
		}
		vertices.push_back(vertex);
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	if (mesh->mMaterialIndex >= 0) {
		cout << "Loading materials..\n";
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		/*//Count materials
		for (int i = aiTextureType_NONE; i <= aiTextureType_TRANSMISSION; i++) {
			cout << setw(13) << left << textureNames[i] << ": ";
			if (material->GetTextureCount((aiTextureType)i) > 0)
				cout << material->GetTextureCount((aiTextureType)i);
			else
				cout << 0 << " ";
			cout << endl;
		}*/
		
		for (int i = 0; i < 22; i++) {
			vector<Texture> tempMaps = loadMaterialtextures(material, (aiTextureType)i, "texture_"+textureNames[i]);
			textures.insert(textures.end(), tempMaps.begin(), tempMaps.end());
		}
		cout << "Materials loading done\n";
	}
	
	return Mesh(vertices, indices, textures);
}
vector<Texture> Model::loadMaterialtextures(aiMaterial* material, aiTextureType type, string typeName) {
	vector<Texture> textures;
	for (unsigned int i = 0; i < material->GetTextureCount(type); i++) {
		//cout << typeName << " " << i << endl;
		aiString str;
		material->GetTexture(type, i, &str);
		
		const aiTexture *extractedTex = this->scene->GetEmbeddedTexture(str.C_Str());

		bool skip = false;
		for (int j = 0; j < this->textures_loaded.size(); j++) {
			if (this->textures_loaded[j].type == typeName && std::strcmp(this->textures_loaded[j].path.data(), str.C_Str()) == 0) {
				textures.push_back(this->textures_loaded[j]);
				skip = true;
				break;
			}
		}
		if (!skip) {
			//cout << "load texture from file: " << str.C_Str() <<endl;
			aiTexel* data = extractedTex->pcData;
			
			Texture texture;
			int channels;
			texture.id = TextureFromMemory(data, extractedTex->mWidth + 1, &channels);
			texture.type = typeName;
			texture.path = str.C_Str();
			texture.channels = channels;
			textures.push_back(texture);

			this->textures_loaded.push_back(texture);
		}
	}
	return textures;
}
void Model::deleteModel() {
	for (int i = 0; i < this->textures_loaded.size(); i++) {
		glDeleteTextures(1, &this->textures_loaded[i].id);
	}
	for (int i = 0; i < this->meshes.size(); i++) {
		this->meshes[i].deleteMesh();
	}
}
unsigned int TextureFromFile(const char* path, const string& directory) {
	string filename = string(path);
	filename = directory + '/' + filename;

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);
	GLenum format;
	if (nrChannels == 1) format = GL_RED;
	else if (nrChannels == 2) format = GL_RG;
	else if (nrChannels == 3) format = GL_RGB; 
	else if (nrChannels == 4) format = GL_RGBA;
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);  //free memory
	return texture;
}
unsigned int TextureFromMemory(aiTexel* texelData, unsigned int len, int *channels) {
	unsigned char* stbiImageData = new unsigned char[len];
	int components = 4;
	for (int i = 0; i < len / components; i++) {
		stbiImageData[i * components + 0] = texelData[i].b;
		stbiImageData[i * components + 1] = texelData[i].g;
		stbiImageData[i * components + 2] = texelData[i].r;
		stbiImageData[i * components + 3] = texelData[i].a;
	}

	int width, height, nrChannels;
	unsigned char *data = stbi_load_from_memory(stbiImageData, len, &width, &height, &nrChannels, 0);
	//cout << "Texture's x, y, channels: " << (unsigned int)width << " " << (unsigned int)height << " " << (unsigned int)nrChannels << endl;
	*channels = nrChannels;

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLenum format;
	if (nrChannels == 1) format = GL_RED;
	else if (nrChannels == 2) format = GL_RG;
	else if (nrChannels == 3) format = GL_RGB;
	else if (nrChannels == 4) format = GL_RGBA;
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		const char* errorMessage = stbi_failure_reason();
		std::cout << "Failed to load texture::" << "Image loading error: "<< errorMessage <<  std::endl;
	}
	stbi_image_free(data);  //free memory
	delete[] stbiImageData;
	return texture;
}

#endif
