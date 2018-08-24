#include<iostream>
#include<cstdio>

#define GLEW_STATIC
#include<GLEW/glew.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include<glm/gtx/transform.hpp>

#include<assimp/cimport.h>
#include<assimp/scene.h>
#include<assimp/postprocess.h>

#include<AntTweakBar.h>

#include"bmpfuncs.h"//for processing bmp images

#include"shader.h"
#include"Camera.h"


#define PI 3.14159265
#define radius 2.5
#define numberOfSides 40
#define numberOfCircleVertices numberOfSides

#define MOVEMENT_SENSITIVITY 0.1f
#define ROTATION_SENSITIVITY 0.01f
/**struct*********************/
struct circleVertex
{
	float position[3];
	float color[3];
};

struct sphereVertex
{
	float position[3];
	float texCoord[2];
};

struct mesh
{
	sphereVertex* meshVerticesPtr;
	int numberOfVertices;
	int* meshIndicesPtr;
	int numberOfFaces;

	mesh()
	{
		meshVerticesPtr = NULL;
		numberOfVertices = 0;

		meshIndicesPtr = NULL;
		numberOfFaces = 0;
	}

	mesh& operator=(const mesh& obj)
	{
		numberOfVertices = obj.numberOfVertices;
		numberOfFaces = obj.numberOfFaces;

		meshVerticesPtr = new sphereVertex[numberOfVertices];
		for (int i = 0; i < numberOfVertices; i++)
		{
			meshVerticesPtr[i] = obj.meshVerticesPtr[i];
		}

		meshIndicesPtr = new int[numberOfFaces * 3];
		for (int i = 0; i < numberOfFaces; i++)
		{
			meshIndicesPtr[i] = obj.meshIndicesPtr[i];
		}

		return *this;
	}
	
	~mesh()
	{
		delete meshVerticesPtr;
		delete meshIndicesPtr;
	}
};
/*****************************/



/**global variables***********/
const int WIDTH = 1500;
const int HEIGHT = 900;

circleVertex orbitPath[numberOfCircleVertices];


const int numOfBuffers = 4;
unsigned int VAO[numOfBuffers];//VAO[0] for orbit paths, VAO[1] for planets, VAO[2] for saturnRing, VAO[3] for asteroids
unsigned int VBO[numOfBuffers];

const int numberOfIBOs = numOfBuffers-1;
unsigned int IBO[numberOfIBOs];//IBO[0] for planets



const int numOfShaderPrograms = 2;
unsigned int shaderProgramID[numOfShaderPrograms];
unsigned int MVP_Index[numOfShaderPrograms];

unsigned int alphaIndex;
/////////////////////////////////////////////
//model matrices
const int numOfOrbitPaths = 9;
glm::mat4 orbitPathModelMatrix[numOfOrbitPaths];

const int numOfPlanets = 10;
glm::mat4 planetModelMatrix[numOfPlanets];
glm::mat4 saturnRingModelMatrix;

const int numOfAsteroids = 120;
glm::mat4 asteroidModelMatrix[numOfAsteroids];

/////////////////////////////////////////////
//textures
unsigned int texSamplerIndex[numOfShaderPrograms];


const int numOfplanetTextures = 10;
unsigned char* planetTexImage[numOfplanetTextures];
unsigned int planetTexID[numOfplanetTextures];

unsigned char* saturnRingTexImage;
unsigned int saturnRingTexID;

const int numOfAsteroidTextures = 3;
unsigned char* asteroidTexImage[numOfAsteroidTextures];
unsigned int asteroidTexID[numOfAsteroidTextures];

/////////////////////////////////////////////
Camera camera;
glm::mat4 topViewMatrix, topProjectionMatrix;

mesh sphere, saturnRing, asteroid;
//////////////////////////////////
//booleans
bool rightMouseButtonHeld = false;
bool enableWireframe = false;
bool animate = false;
bool topView = false;
/*****************************/



void drawCircle(float x, float y, float z)
{
	float doublePI = 2.0f*PI;

	for (int i = 0; i < numberOfCircleVertices; i++)
	{
		orbitPath[i].position[0] = x + (radius*cos(doublePI*i / numberOfSides));
		orbitPath[i].position[1] = y;
		orbitPath[i].position[2] = z + (radius*sin(doublePI*i / numberOfSides));

		orbitPath[i].color[0] = 0.8f;
		orbitPath[i].color[1] = 0.8f;
		orbitPath[i].color[2] = 0.8f;
	}
}

bool loadMesh(const char* fileName, mesh* obj)
{
	const aiScene* scene = aiImportFile(fileName, aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices);

	if (!scene)
	{
		std::cout << "could not load .obj file" << std::endl;
		return false;
	}

	const aiMesh* meshPtr = scene->mMeshes[0];
	obj->numberOfVertices = meshPtr->mNumVertices;

	if (meshPtr->HasPositions())
	{
		obj->meshVerticesPtr = new sphereVertex[meshPtr->mNumVertices];
		 
		for (int i = 0; i < meshPtr->mNumVertices; i++)
		{
			const aiVector3D* vertexPtr = &(meshPtr->mVertices[i]);

			obj->meshVerticesPtr[i].position[0] = (float)vertexPtr->x;
			obj->meshVerticesPtr[i].position[1] = (float)vertexPtr->y;
			obj->meshVerticesPtr[i].position[2] = (float)vertexPtr->z;
		}
	}

	if (meshPtr->HasTextureCoords(0))
	{
		for (int i = 0; i < meshPtr->mNumVertices; i++)
		{
			obj->meshVerticesPtr[i].texCoord[0] = (float)meshPtr->mTextureCoords[0][i].x;
			obj->meshVerticesPtr[i].texCoord[1] = (float)meshPtr->mTextureCoords[0][i].y;
		}
	}

	if (meshPtr->HasFaces())
	{
		obj->numberOfFaces = meshPtr->mNumFaces;
		obj->meshIndicesPtr = new int[meshPtr->mNumFaces * 3];

		for (int i = 0; i < meshPtr->mNumFaces; i++)
		{
			const aiFace* facePtr = &(meshPtr->mFaces[i]);

			obj->meshIndicesPtr[i * 3] = (int)facePtr->mIndices[0];
			obj->meshIndicesPtr[i * 3 + 1] = (int)facePtr->mIndices[1];
			obj->meshIndicesPtr[i * 3 + 2] = (int)facePtr->mIndices[2];
		}
	}

	aiReleaseImport(scene);

	return true;
}



/**initialization functions***********************/
void initOrbitPath()
{
	drawCircle(0.0f, 0.0f, 0.0f);

	unsigned int positionIndex = glGetAttribLocation(shaderProgramID[0], "aPosition");
	unsigned int colorIndex = glGetAttribLocation(shaderProgramID[0], "aColor");

	//passing data to buffers
	glBindVertexArray(VAO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(orbitPath), orbitPath, GL_STATIC_DRAW);

	glVertexAttribPointer(positionIndex, 3, GL_FLOAT, false, sizeof(circleVertex), (void*)(offsetof(circleVertex, position)));
	glVertexAttribPointer(colorIndex, 3, GL_FLOAT, false, sizeof(circleVertex), (void*)(offsetof(circleVertex, color)));

	glEnableVertexAttribArray(positionIndex);
	glEnableVertexAttribArray(colorIndex);
	/////////////////////////////////////////////
	//initialize orbit path model matrices
	for (int i = 0; i < numOfOrbitPaths; i++)
	{
		orbitPathModelMatrix[i] = glm::mat4(1.0f);
	}

	float planetOrbit[] = {1.4f, 1.5f, 1.64f, 1.9f, 3.5f, 5.1f, 9.0f, 13.5f, 23.0f};
	for (int i = 0; i < numOfOrbitPaths; i++)
	{
		orbitPathModelMatrix[i] = glm::scale(glm::vec3(planetOrbit[i], planetOrbit[i], planetOrbit[i]));
	}
	
}

void initPlanets()
{
	loadMesh("assets/models/sphere.obj", &sphere);

	unsigned int positionIndex = glGetAttribLocation(shaderProgramID[1], "aPosition");
	unsigned int texCoordIndex = glGetAttribLocation(shaderProgramID[1], "aTexCoord");

	//passing data to buffers
	glBindVertexArray(VAO[1]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sphereVertex)*sphere.numberOfVertices, sphere.meshVerticesPtr, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*sphere.numberOfFaces * 3, sphere.meshIndicesPtr, GL_STATIC_DRAW);

	glVertexAttribPointer(positionIndex, 3, GL_FLOAT, false, sizeof(sphereVertex), (void*)(offsetof(sphereVertex, position)));
	glVertexAttribPointer(texCoordIndex, 2, GL_FLOAT, false, sizeof(sphereVertex), (void*)(offsetof(sphereVertex, texCoord)));

	glEnableVertexAttribArray(positionIndex);
	glEnableVertexAttribArray(texCoordIndex);
	///////////////////////////////////////////////////////////
	//initialize planets textures
	glGenTextures(numOfplanetTextures, planetTexID);

	int imageWidth[numOfplanetTextures];
	int imageHeight[numOfplanetTextures];

	const char* const textures[] = { "assets/textures/sun.bmp", "assets/textures/mercury.bmp",
		"assets/textures/venus.bmp", "assets/textures/earth.bmp", "assets/textures/mars.bmp",
		"assets/textures/jupiter.bmp", "assets/textures/saturn.bmp", "assets/textures/uranus.bmp",
		"assets/textures/neptune.bmp", "assets/textures/pluto.bmp"  
	};
	
	for (int i = 0; i < numOfplanetTextures; i++)
	{
		planetTexImage[i] = readBitmapRGBImage(textures[i], &imageWidth[i], &imageHeight[i]);

		glBindTexture(GL_TEXTURE_2D, planetTexID[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth[i], imageHeight[i], 0, GL_BGR, GL_UNSIGNED_BYTE, planetTexImage[i]);

		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	///////////////////////////////////////////////////////////
	//initialize planets model matrices
	for (int i = 0; i < numOfPlanets; i++)
	{
		planetModelMatrix[i] = glm::mat4(1.0f);
	}

	planetModelMatrix[0] = glm::scale(glm::vec3(3.0f, 3.0f, 3.0f));//Sun
	
}

void initSaturnRing()
{
	loadMesh("assets/models/plane.obj", &saturnRing);

	unsigned int positionIndex = glGetAttribLocation(shaderProgramID[1], "aPosition");
	unsigned int texCoordIndex = glGetAttribLocation(shaderProgramID[1], "aTexCoord");

	glBindVertexArray(VAO[2]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sphereVertex)*saturnRing.numberOfVertices, saturnRing.meshVerticesPtr, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*saturnRing.numberOfFaces * 3, saturnRing.meshIndicesPtr, GL_STATIC_DRAW);

	glVertexAttribPointer(positionIndex, 3, GL_FLOAT, false, sizeof(sphereVertex), (void*)(offsetof(sphereVertex, position)));
	glVertexAttribPointer(texCoordIndex, 2, GL_FLOAT, false, sizeof(sphereVertex), (void*)(offsetof(sphereVertex, texCoord)));

	glEnableVertexAttribArray(positionIndex);
	glEnableVertexAttribArray(texCoordIndex);
	///////////////////////////////////////////////////////////
	glGenTextures(1, &saturnRingTexID);

	int imageWidth, imageHeight;

	saturnRingTexImage = readBitmapRGBImage("assets/textures/saturnRing.bmp", &imageWidth, &imageHeight);
	glBindTexture(GL_TEXTURE_2D, saturnRingTexID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, saturnRingTexImage);

	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	
	///////////////////////////////////////////////////////////
	saturnRingModelMatrix = glm::mat4(1.0f);
}

void initAsteroid()
{
	loadMesh("assets/models/rock01.obj", &asteroid);

	unsigned int positionIndex = glGetAttribLocation(shaderProgramID[1], "aPosition");
	unsigned int texCoordIndex = glGetAttribLocation(shaderProgramID[1], "aTexCoord");

	//passing data to buffers
	glBindVertexArray(VAO[3]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[3]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sphereVertex)*asteroid.numberOfVertices, asteroid.meshVerticesPtr, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*asteroid.numberOfFaces * 3, asteroid.meshIndicesPtr, GL_STATIC_DRAW);

	glVertexAttribPointer(positionIndex, 3, GL_FLOAT, false, sizeof(sphereVertex), (void*)(offsetof(sphereVertex, position)));
	glVertexAttribPointer(texCoordIndex, 2, GL_FLOAT, false, sizeof(sphereVertex), (void*)(offsetof(sphereVertex, texCoord)));

	glEnableVertexAttribArray(positionIndex);
	glEnableVertexAttribArray(texCoordIndex);
	///////////////////////////////////////////////////////////
	//initialize asteroids textures
	glGenTextures(numOfAsteroidTextures, asteroidTexID);

	int imageWidth[numOfAsteroidTextures];
	int imageHeight[numOfAsteroidTextures];

	for (int i = 0; i < numOfAsteroidTextures; i++)
	{
		string number = to_string(i);

		asteroidTexImage[i] = readBitmapRGBImage(("assets/textures/asteroid" + number + ".bmp").c_str(), &imageWidth[i], &imageHeight[i]);
		glBindTexture(GL_TEXTURE_2D, asteroidTexID[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth[i], imageHeight[i], 0, GL_BGR, GL_UNSIGNED_BYTE, asteroidTexImage[i]);

		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	///////////////////////////////////////////////////////////
	//initialize asteroids model matrices
	for (int i = 0; i < numOfAsteroids; i++)
	{
		asteroidModelMatrix[i] = glm::mat4(1.0f);
	}

}





static void init(GLFWwindow* window)
{
	glEnable(GL_DEPTH_TEST);

	//shaderProgram for orbits
	shaderProgramID[0] = loadShaders("circleVertexShader.vert", "circleFrag.frag");
	MVP_Index[0] = glGetUniformLocation(shaderProgramID[0], "uModelViewProjectionMatrix");

	//shaderProgram for planets & asteroids
	shaderProgramID[1] = loadShaders("textureVS.vert", "textureFS.frag");
	MVP_Index[1] = glGetUniformLocation(shaderProgramID[1], "uModelViewProjectionMatrix");


	texSamplerIndex[0] = glGetUniformLocation(shaderProgramID[1], "uTextureSampler");

	alphaIndex = glGetUniformLocation(shaderProgramID[1], "uAlpha");
	//////////////////////////////////////////////////////////
	//generate buffers
	glGenBuffers(numOfBuffers, VBO);
	glGenBuffers(numberOfIBOs, IBO);
	glGenVertexArrays(numOfBuffers, VAO);
	/////////////////////////////////////////////////////////
	//call perspective initialization function to initialize component
	initOrbitPath();
	initPlanets();
	initSaturnRing();
	initAsteroid();
	//////////////////////////////////////////////
	//perspective view 
	camera.setViewMatrix(glm::vec3(-2, 4, 20), glm::vec3(10, -2, 1), glm::vec3(0, 1, 0));

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	float aspectRatio = (float)width / height;
	camera.setProjection(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);


	//top view
	//right-left = aspectRatio*(top-buttom)
	float left, right, bottom, top;
	left = -58.0f;
	bottom = -35.0f;
	top = 35.0f;
	right = aspectRatio * (top - bottom) + left;

	topProjectionMatrix = glm::ortho(left, right, bottom, top, 0.1f, 100.0f);//top
	topViewMatrix = glm::lookAt(glm::vec3(-1, 3, 0), glm::vec3(-1, -1, 0), glm::vec3(0, 0, -1));//top

}
/*************************************************/



/**draw calls***********************/
static void drawOrbitPath(glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
	glUseProgram(shaderProgramID[0]);
	glBindVertexArray(VAO[0]);

	glm::mat4 MVP;

	//draw the planet orbit paths
	for (int i = 0; i < numOfOrbitPaths; i++)
	{
		MVP = projectionMatrix * viewMatrix *orbitPathModelMatrix[i];
		glUniformMatrix4fv(MVP_Index[0], 1, false, &MVP[0][0]);
		glDrawArrays(GL_LINE_LOOP, 0, numberOfCircleVertices);
	}
	

}

static void drawPlanets(glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
	glUseProgram(shaderProgramID[1]);

	glBindVertexArray(VAO[1]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO[0]);

	glm::mat4 MVP;
	float alpha = 1.0f;

	for (int i = 0; i < numOfplanetTextures; i++)
	{
		MVP = projectionMatrix * viewMatrix *planetModelMatrix[i];
		glUniformMatrix4fv(MVP_Index[1], 1, false, &MVP[0][0]);

		glUniform1fv(alphaIndex, 1, &alpha);//fully opaque

		//bind to texture
		glUniform1i(texSamplerIndex[0], 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, planetTexID[i]);

		//draw the indices
		glDrawElements(GL_TRIANGLES, sphere.numberOfFaces * 3, GL_UNSIGNED_INT, 0);
	}

	
}

static void drawSaturnRing(glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
	glUseProgram(shaderProgramID[1]);

	glBindVertexArray(VAO[2]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO[1]);

	glm::mat4 MVP;
	float alpha = 1.0f;

	MVP = projectionMatrix * viewMatrix * saturnRingModelMatrix;
	glUniformMatrix4fv(MVP_Index[1], 1, false, &MVP[0][0]);

	glUniform1fv(alphaIndex, 1, &alpha);//fully opaque

	//bind to textures
	glUniform1i(texSamplerIndex[0], 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, saturnRingTexID);

	//draw the indices
	glDrawElements(GL_TRIANGLES, saturnRing.numberOfFaces * 3, GL_UNSIGNED_INT, 0);

}

static void drawAsteroids(glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
	glUseProgram(shaderProgramID[1]);

	glBindVertexArray(VAO[3]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO[2]);

	glm::mat4 MVP;
	float alpha = 1.0f;

	 
	for (int i = 0; i < numOfAsteroids; i++)
	{
		MVP = projectionMatrix * viewMatrix *asteroidModelMatrix[i];
		glUniformMatrix4fv(MVP_Index[1], 1, false, &MVP[0][0]);

		glUniform1fv(alphaIndex, 1, &alpha);

		glUniform1i(texSamplerIndex[0], 0);
		glActiveTexture(GL_TEXTURE0);
		
		//assign asteroids different textures
		if(i < 40)
			glBindTexture(GL_TEXTURE_2D, asteroidTexID[0]);
		if (i >= 40 && i < 80)
			glBindTexture(GL_TEXTURE_2D, asteroidTexID[1]);
		if (i >= 80 && i < numOfAsteroids)
			glBindTexture(GL_TEXTURE_2D, asteroidTexID[2]);

		
		glDrawElements(GL_TRIANGLES, asteroid.numberOfFaces * 3, GL_UNSIGNED_INT, 0);
	}


}




static void renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	
	if (topView)
	{
		drawOrbitPath(topProjectionMatrix, topViewMatrix);
		drawPlanets(topProjectionMatrix, topViewMatrix);
		drawSaturnRing(topProjectionMatrix, topViewMatrix);
		drawAsteroids(topProjectionMatrix, topViewMatrix);
	}
	else //perspective view
	{
		drawOrbitPath(camera.getProjectionMatrix(), camera.getViewMatrix());
		drawPlanets(camera.getProjectionMatrix(), camera.getViewMatrix());
		drawSaturnRing(camera.getProjectionMatrix(), camera.getViewMatrix());
		drawAsteroids(camera.getProjectionMatrix(), camera.getViewMatrix());
	}
	

	glFlush();
}
/*************************************************/


static void animatePlanets()
{
	static float orbitAngle[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	static float orbitSpeed[] = { 1.59f, 1.18f, 1.0f, 0.808f,0.439f,0.325f,0.228f,0.182f, 0.157f };
	float orbitScale = 0.01f;
	//https://nssdc.gsfc.nasa.gov/planetary/factsheet/planet_table_ratio.html

	static float rotationAngle[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	static float rotationSpeed[] = { 10.83f, 6.52f, 1674.0f, 866.0f, 45583.0f, 36840.0f, 14794.0f, 9719.0f, 123.21f };
	float rotationScale = 0.0001f;
	//http://www.grandpapencil.net/projects/plansped.htm

	float saturnRingRotateAngleY = 65.0f;

	if (animate)//if animation mode then change the orbit angle and rotation angle
	{
		for (int i = 0; i < numOfOrbitPaths; i++)
		{
			orbitAngle[i] += orbitSpeed[i] * orbitScale;
			rotationAngle[i] += rotationSpeed[i] * rotationScale;
		}

		saturnRingRotateAngleY += 0.05f;
	}

	float planetScale[] = { 0.008f, 0.086f, 0.1f, 0.015f, 1.6f, 0.8f, 0.3f, 0.5f, 0.01f };
	float planetTranslate[] = { 3.5f, 3.75f, 4.1f, 4.75f, 8.5f, 12.5f, 22.5f, 34.0f, 57.5f };
	//https://www.universetoday.com/37124/volume-of-the-planets/

	for (int i = 1; i < numOfPlanets; i++)
	{
		//animate planets
		planetModelMatrix[i] = glm::rotate(orbitAngle[i-1], glm::vec3(0.0f, 1.0f, 0.0f))*
							   glm::translate(glm::vec3(planetTranslate[i-1], 0.0f, 0.0f))*
							   glm::rotate(rotationAngle[i-1], glm::vec3(0.0f, 1.0f, 0.0f))*
							   glm::scale(glm::vec3(planetScale[i-1], planetScale[i - 1], planetScale[i - 1]));
	}

	
	//animate Saturn ring
	saturnRingModelMatrix = glm::rotate(orbitAngle[5], glm::vec3(0.0f, 1.0f, 0.0f))* glm::translate(glm::vec3(12.5f, 0.0f, 0.0f)) *
		glm::rotate(glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::radians(saturnRingRotateAngleY), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::rotate(glm::radians(18.0f),glm::vec3(0.0f,0.0f,1.0f))*
		glm::scale(glm::vec3(4.7f, 2.5f, 2.8f));

	
}

static void animateAsteroids()
{
	static float orbitAngle = 0.0f;
	static float orbitSpeed = 0.83f;
	float orbitScale = 0.01f;
	

	if (animate)//if animation mode then change orbit angle 
		orbitAngle += orbitSpeed * orbitScale;
	
	

	float asteroidPositionX = -1.0f, asteroidPositionZ = -6.0f;
	float doublePI = 2.0f*PI;
	float asteroidRadius = 1.0f;


	//animate asteroids 
	//asteroid 0-40
	for (int i = 0; i < numOfAsteroids / 3; i++)
	{
		asteroidModelMatrix[i] = glm::rotate(orbitAngle, glm::vec3(0.0f, 1.0f, 0.0f))*
                                 glm::translate(glm::vec3(asteroidPositionX, 0.0f, asteroidPositionZ))*glm::scale(glm::vec3(0.0005f, 0.0005f, 0.0005f));

		asteroidPositionX += (asteroidRadius*cos(doublePI*i / numberOfSides));
		asteroidPositionZ += (asteroidRadius*sin(doublePI*i / numberOfSides));
	}


	//asteroid 40-80
	asteroidPositionX = -0.5f;
	asteroidPositionZ = -6.5f;
	for (int i = numOfAsteroids / 3; i < (numOfAsteroids / 3) * 2; i++)
	{
		asteroidModelMatrix[i] = glm::rotate(orbitAngle, glm::vec3(0.0f, 1.0f, 0.0f))*
			                     glm::translate(glm::vec3(asteroidPositionX, 0.0f, asteroidPositionZ))*glm::scale(glm::vec3(0.0003f, 0.0003f, 0.0003f));

		asteroidPositionX += (asteroidRadius*cos(doublePI*i / numberOfSides));
		asteroidPositionZ += (asteroidRadius*sin(doublePI*i / numberOfSides));
	}
	
	//asteroid 80-120
	asteroidPositionZ = -6.8f;
	for (int i = (numOfAsteroids / 3) * 2; i < numOfAsteroids; i++)
	{
		asteroidModelMatrix[i] = glm::rotate(orbitAngle, glm::vec3(0.0f, 1.0f, 0.0f))*
			                     glm::translate(glm::vec3(asteroidPositionX, 0.0f, asteroidPositionZ))*glm::scale(glm::vec3(0.0006f, 0.0006f, 0.0006f));

		asteroidPositionX += (asteroidRadius*cos(doublePI*i / numberOfSides));
		asteroidPositionZ += (asteroidRadius*sin(doublePI*i / numberOfSides));
	}


}


static void updateScene(GLFWwindow* window)
{
	animatePlanets();
	animateAsteroids();


	float moveForward = 0.0f;
	float strafeRight = 0.0f;

	//W,S,A,D to control movement
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		moveForward += MOVEMENT_SENSITIVITY;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		moveForward -= MOVEMENT_SENSITIVITY;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		strafeRight -= MOVEMENT_SENSITIVITY;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		strafeRight += MOVEMENT_SENSITIVITY;

	camera.update(moveForward, strafeRight);
}



/**callback functions*****************************/
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	//close window when user press escape key
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
		return;
	}

}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	//check if right mouse button is held
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		rightMouseButtonHeld = true;
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
		rightMouseButtonHeld = false;

	TwEventMouseButtonGLFW(button, action);//antweakbar callback
}

static void cursorCallback(GLFWwindow* window, double xpos, double ypos)
{
	static double previousXPos = xpos;
	static double previousYPos = ypos;

	double deltaX = previousXPos - xpos;
	double deltaY = previousYPos - ypos;

	if (rightMouseButtonHeld)
		camera.updateRotation(deltaX*ROTATION_SENSITIVITY, deltaY*ROTATION_SENSITIVITY);

	previousXPos = xpos;
	previousYPos = ypos;


	TwEventMousePosGLFW(xpos, ypos);//antweakbar callback
}
/*************************************************/



static void initATB()
{
	TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(WIDTH, HEIGHT);

	TwDefine("TW_HELP visible=false");
	TwDefine("GLOBAL fontsize=3");

	TwBar* tweakBar;
	tweakBar = TwNewBar("main");

	TwDefine("main label='GUI' refresh=0.02 text=light size='220 180' ");
	//////////////////////////////////////
	TwAddVarRW(tweakBar, "wireframe:", TW_TYPE_BOOLCPP, &enableWireframe, "group = 'control' ");
	TwAddVarRW(tweakBar, "animation:", TW_TYPE_BOOLCPP, &animate, "group = 'control' ");
	TwAddVarRW(tweakBar, "top view:", TW_TYPE_BOOLCPP, &topView, "group = 'control' ");
}


int main()
{
	GLFWwindow* window = NULL;

	//initialize glfw
	if (!glfwInit())
	{
		std::cout << "glfw initialization fail" << std::endl;
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	
	//create window
	window = glfwCreateWindow(WIDTH, HEIGHT, "SOLAR SYSTEM", NULL, NULL);

	if (window == NULL)
	{
		std::cout << "window is null" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);


	//initialize glew
	if (glewInit() != GLEW_OK)
	{
		std::cout << "glew initialization fail" << std::endl;
		exit(EXIT_FAILURE);
	}
	///////////////////////////////////////////////////////////
	//set callbacks
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorCallback);

	init(window);
	initATB();//function to initialize antweak bar

	//main while loop
	while (!glfwWindowShouldClose(window))
	{
		updateScene(window);

		if (enableWireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		renderScene();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		TwDraw();//draw antweak bar

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	//////////////////////////////////////////
	//clean up
	for (int i = 0; i < numOfShaderPrograms; i++)
	{
		glDeleteProgram(shaderProgramID[i]);
	}

	glDeleteBuffers(numOfBuffers, VBO);
	glDeleteBuffers(numberOfIBOs, IBO);
	glDeleteVertexArrays(numOfBuffers, VAO);
	

	for (int i = 0; i < numOfplanetTextures; i++)
	{
		if (planetTexImage[i])
			delete[] planetTexImage[i];
	}
	glDeleteTextures(numOfplanetTextures, planetTexID);


	if (saturnRingTexImage)
		delete[] saturnRingTexImage;
	glDeleteTextures(1, &saturnRingTexID);


	for (int i = 0; i < numOfAsteroidTextures; i++)
	{
		if (asteroidTexImage[i])
			delete[] asteroidTexImage[i];
	}
	glDeleteTextures(numOfAsteroidTextures, asteroidTexID);


	TwTerminate();

	glfwDestroyWindow(window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}
