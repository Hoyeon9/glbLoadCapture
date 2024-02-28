#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>
using namespace std;

const string textureNames[22] = {
	"none",	"diffuse", "specular", "ambient", "emissive", "height", "normal", "shininess",	"opacity",
	"displacement", "lightmap", "reflection", "albedo", "normal_camera", "emission",
	"metallic", "roughness", "ao", "unknown", "sheen", "clearcoat", "transmission"
};

struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
	glm::vec3 Tangent;
};
struct Texture {
	unsigned int id;
	string type;
	string path;
	int channels;
};

class Mesh {
public:
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<Texture> textures;

	Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures);
	void Draw(unsigned int shaderID);
	void deleteMesh();
private:
	unsigned int VAO, VBO, EBO;
	void setupMesh();
};
Mesh::Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures) {
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;
	setupMesh();
}
void Mesh::setupMesh() {
	glGenVertexArrays(1, &this->VAO);
	glGenBuffers(1, &this->VBO);
	glGenBuffers(1, &this->EBO);

	glBindVertexArray(this->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
	glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(unsigned int), &this->indices[0], GL_STATIC_DRAW);

	//vertex
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	//texture coodinates
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	//normals
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	//tangents
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

	glBindVertexArray(0);
}
void Mesh::Draw(unsigned int shaderID) {
	unsigned int ctnArr[22];
	for (int i = 0; i < 22; i++) {
		ctnArr[i] = 1;
	}

	for (int i = 0; i < this->textures.size(); i++) {
		glActiveTexture(GL_TEXTURE3 + i);
		string number;
		string name = this->textures[i].type;
		for (int j = 0; j < 22; j++) {
			if(name == "texture_"+textureNames[j])
				number = std::to_string(ctnArr[j]++);
			if (name == "texture_diffuse")
				glUniform1i(glGetUniformLocation(shaderID, "albedoChannels"), this->textures[i].channels);
		}
		glUniform1i(glGetUniformLocation(shaderID, ("material."+name + number).c_str()), 3 + i);
		glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
	}

	//draw
	glBindVertexArray(this->VAO);
	glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}
void Mesh::deleteMesh() {	
	glDeleteBuffers(1, &this->VBO);
	glDeleteBuffers(1, &this->EBO);
	glDeleteVertexArrays(1, &this->VAO);
	for (int i = 0; i < this->textures.size(); i++) {
		glDeleteTextures(1, &this->textures[i].id);
	}
}
#endif
