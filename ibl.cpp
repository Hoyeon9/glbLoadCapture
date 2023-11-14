#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <iostream>
using namespace std;

string modelsLoc = "D:\\Amazon_models\\abo-3dmodels\\3dmodels\\original";
string hdrLoc = "C:\\src\\glbLoadCapture\\hdr\\";
string textureLoc = "C:\\Users\\gcoh0\\source\\repos\\glbLoadCapture\\testSave\\";
string savePath = "C:\\src\\capture_231113\\";
const int CAPTURE_WIDTH = 800;
const int CAPTURE_HEIGHT = 600;
const int ENV_RESOLUTION = 1024;
float clearColor = 0.5f;
const int irradResolution = 32;
const int prefResolution = 128;

#include <map>
#include <experimental/filesystem>
namespace fs = experimental::filesystem;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <math.h>
#include <stb_image.h>
#include "model.h"
#include "shader.h"

#define GL_BGR_EXT 0x80E0
#include <opencv2/opencv.hpp>
using namespace cv;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
GLuint loadTexture(char const* texPath);
GLuint loadHDR(char const* texPath);
vector<string> LoadFileList(string root);
GLuint envFormEqui(GLuint equiProgram, GLuint equiTexture);
GLuint irradFromEnv(GLuint irradianceShader, GLuint envCubemap);
GLuint prefiltFromEnv(GLuint prefilterShader, GLuint envCubemap);
GLuint brdfFromEnv(GLuint brdfShader);
void rotateCapture(Model loadedModel, GLuint renderProgram, string fileName, glm::mat4 model);
void captureImage(string fileName);
void captureTextureImage(GLuint texture, string fileName); //For debugging
void captureCubeTextureImage(GLuint texture, string fileName); //For debugging
double boundingBox(vector<glm::vec3> vertices, glm::vec3& boxCtr);
GLuint cubeFromPng(int texNum, string type);

GLuint captureFBO, captureRBO, skyboxVAO, quadVAO;
vector<glm::vec3> bBoxCoords;
unsigned int lightShader, bBoxVAO;

float capRad = 2.6;
glm::vec3 cameraPos = glm::vec3(0.0f, 0.5f, 1.5f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
glm::mat4 captureViews[] =
{
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};
//float deltaTime = 0.0f;
//float lastFrame = 0.0f;
float fov = 45.0f;

string renderModes[] = {
		"IBL",
		"Albedo",
		"Metallic-Roughness",
		"Metallic",
		"Roughness",
		"AO",
		"Mask"
};

int main() {
	//Create repo
	if (!fs::exists(savePath + "A")) {
		for (int i = 48; i <= 57; i++) {
			char buff[256];
			sprintf(buff, "mkdir %s%c", savePath.c_str(), i);
			system(buff);
		}
		for (int i = 65; i <= 90; i++) {
			char buff[256];
			sprintf(buff, "mkdir %s%c", savePath.c_str(), i);
			system(buff);
		}
		std::cout << "Save repositories are created\n";
	}


	//---Creating window(from initialize GLFW~~)---
	glfwInit(); //initilalizes GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //(possible options, valule of our options)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); //use major, minor both 3 version
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //core profile-not forward-compatible
	glfwWindowHint(GLFW_SAMPLES, 9);
	GLFWwindow* window = glfwCreateWindow(CAPTURE_WIDTH, CAPTURE_HEIGHT, "GLB Load Capture", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window\n";
		glfwTerminate();
		return -1;
	}
	glfwSetWindowPos(window, 200, 100);
	glfwMakeContextCurrent(window);


	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		//pass GLAD the function to load the address of the OpenGL function pointers which is OS-specific
		//glfwGetProcAddress: defines the correct function based on which OS we're compiling for
		std::cout << "Failed to initialize GLAD\n";
		return -1;
	}

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	//callback function, it will resize window size when the change is detected
	glClearColor(clearColor, clearColor, clearColor, 1.0f);

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	std::cout << "Loading paths...\n";
	auto modelPaths = LoadFileList(modelsLoc);
	vector<string> hdrPaths = LoadFileList(hdrLoc);
	std::cout << "Path loading done\n";

	std::cout << "Preparing sources..\n";
	//Loading shaders--------------------------
	unsigned int equiProgram = loadShader("hdr2cube.vs", "hdr2cube.fs");
	unsigned int irradianceShader = loadShader("hdr2cube.vs", "irradiance.fs");
	unsigned int prefilterShader = loadShader("hdr2cube.vs", "prefilter.fs");
	unsigned int brdfShader = loadShader("preBRDF.vs", "preBRDF.fs");
	unsigned int skyboxProgram = loadShader("skybox.vs", "skybox.fs");
	unsigned int quadShader = loadShader("quad.vs", "quad.fs");
	lightShader = loadShader("lightVer.vs", "lightFrag.fs");

	unsigned int renderProgram = loadShader("render.vs", "renderGLB.fs");

	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};
	float quadVertices[] = {
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};
	// skybox VAO
	unsigned int skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	// quad VAO
	unsigned int quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	
	//Framebuffer setting to generate sources
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	//Preprocess
	vector<GLuint> processedTextures;
	for (int i = 0; i < hdrPaths.size(); i++) {
		unsigned int equiTexture = loadHDR(hdrPaths[i].c_str());
		unsigned int envCubemap = envFormEqui(equiProgram, equiTexture);
		unsigned int irradianceMap = irradFromEnv(irradianceShader, envCubemap);
		unsigned int prefiltedMap = prefiltFromEnv(prefilterShader, envCubemap);
		unsigned int brdfLUTTexture = brdfFromEnv(brdfShader);

		//unsigned int irradianceMap = cubeFromPng(i, "irrad");
		//unsigned int prefiltedMap = cubeFromPng(i, "pref");

		//captureTextureImage(equiTexture, "hdr" + to_string(i + 1) + "_equi.png");
		//captureCubeTextureImage(envCubemap, "hdr" + to_string(i + 1) + "_env");
		//captureCubeTextureImage(irradianceMap, "hdr" + to_string(i + 1) + "_irrad");
		//captureCubeTextureImage(irradianceMap, "hdr" + to_string(i + 1) + "_irrad_load");
		//captureCubeTextureImage(prefiltedMap, "hdr" + to_string(i + 1) + "_pref");
		//captureCubeTextureImage(prefiltedMap, "hdr" + to_string(i + 1) + "_load_pref");
		//captureTextureImage(brdfLUTTexture, "hdr" + to_string(i + 1) + "_brdf.png");

		processedTextures.push_back(irradianceMap);
		processedTextures.push_back(prefiltedMap);
		processedTextures.push_back(brdfLUTTexture);

		cout << "Texture " << i + 1 << " done\n";
	}
	
	/*
	glUseProgram(equiProgram);
	glUniform1i(glGetUniformLocation(equiProgram, ("equirectangularMap")), 0);

	//Set captureFBO
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	unsigned int envCubemap;
	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		// note that we store each face with 16 bit floating point values
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
			1024, 1024, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	// convert HDR equirectangular environment map to cubemap equivalent
	glUniformMatrix4fv(glGetUniformLocation(equiProgram, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, equiTexture);
	
	glViewport(0, 0, 1024, 1024); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i) {
		glUniformMatrix4fv(glGetUniformLocation(equiProgram, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//renderCube(); // renders a 1x1 cube
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, equiTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	//-------------------------equirectanglar-------------------------------
	
	//Irradiance Map-------------------------------
	unsigned int irradianceMap;
	glGenTextures(1, &irradianceMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0,
			GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//reuse capture framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

	glUseProgram(irradianceShader);
	glUniform1i(glGetUniformLocation(irradianceShader, ("environmentMap")), 0);
	glUniformMatrix4fv(glGetUniformLocation(irradianceShader, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i) {
		glUniformMatrix4fv(glGetUniformLocation(irradianceShader, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//renderCube();
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//----------------irradiance-------------

	//Prefiltered Map----------------------------------
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	unsigned int prefilterMap;
	glGenTextures(1, &prefilterMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glUseProgram(prefilterShader);
	glUniform1i(glGetUniformLocation(prefilterShader, ("environmentMap")), 0);
	glUniformMatrix4fv(glGetUniformLocation(prefilterShader, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
	{
		// reisze framebuffer according to mip-level size.
		unsigned int mipWidth = 128 * std::pow(0.5, mip);
		unsigned int mipHeight = 128 * std::pow(0.5, mip);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		glUniform1f(glGetUniformLocation(prefilterShader, ("roughness")), roughness);
		for (unsigned int i = 0; i < 6; ++i) {
			glUniformMatrix4fv(glGetUniformLocation(prefilterShader, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//renderCube();
			glBindVertexArray(skyboxVAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//--------------------prefiltered----------------------

	//Pre-computing BRDF
	unsigned int brdfLUTTexture;
	glGenTextures(1, &brdfLUTTexture);

	// pre-allocate enough memory for the LUT texture.
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

	glViewport(0, 0, 512, 512);
	glUseProgram(brdfShader);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//RenderQuad();
	glBindVertexArray(quadVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//-----------------specular BRDF---------------*/
	
	//glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_MULTISAMPLE);

	//To original size
	int scrWidth, scrHeight;
	glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
	glViewport(0, 0, scrWidth, scrHeight);

	//lights
	/*glm::vec3 lightPositions[] = {
		glm::vec3(-10.0f,  10.0f, 10.0f),
		glm::vec3(10.0f,  10.0f, 10.0f),
		glm::vec3(-10.0f, -10.0f, 10.0f),
		glm::vec3(10.0f, -10.0f, 10.0f),
	};
	glm::vec3 lightColors[] = {
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f)
	};*/
	std::cout << "Preprocesses are done\n\n";

	std::cout << "Main Loop---------------------------------------\n";
	for (auto filePath : modelPaths) {
		Model loadedModel = Model(filePath);
		//Create model's picture directory
		size_t found = filePath.find_last_of("\\");
		string fileName = filePath.substr(found - 2);
		cout << savePath + fileName << endl;
		if (!fs::exists(savePath + fileName)) {
			char buff[256];
			sprintf(buff, "mkdir %s%s", savePath.c_str(), fileName.c_str());
			system(buff);
		}

		std::cout << "Cature for " + fileName.substr(3) + "\n";

		//Calculate min-max bounding box
		vector<glm::vec3> allVertices = loadedModel.getAllVertices();
		glm::vec3 boxCtr;
		float boxDiagonalLen = boundingBox(allVertices, boxCtr);

		float scalingSize = 1.0 / boxDiagonalLen;
		glm::mat4 scalingMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scalingSize, scalingSize, scalingSize));
		glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-boxCtr.x, -boxCtr.y, -boxCtr.z));
		glm::mat4 normalizeMatrix = scalingMatrix * translateMatrix;

		unsigned int bBoxIndices[] = {
			0, 2, 3, 0, 3, 1,
			7, 6, 2, 7, 2, 3,
			4, 6, 2, 4, 2, 0,
			4, 6, 7, 4, 7, 5,
			5, 7, 3, 5, 3, 1,
			5, 4, 0, 5, 0, 1
		};

		unsigned int bBoxVBO, bBoxEBO;
		glGenVertexArrays(1, &bBoxVAO);
		glGenBuffers(1, &bBoxVBO);
		glGenBuffers(1, &bBoxEBO);
		glBindVertexArray(bBoxVAO);
		glBindBuffer(GL_ARRAY_BUFFER, bBoxVBO);
		glBufferData(GL_ARRAY_BUFFER, bBoxCoords.size() * sizeof(glm::vec3), &bBoxCoords[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bBoxEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(bBoxIndices), &bBoxIndices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);
	
		//delta time
		//float currentFrame = glfwGetTime();
		//deltaTime = currentFrame - lastFrame;
		//lastFrame = currentFrame;

		//rendering part

		//mat------------
		glm::mat4 model = normalizeMatrix;

		
		//Draw skybox----------------------
		/*glDepthFunc(GL_LEQUAL);
		glUseProgram(skyboxProgram);
		glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

		//Draw cube
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthFunc(GL_LESS);
		//--------skybox----------------------*/

		glUseProgram(renderProgram);
		glUniform3fv(glGetUniformLocation(renderProgram, "camPos"), 1, glm::value_ptr(cameraPos));
		glUniform3fv(glGetUniformLocation(renderProgram, "camView"), 1, glm::value_ptr(cameraFront));
		glUniformMatrix3fv(glGetUniformLocation(renderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(glm::mat3(model)))));
		
		glUniform1i(glGetUniformLocation(renderProgram, ("irradianceMap")), 0);
		glUniform1i(glGetUniformLocation(renderProgram, ("prefilterMap")), 1);
		glUniform1i(glGetUniformLocation(renderProgram, ("brdfLUT")), 2);

		//light sources
		/*//glUniform1i(glGetUniformLocation(renderProgram, "lightNum"), sizeof(lightPositions) / sizeof(lightPositions[0]));
		for (int i = 0; i < sizeof(lightPositions) / sizeof(lightPositions[0]); i++) {
			model = glm::mat4(1.0f);
			model = glm::translate(model, lightPositions[i]);
			model = glm::scale(model, glm::vec3(0.3f));

			glUseProgram(lightShader);
			glUniformMatrix4fv(glGetUniformLocation(lightShader, "mvp"), 1, GL_FALSE, glm::value_ptr(projection * view * model));
			glUniform3fv(glGetUniformLocation(lightShader, "lightColor"), 1, glm::value_ptr(lightColors[i]));
			glBindVertexArray(sphereVAO);
			glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);

			glUseProgram(renderProgram);
			//glm::vec3 newPos = glm::vec3(cos(currentFrame * 3) * 10, 0.0, sin(currentFrame * 3) * 10);
			glUniform3fv(glGetUniformLocation(renderProgram, ("lightPositions[" + to_string(i) + "]").c_str()), 1, glm::value_ptr(lightPositions[i]));;
			glUniform3fv(glGetUniformLocation(renderProgram, ("lightColors[" + to_string(i) + "]").c_str()), 1, glm::value_ptr(lightColors[i]));

		}*/
		for (int i = 0; i < processedTextures.size() / 3; i++) {
			glUseProgram(renderProgram);
			size_t found = hdrPaths[i].find_last_of("\\");
			string hdrName = hdrPaths[i].substr(found + 1);

			std::cout << " Capture with background image " + hdrName + "\n";
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, processedTextures[3 * i]);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_CUBE_MAP, processedTextures[3 * i + 1]);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, processedTextures[3 * i + 2]);

			//Capture--------------------------
			std::cout << " Capturing images...\n";
			glUniform1i(glGetUniformLocation(renderProgram, "renderMode"), 0);
			string imgName = fileName + "\\" + "HDR" + to_string(i + 1) + "_" + renderModes[0] + "_";
			rotateCapture(loadedModel, renderProgram, imgName, model);
			std::cout << " Capturing done\n";
		}
		std::cout << " Capturing with source textures...\n";
		for (int i = 1; i < sizeof(renderModes) / sizeof(renderModes[0]); i++) {
			glUseProgram(renderProgram);
			glUniform1i(glGetUniformLocation(renderProgram, "renderMode"), i);
			string imgName = fileName + "\\" + renderModes[i] + "_";
			if (i == 6) {
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			}
			rotateCapture(loadedModel, renderProgram, imgName, model);
		}
		std::cout << " Capturing done\n\n";
		glClearColor(clearColor, clearColor, clearColor, 1.0f);

		glDeleteBuffers(1, &bBoxEBO);
		glDeleteBuffers(1, &bBoxVBO);
		glDeleteVertexArrays(1, &bBoxVAO);

		loadedModel.deleteModel();
	
		glfwSwapBuffers(window);
		glfwPollEvents();
		
	}
	glDeleteProgram(equiProgram);
	glDeleteProgram(irradianceShader);
	glDeleteProgram(prefilterShader);
	glDeleteProgram(brdfShader);
	glDeleteProgram(skyboxProgram);
	glDeleteProgram(quadShader);
	glDeleteProgram(renderProgram);
	for (int i = 0; i < hdrPaths.size(); i++) {
		glDeleteTextures(1, &processedTextures[i]);
	}
	glDeleteVertexArrays(1, &skyboxVAO);
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteRenderbuffers(1, &captureRBO);
	glDeleteFramebuffers(1, &captureFBO);
	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

GLuint loadTexture(char const* texPath) {
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	//load texture image
	int width, height, nrChannels;
	unsigned char* data = stbi_load(texPath, &width, &height, &nrChannels, 0);
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
		return -1;
	}
	//setting wrapping, filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(data);  //free memory

	return texture;
}
GLuint loadHDR(char const* texPath) {
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrComponents;
	float* data = stbi_loadf(texPath, &width, &height, &nrComponents, 0);
	unsigned int hdrTexture;
	if (data)
	{
		glGenTextures(1, &hdrTexture);
		glBindTexture(GL_TEXTURE_2D, hdrTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Failed to load HDR image." << std::endl;
		return -1;
	}
	stbi_set_flip_vertically_on_load(false);
	return hdrTexture;
}
vector<string> LoadFileList(string root) {
	vector<string> res;
	for (const auto& entry : fs::directory_iterator(root)) {
		auto cur = entry.path().native();
		string path(cur.begin(), cur.end());
		if (is_directory(entry.path())) {
			auto tmp = LoadFileList(path);
			for (auto i : tmp)
				res.push_back(i);
		}
		else
			res.push_back(path);
	}
	return res;
}

GLuint envFormEqui(GLuint equiProgram, GLuint equiTexture) {
	glUseProgram(equiProgram);
	glUniform1i(glGetUniformLocation(equiProgram, ("equirectangularMap")), 0);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, ENV_RESOLUTION, ENV_RESOLUTION);

	unsigned int envCubemap;
	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		// note that we store each face with 16 bit floating point values
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
			ENV_RESOLUTION, ENV_RESOLUTION, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// convert HDR equirectangular environment map to cubemap equivalent
	glUniformMatrix4fv(glGetUniformLocation(equiProgram, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, equiTexture);

	glViewport(0, 0, 1024, 1024); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i) {
		glUniformMatrix4fv(glGetUniformLocation(equiProgram, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//renderCube(); // renders a 1x1 cube
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, equiTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	return envCubemap;
}
GLuint irradFromEnv(GLuint irradianceShader, GLuint envCubemap) {
	unsigned int irradianceMap;
	glGenTextures(1, &irradianceMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, irradResolution, irradResolution, 0,
			GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//reuse capture framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, irradResolution, irradResolution);

	glUseProgram(irradianceShader);
	glUniform1i(glGetUniformLocation(irradianceShader, ("environmentMap")), 0);
	glUniformMatrix4fv(glGetUniformLocation(irradianceShader, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glViewport(0, 0, irradResolution, irradResolution); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i) {		
		glUniformMatrix4fv(glGetUniformLocation(irradianceShader, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);	
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//renderCube();
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return irradianceMap;
}
GLuint prefiltFromEnv(GLuint prefilterShader, GLuint envCubemap) {
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	unsigned int prefilterMap;
	glGenTextures(1, &prefilterMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, prefResolution, prefResolution, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glUseProgram(prefilterShader);
	glUniform1i(glGetUniformLocation(prefilterShader, ("environmentMap")), 0);
	glUniformMatrix4fv(glGetUniformLocation(prefilterShader, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
	{
		// resize framebuffer according to mip-level size.
		unsigned int mipWidth = prefResolution * std::pow(0.5, mip);
		unsigned int mipHeight = prefResolution * std::pow(0.5, mip);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		glUniform1f(glGetUniformLocation(prefilterShader, ("roughness")), roughness);
		for (unsigned int i = 0; i < 6; ++i) {			
			glUniformMatrix4fv(glGetUniformLocation(prefilterShader, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);
			
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//renderCube();
			glBindVertexArray(skyboxVAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return prefilterMap;
}
GLuint brdfFromEnv(GLuint brdfShader) {
	unsigned int brdfLUTTexture;
	glGenTextures(1, &brdfLUTTexture);

	// pre-allocate enough memory for the LUT texture.
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

	glViewport(0, 0, 512, 512);
	glUseProgram(brdfShader);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//RenderQuad();
	glBindVertexArray(quadVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return brdfLUTTexture;
}

void rotateCapture(Model loadedModel, GLuint renderProgram, string fileName, glm::mat4 model) {
	for (float th = 0; th <= 180; th += 45) {
		for (float pi = 0; pi <= 315; pi += 45) {
			glUseProgram(renderProgram);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glm::mat4 view = glm::mat4(1.0f);
			glm::mat4 projection = glm::perspective(glm::radians(fov), float(CAPTURE_WIDTH) / CAPTURE_HEIGHT, 0.1f, 100.0f);
			cameraPos = glm::vec3(capRad * cos(glm::radians(th - 90)) * cos(glm::radians(pi)), capRad * sin(glm::radians(th - 90)), capRad * cos(glm::radians(th - 90)) * sin(glm::radians(pi)));
			cameraFront = glm::vec3(0,0,0) - cameraPos;
			view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
			glUniformMatrix4fv(glGetUniformLocation(renderProgram, "mvp"), 1, GL_FALSE, glm::value_ptr(projection * view * model));
			glUniformMatrix4fv(glGetUniformLocation(renderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
			glUniform3fv(glGetUniformLocation(renderProgram, "camPos"), 1, glm::value_ptr(cameraPos));
			glUniform3fv(glGetUniformLocation(renderProgram, "camView"), 1, glm::value_ptr(cameraFront));

			loadedModel.Draw(renderProgram);

			glUseProgram(lightShader);
			glUniformMatrix4fv(glGetUniformLocation(lightShader, "mvp"), 1, GL_FALSE, glm::value_ptr(projection * view * model));
			glUniform3fv(glGetUniformLocation(lightShader, "lightColor"), 1, glm::value_ptr(glm::vec3(1.0, 1.0, 0.0)));
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glBindVertexArray(bBoxVAO);
			//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			captureImage(fileName + to_string((int)th) + "_" + to_string((int)pi) + ".png");
		}
	}
}
void captureImage(string fileName) {
	int bitsNum;
	GLubyte* bits; //RGB bits
	GLint imageInfo[4]; //current viewport

	//get current viewport
	glGetIntegerv(GL_VIEWPORT, imageInfo); // 이미지 크기 알아내기

	int rows = imageInfo[3];
	int cols = imageInfo[2];

	bitsNum = 3 * cols * rows;
	bits = new GLubyte[bitsNum]; // opengl에서 읽어오는 비트

	//read pixel from frame buffer
	//glFinish(); //finish all commands of OpenGL

	glPixelStorei(GL_PACK_ALIGNMENT, 1); //or glPixelStorei(GL_PACK_ALIGNMENT,4);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
	glReadPixels(0, 0, cols, rows, GL_BGR_EXT, GL_UNSIGNED_BYTE, bits);

	Mat outputImage(rows, cols, CV_8UC3);
	int currentIdx;

	for (int i = 0; i < outputImage.rows; i++)
	{
		for (int j = 0; j < outputImage.cols; j++)
		{
			// stores image from top to bottom, left to right
			currentIdx = (rows - i - 1) * 3 * cols + j * 3; // +0

			outputImage.at<Vec3b>(i, j)[0] = (uchar)(bits[currentIdx]);
			outputImage.at<Vec3b>(i, j)[1] = (uchar)(bits[++currentIdx]); // +1
			outputImage.at<Vec3b>(i, j)[2] = (uchar)(bits[++currentIdx]); // +2
		}
	}
	string filePath = savePath + fileName;

	imwrite(filePath, outputImage);
	delete[] bits;
	outputImage.release();
}
void captureTextureImage(GLuint texture, string fileName) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, texture);
	int w, h;
	int miplevel = 0;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_HEIGHT, &h);
	int bitsNum = 3 * h * w;
	GLubyte* bits = new GLubyte[bitsNum]; // opengl에서 읽어오는 비트
	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, bits);
	Mat outputImage(h, w, CV_8UC3);
	int currentIdx;
	for (int i = 0; i < outputImage.rows; i++)
	{
		for (int j = 0; j < outputImage.cols; j++)
		{
			// stores image from top to bottom, left to right
			currentIdx = (h - i - 1) * 3 * w + j * 3; // +0

			outputImage.at<Vec3b>(i, j)[0] = (uchar)(bits[currentIdx]);
			outputImage.at<Vec3b>(i, j)[1] = (uchar)(bits[++currentIdx]); // +1
			outputImage.at<Vec3b>(i, j)[2] = (uchar)(bits[++currentIdx]); // +2
		}
	}
	string filePath = savePath + fileName;

	imwrite(filePath, outputImage);
	delete[] bits;
	outputImage.release();
	
}
void captureCubeTextureImage(GLuint texture, string fileName) {
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	int mipMaxLevel = 0;
	if (fileName.substr(fileName.find_last_of("_") + 1) == "pref") {
		mipMaxLevel = 4;
	}
		
	for (int j = 0; j <= mipMaxLevel; j++) {
		for (unsigned int i = 0; i < 6; ++i)
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			int w, h;
			glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, j, GL_TEXTURE_WIDTH, &w);
			glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, j, GL_TEXTURE_HEIGHT, &h);
			int bitsNum = 3 * h * w;
			GLubyte* bits = new GLubyte[bitsNum]; // opengl에서 읽어오는 비트
			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, j, GL_BGR_EXT, GL_UNSIGNED_BYTE, bits);
			Mat outputImage(h, w, CV_8UC3);
			int currentIdx;
			for (int k = 0; k < outputImage.rows; k++)
			{
				for (int l = 0; l < outputImage.cols; l++)
				{
					// stores image from top to bottom, left to right
					currentIdx = (h - k - 1) * 3 * w + l * 3; // +0

					outputImage.at<Vec3b>(k, l)[0] = (uchar)(bits[currentIdx]);
					outputImage.at<Vec3b>(k, l)[1] = (uchar)(bits[++currentIdx]); // +1
					outputImage.at<Vec3b>(k, l)[2] = (uchar)(bits[++currentIdx]); // +2
				}
			}
			string filePath = savePath + fileName + to_string(j) + to_string(i + 1) + ".png";
			if (mipMaxLevel != 4)
				filePath = savePath + fileName + to_string(i + 1) + ".png";

			imwrite(filePath, outputImage);
			delete[] bits;
			outputImage.release();
		}
	}
}

double boundingBox(vector<glm::vec3> vertices, glm::vec3& boxCtr)
{
	float maxCoordX = std::numeric_limits<float>::min();
	float maxCoordY = std::numeric_limits<float>::min();
	float maxCoordZ = std::numeric_limits<float>::min();

	float minCoordX = std::numeric_limits<float>::max();
	float minCoordY = std::numeric_limits<float>::max();
	float minCoordZ = std::numeric_limits<float>::max();

	int num = 0;

	// 8 coordinates of bounding box vertices
	for (int i = 0; i < vertices.size(); i++)
	{
		glm::vec3 temp = vertices[i];

		if (maxCoordX < temp.x)
			maxCoordX = temp.x;

		if (minCoordX > temp.x)
			minCoordX = temp.x;

		if (maxCoordY < temp.y)
			maxCoordY = temp.y;

		if (minCoordY > temp.y)
			minCoordY = temp.y;

		if (maxCoordZ < temp.z)
			maxCoordZ = temp.z;

		if (minCoordZ > temp.z)
			minCoordZ = temp.z;
	}

	vector<glm::vec3> tempVec;
	tempVec.push_back(glm::vec3(minCoordX, minCoordY, minCoordZ));
	tempVec.push_back(glm::vec3(minCoordX, minCoordY, maxCoordZ));
	tempVec.push_back(glm::vec3(minCoordX, maxCoordY, minCoordZ));
	tempVec.push_back(glm::vec3(minCoordX, maxCoordY, maxCoordZ));
	tempVec.push_back(glm::vec3(maxCoordX, minCoordY, minCoordZ));
	tempVec.push_back(glm::vec3(maxCoordX, minCoordY, maxCoordZ));
	tempVec.push_back(glm::vec3(maxCoordX, maxCoordY, minCoordZ));
	tempVec.push_back(glm::vec3(maxCoordX, maxCoordY, maxCoordZ));
	bBoxCoords = tempVec;

	// Center of bounding box
	boxCtr.x = (maxCoordX + minCoordX) * 0.5f;
	boxCtr.y = (maxCoordY + minCoordY) * 0.5f;
	boxCtr.z = (maxCoordZ + minCoordZ) * 0.5f;

	// diagonal length
	float maxDist = sqrt(pow(maxCoordX - boxCtr.x, 2) + pow(maxCoordY - boxCtr.y, 2) + pow(maxCoordZ - boxCtr.z, 2));

	return maxDist;
}
GLuint cubeFromPng(int texNum, string type) {
	unsigned int cubeMap;
	glGenTextures(1, &cubeMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	int miplevel = 0;
	if (type == "pref") {
		miplevel = 4;
		//glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}
		
	for (int j = 0; j <= miplevel; j++) {
		for (unsigned int i = 0; i < 6; ++i)
		{
			string path = textureLoc + "\\hdr" + to_string(texNum + 1) + "_" + type + to_string(j) + to_string(i + 1) + ".png";
			if(type != "pref")
				path = textureLoc + "\\hdr" + to_string(texNum + 1) + "_" + type + to_string(i + 1) + ".png";
			int width, height, nrChannels;
			unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
			if (data)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, j, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			else {
				std::cout << "Failed to load" + type + "texture" << std::endl;
				return -1;
			}
			if (type == "pref" && i == 0) {
				//glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			}
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	if(type == "pref")
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	else
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (type == "pref") {
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}

	return cubeMap;
}