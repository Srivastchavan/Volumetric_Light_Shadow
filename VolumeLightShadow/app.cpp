#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>'

#include "GLSLShader.h"
#include <fstream>
#include <algorithm>

#include "GridLines.h"
CGrid* grid;
#pragma comment(lib, "glew32.lib")


#define GL_CHECK_ERRORS assert(glGetError()== GL_NO_ERROR);
using namespace std;

GLSLShader shader, shadowShader, flatShader, polyShader;

const float EPSILON = 0.0001f;
const int width = 1280;
const int height = 960;

int camState = 0;
int prevX = 0;
int prevY = 0;
float rotX = 4;
float rotY = 50;
float camDis = -2;

GLuint VBO;
GLuint VAO;

glm::mat4 MV, P;


const int maxSlices = 512;

glm::vec3 textureSlices[maxSlices * 12];

std::string assetpath[3] = {"assets/BostonTeapot256.raw","assets/Engine256.raw","assets/head256.raw"};
std::string filepath = assetpath[2];

static BOOL enableLight = TRUE;
static int toggleAsset = 0;
static BOOL fillPolygon = TRUE;
static BOOL toggleColor = FALSE;
bool rotateView = false;

enum {
	MENU_Lighting = 1,
	MENU_Polymode,
	MENU_Asset,
	MENU_Color,
	MENU_EXIT
};

const int XDIM = 256;
const int YDIM = 256;
const int ZDIM = 256;

int num_slices = 500;

GLuint lightVAO;
GLuint lightVBO;
glm::vec3 lightPos = glm::vec3(0, 2, 0);

float theta = 0.66f;
float phi = -1.0f;
float radius = 2;

GLuint textureID;

glm::vec3 viewDir;
glm::vec3 lightVec;
glm::vec3 halfAngle;

GLuint lightBuffer;
GLuint eyeBuffer;
GLuint colorTexID;
GLuint lightFBO;
GLuint polyVAO;
GLuint polyVBO;
const int downSamp = 2;
const int imgWidth = width / downSamp;
const int imgHeight = height / downSamp;

glm::mat4 lightMV;  //light modelview matrix
glm::mat4 lightP;  //light projection matrix
glm::mat4 lightB;  //light bias matrix
glm::mat4 lightBP;	//light bias and projection combined matrix
glm::mat4 lightShadow;	//light BP matrix combined with MV matrix

bool isViewInverted = false;
glm::vec4 colors[6] = {
	glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),	// Grey
	glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),	// Red
	glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), // Green
	glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), // Blue
	glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), // Yellow
	glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)	// Purple
};

glm::vec4 lightColor = colors[0];	//colour of light
glm::vec3 lightAttenuation = glm::vec3(0.1, 0.2, 0.3);	//colout of light attenuation 

float shadowAlpha = 1;

GLenum attachIDs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

bool LoadVolume();
void SliceVolume();
int FindAbsMax(glm::vec3 v);
void OnShutdown();
void ShutdownFBO();
void DrawSlices(const glm::mat4& MVP);
void OnRender();
void OnResize(int w, int h);
void OnMouseDown(int button, int s, int x, int y);
void OnMouseMove(int x, int y);
void OnMouseWheel(int button, int dir, int x, int y);
void OnKey(unsigned char key, int x, int y);
int BuildPopupMenu(void);
void DrawSliceFromEyePointOfView(const int i);
void DrawSliceFromLightPointOfView(const int i);
void SelectFromMenu(int idCommand);
void subMenuHandler(int choice);
void polyMenu(int choice);
void assetMenu(int choice);

glm::vec3 vertexList[8] = { glm::vec3(-0.5,-0.5,-0.5),
						   glm::vec3(0.5,-0.5,-0.5),
						   glm::vec3(0.5, 0.5,-0.5),
						   glm::vec3(-0.5, 0.5,-0.5),
						   glm::vec3(-0.5,-0.5, 0.5),
						   glm::vec3(0.5,-0.5, 0.5),
						   glm::vec3(0.5, 0.5, 0.5),
						   glm::vec3(-0.5, 0.5, 0.5) };

int edgeList[8][12] = {
	{ 0,1,5,6,   4,8,11,9,  3,7,2,10 }, // v0 is front
	{ 0,4,3,11,  1,2,6,7,   5,9,8,10 }, // v1 is front
	{ 1,5,0,8,   2,3,7,4,   6,10,9,11}, // v2 is front
	{ 7,11,10,8, 2,6,1,9,   3,0,4,5  }, // v3 is front
	{ 8,5,9,1,   11,10,7,6, 4,3,0,2  }, // v4 is front
	{ 9,6,10,2,  8,11,4,7,  5,0,1,3  }, // v5 is front
	{ 9,8,5,4,   6,1,2,0,   10,7,11,3}, // v6 is front
	{ 10,9,6,5,  7,2,3,1,   11,4,8,0 }  // v7 is front
};
const int edges[12][2] = { {0,1},{1,2},{2,3},{3,0},{0,4},{1,5},{2,6},{3,7},{4,5},{5,6},{6,7},{7,4} };



GLuint CreateTexture(const int w, const int h, GLenum internalFormat, GLenum format) {
	GLuint texid;
	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, GL_FLOAT, 0);
	return texid;
}

bool LoadVolume() {
	std::ifstream infile(filepath.c_str(), std::ios_base::binary);

	if (infile.good()) {

		GLubyte* pData = new GLubyte[XDIM * YDIM * ZDIM];
		infile.read(reinterpret_cast<char*>(pData), XDIM* YDIM* ZDIM * sizeof(GLubyte));
		infile.close();

		glGenTextures(1, &textureID);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, textureID);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 4);
	
		glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, XDIM, YDIM, ZDIM, 0, GL_RED, GL_UNSIGNED_BYTE, pData);
		GL_CHECK_ERRORS

			glGenerateMipmap(GL_TEXTURE_3D);

		delete[] pData;

		return true;
	}
	else {
		return false;
	}
}

void SliceVolume() {
	
	float max_dist = glm::dot(halfAngle, vertexList[0]);
	float min_dist = max_dist;
	int max_index = 0;
	int count = 0;

	for (int i = 1; i < 8; i++) {
		
		float dist = glm::dot(halfAngle, vertexList[i]);

		if (dist > max_dist) {
			max_dist = dist;
			max_index = i;
		}

		if (dist < min_dist)
			min_dist = dist;
	}

	int max_dim = FindAbsMax(halfAngle);
	min_dist -= EPSILON;
	max_dist += EPSILON;

	glm::vec3 vecStart[12];
	glm::vec3 vecDir[12];
	float lambda[12];
	float lambda_inc[12];
	float denom = 0;

	float plane_dist = min_dist;
	float plane_dist_inc = (max_dist - min_dist) / float(num_slices);

	for (int i = 0; i < 12; i++) {
		vecStart[i] = vertexList[edges[edgeList[max_index][i]][0]];

		vecDir[i] = vertexList[edges[edgeList[max_index][i]][1]] - vecStart[i];

		denom = glm::dot(vecDir[i], halfAngle);

		if (1.0 + denom != 1.0) {
			lambda_inc[i] = plane_dist_inc / denom;
			lambda[i] = (plane_dist - glm::dot(vecStart[i], halfAngle)) / denom;
		}
		else {
			lambda[i] = -1.0;
			lambda_inc[i] = 0.0;
		}
	}

	glm::vec3 intersection[6];
	float dL[12];

	for (int i = num_slices - 1; i >= 0; i--) {

		for (int e = 0; e < 12; e++)
		{
			dL[e] = lambda[e] + i * lambda_inc[e];
		}

		if ((dL[0] >= 0.0) && (dL[0] < 1.0)) {
			intersection[0] = vecStart[0] + dL[0] * vecDir[0];
		}
		else if ((dL[1] >= 0.0) && (dL[1] < 1.0)) {
			intersection[0] = vecStart[1] + dL[1] * vecDir[1];
		}
		else if ((dL[3] >= 0.0) && (dL[3] < 1.0)) {
			intersection[0] = vecStart[3] + dL[3] * vecDir[3];
		}
		else continue;

		if ((dL[2] >= 0.0) && (dL[2] < 1.0)) {
			intersection[1] = vecStart[2] + dL[2] * vecDir[2];
		}
		else if ((dL[0] >= 0.0) && (dL[0] < 1.0)) {
			intersection[1] = vecStart[0] + dL[0] * vecDir[0];
		}
		else if ((dL[1] >= 0.0) && (dL[1] < 1.0)) {
			intersection[1] = vecStart[1] + dL[1] * vecDir[1];
		}
		else {
			intersection[1] = vecStart[3] + dL[3] * vecDir[3];
		}

		if ((dL[4] >= 0.0) && (dL[4] < 1.0)) {
			intersection[2] = vecStart[4] + dL[4] * vecDir[4];
		}
		else if ((dL[5] >= 0.0) && (dL[5] < 1.0)) {
			intersection[2] = vecStart[5] + dL[5] * vecDir[5];
		}
		else {
			intersection[2] = vecStart[7] + dL[7] * vecDir[7];
		}
		if ((dL[6] >= 0.0) && (dL[6] < 1.0)) {
			intersection[3] = vecStart[6] + dL[6] * vecDir[6];
		}
		else if ((dL[4] >= 0.0) && (dL[4] < 1.0)) {
			intersection[3] = vecStart[4] + dL[4] * vecDir[4];
		}
		else if ((dL[5] >= 0.0) && (dL[5] < 1.0)) {
			intersection[3] = vecStart[5] + dL[5] * vecDir[5];
		}
		else {
			intersection[3] = vecStart[7] + dL[7] * vecDir[7];
		}
		if ((dL[8] >= 0.0) && (dL[8] < 1.0)) {
			intersection[4] = vecStart[8] + dL[8] * vecDir[8];
		}
		else if ((dL[9] >= 0.0) && (dL[9] < 1.0)) {
			intersection[4] = vecStart[9] + dL[9] * vecDir[9];
		}
		else {
			intersection[4] = vecStart[11] + dL[11] * vecDir[11];
		}

		if ((dL[10] >= 0.0) && (dL[10] < 1.0)) {
			intersection[5] = vecStart[10] + dL[10] * vecDir[10];
		}
		else if ((dL[8] >= 0.0) && (dL[8] < 1.0)) {
			intersection[5] = vecStart[8] + dL[8] * vecDir[8];
		}
		else if ((dL[9] >= 0.0) && (dL[9] < 1.0)) {
			intersection[5] = vecStart[9] + dL[9] * vecDir[9];
		}
		else {
			intersection[5] = vecStart[11] + dL[11] * vecDir[11];
		}

		int indices[] = { 0,1,2, 0,2,3, 0,3,4, 0,4,5 };

		for (int i = 0; i < 12; i++)
			textureSlices[count++] = intersection[indices[i]];
	}

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(textureSlices), &(textureSlices[0].x));
}

int FindAbsMax(glm::vec3 v) {
	v = glm::abs(v);
	int max_dim = 0;
	float val = v.x;
	if (v.y > val) {
		val = v.y;
		max_dim = 1;
	}
	if (v.z > val) {
		val = v.z;
		max_dim = 2;
	}
	return max_dim;
}

int main(int argc, char** argv) {
	
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitContextVersion(3, 3);
	glutInitContextFlags(GLUT_CORE_PROFILE | GLUT_DEBUG);
	glutInitWindowSize(width, height);
	glutCreateWindow("Volume lighting using Half Angle Slicing");

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		cerr << "Error: " << glewGetErrorString(err) << endl;
	}
	else {
		if (GLEW_VERSION_3_3)
		{
			cout << "Driver supports OpenGL 3.3\nDetails:" << endl;
		}
	}
	err = glGetError(); 
	GL_CHECK_ERRORS

	cout << "\tUsing GLEW " << glewGetString(GLEW_VERSION) << endl;
	cout << "\tVendor: " << glGetString(GL_VENDOR) << endl;
	cout << "\tRenderer: " << glGetString(GL_RENDERER) << endl;
	cout << "\tVersion: " << glGetString(GL_VERSION) << endl;
	cout << "\tGLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	GL_CHECK_ERRORS

	grid = new CGrid(20, 20);

	GL_CHECK_ERRORS

	
	shader.LoadFromFile(GL_VERTEX_SHADER, "shaders/textureSlicer.vert");
	shader.LoadFromFile(GL_FRAGMENT_SHADER, "shaders/textureSlicer.frag");

	shader.CreateAndLinkProgram();
	shader.Use();
	
	shader.AddAttribute("vVertex");
	shader.AddUniform("MVP");
	shader.AddUniform("color");
	shader.AddUniform("volume");

	glUniform1i(shader("volume"), 0);
	glUniform4f(shader("color"), lightAttenuation.x * shadowAlpha, lightAttenuation.y * shadowAlpha, lightAttenuation.z * shadowAlpha, 1);

	shader.UnUse();

	GL_CHECK_ERRORS

	shadowShader.LoadFromFile(GL_VERTEX_SHADER, "shaders/slicerShadow.vert");
	shadowShader.LoadFromFile(GL_FRAGMENT_SHADER, "shaders/slicerShadow.frag");

	shadowShader.CreateAndLinkProgram();
	shadowShader.Use();
	
	shadowShader.AddAttribute("vVertex");
	shadowShader.AddUniform("MVP");
	shadowShader.AddUniform("S");
	shadowShader.AddUniform("color");
	shadowShader.AddUniform("shadowTex");
	shadowShader.AddUniform("volume");

	glUniform1i(shadowShader("volume"), 0);
	glUniform1i(shadowShader("shadowTex"), 1);
	glUniform4f(shadowShader("color"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
	shadowShader.UnUse();

	flatShader.LoadFromFile(GL_VERTEX_SHADER, "shaders/flatShader.vert");
	flatShader.LoadFromFile(GL_FRAGMENT_SHADER, "shaders/flatShader.frag");

	flatShader.CreateAndLinkProgram();
	flatShader.Use();
	
	flatShader.AddAttribute("vVertex");
	flatShader.AddUniform("MVP");
	flatShader.UnUse();

	GL_CHECK_ERRORS

	
	polyShader.LoadFromFile(GL_VERTEX_SHADER, "shaders/polyShader.vert");
	polyShader.LoadFromFile(GL_FRAGMENT_SHADER, "shaders/polyShader.frag");

	polyShader.CreateAndLinkProgram();
	polyShader.Use();
	
	polyShader.AddAttribute("vVertex");
	polyShader.AddUniform("MVP");
	polyShader.AddUniform("textureMap");
	
	glUniform1i(polyShader("textureMap"), 1);
	polyShader.UnUse();

	GL_CHECK_ERRORS

		
		if (LoadVolume()) {
			std::cout << "Volume data loaded successfully." << std::endl;

		}
		else {
			std::cout << "Cannot load volume data." << std::endl;
			exit(EXIT_FAILURE);
		}

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureSlices), 0, GL_DYNAMIC_DRAW);

	GL_CHECK_ERRORS

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindVertexArray(0);
	
	glm::vec3 crossHairVertices[6];
	crossHairVertices[0] = glm::vec3(-0.5f, 0, 0);
	crossHairVertices[1] = glm::vec3(0.5f, 0, 0);
	crossHairVertices[2] = glm::vec3(0, -0.5f, 0);
	crossHairVertices[3] = glm::vec3(0, 0.5f, 0);
	crossHairVertices[4] = glm::vec3(0, 0, -0.5f);
	crossHairVertices[5] = glm::vec3(0, 0, 0.5f);

	glGenVertexArrays(1, &lightVAO);
	glGenBuffers(1, &lightVBO);
	glBindVertexArray(lightVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lightVBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(crossHairVertices), &(crossHairVertices[0].x), GL_STATIC_DRAW);
	GL_CHECK_ERRORS

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GL_CHECK_ERRORS

	glm::vec2 polyVerts[6];
	polyVerts[0] = glm::vec2(0, 0);
	polyVerts[1] = glm::vec2(1, 0);
	polyVerts[2] = glm::vec2(1, 1);
	polyVerts[3] = glm::vec2(0, 0);
	polyVerts[4] = glm::vec2(1, 1);
	polyVerts[5] = glm::vec2(0, 1);

	glGenVertexArrays(1, &polyVAO);
	glGenBuffers(1, &polyVBO);

	glBindVertexArray(polyVAO);
	glBindBuffer(GL_ARRAY_BUFFER, polyVBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(polyVerts), &polyVerts[0], GL_STATIC_DRAW);

	GL_CHECK_ERRORS

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	lightPos.x = radius * cos(theta) * sin(phi);
	lightPos.y = radius * cos(phi);
	lightPos.z = radius * sin(theta) * sin(phi);

	glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, camDis));
	glm::mat4 Rx = glm::rotate(T, rotX, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 MV = glm::rotate(Rx, rotY, glm::vec3(0.0f, 1.0f, 0.0f));

	viewDir = -glm::vec3(MV[0][2], MV[1][2], MV[2][2]);

	SliceVolume();

	glGenFramebuffers(1, &lightFBO);

	glGenTextures(1, &lightBuffer);
	glGenTextures(1, &eyeBuffer);

	GL_CHECK_ERRORS

	glActiveTexture(GL_TEXTURE2);
	lightBuffer = CreateTexture(imgWidth, imgHeight, GL_RGBA16F, GL_RGBA);
	eyeBuffer = CreateTexture(imgWidth, imgHeight, GL_RGBA16F, GL_RGBA);

	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
	
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightBuffer, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, eyeBuffer, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status == GL_FRAMEBUFFER_COMPLETE)
		printf("Light FBO setup successful !!! \n");
	else
		printf("Problem with Light FBO setup");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	lightMV = glm::lookAt(lightPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	lightP = glm::perspective(glm::radians(45.0f), 1.0f, 1.0f, 200.0f);
	lightB = glm::scale(glm::translate(glm::mat4(1), glm::vec3(0.5, 0.5, 0.5)), glm::vec3(0.5, 0.5, 0.5));
	lightBP = lightB * lightP;
	lightShadow = lightBP * lightMV;

	cout << "Initialization successfull" << endl;

	glActiveTexture(GL_TEXTURE1);
	
	glutCloseFunc(OnShutdown);
	glutDisplayFunc(OnRender);
	glutReshapeFunc(OnResize);
	glutMouseFunc(OnMouseDown);
	glutMotionFunc(OnMouseMove);
	glutMouseWheelFunc(OnMouseWheel);
	glutKeyboardFunc(OnKey);
	
	BuildPopupMenu();
	glutAttachMenu(GLUT_MIDDLE_BUTTON);

	glutMainLoop();

	return 0;
}

void OnShutdown() {
	ShutdownFBO();

	shader.DeleteShaderProgram();
	shadowShader.DeleteShaderProgram();
	flatShader.DeleteShaderProgram();
	polyShader.DeleteShaderProgram();

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	glDeleteVertexArrays(1, &polyVAO);
	glDeleteBuffers(1, &polyVBO);

	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &lightVBO);

	glDeleteTextures(1, &textureID);
	delete grid;
	cout << "Shutdown successfull" << endl;
}

void ShutdownFBO() {
	glDeleteFramebuffers(1, &lightFBO);
	glDeleteTextures(1, &lightBuffer);
	glDeleteTextures(1, &eyeBuffer);
}

void OnRender() {
	
	GL_CHECK_ERRORS

	glm::mat4 Tr = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, camDis));
	glm::mat4 Rx = glm::rotate(Tr, rotX, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 MV = glm::rotate(Rx, rotY, glm::vec3(0.0f, 1.0f, 0.0f));

	viewDir = -glm::vec3(MV[0][2], MV[1][2], MV[2][2]);

	lightVec = glm::normalize(lightPos);

	isViewInverted = glm::dot(viewDir, lightVec) < 0;

	halfAngle = glm::normalize((isViewInverted ? -viewDir : viewDir) + lightVec);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 MVP = P * MV;

	grid->Render(glm::value_ptr(MVP));

	SliceVolume();

	glEnable(GL_BLEND);

	DrawSlices(MVP);

	glDisable(GL_BLEND);

	glBindVertexArray(lightVAO); {
		glm::mat4 T = glm::translate(glm::mat4(1), lightPos);
		flatShader.Use();
		glUniformMatrix4fv(flatShader("MVP"), 1, GL_FALSE, glm::value_ptr(P * MV * T));
		glDrawArrays(GL_LINES, 0, 6);
		flatShader.UnUse();
	}

	glutSwapBuffers();
}

void DrawSlices(const glm::mat4& MVP) {

	GL_CHECK_ERRORS

	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
	glDrawBuffer(attachIDs[0]);
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glDrawBuffer(attachIDs[1]);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	GL_CHECK_ERRORS

	glBindVertexArray(VAO);

	for (int i = 0; i < num_slices; i++) {
		
		shadowShader.Use();
		glUniformMatrix4fv(shadowShader("MVP"), 1, GL_FALSE, glm::value_ptr(MVP));
		glUniformMatrix4fv(shadowShader("S"), 1, GL_FALSE, glm::value_ptr(lightShadow));

		glBindTexture(GL_TEXTURE_2D, lightBuffer);

		DrawSliceFromEyePointOfView(i);

		GL_CHECK_ERRORS

		shader.Use();

		glUniformMatrix4fv(shader("MVP"), 1, GL_FALSE, glm::value_ptr(lightP * lightMV));

		DrawSliceFromLightPointOfView(i);

		GL_CHECK_ERRORS
	}
	
	glBindVertexArray(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK_LEFT);

	GL_CHECK_ERRORS

	glViewport(0, 0, width, height);

	glBindTexture(GL_TEXTURE_2D, eyeBuffer);

	glBindVertexArray(polyVAO);
	polyShader.Use();
	
	glDrawArrays(GL_TRIANGLES, 0, 6);
	polyShader.UnUse();

	glBindVertexArray(0);

	GL_CHECK_ERRORS
}

void DrawSliceFromEyePointOfView(const int i) {
	GL_CHECK_ERRORS

	glDrawBuffer(attachIDs[1]);

	glViewport(0, 0, imgWidth, imgHeight);

	GL_CHECK_ERRORS

		if (isViewInverted) {
			
			glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
		}
		else {
			
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}

	glDrawArrays(GL_TRIANGLES, 12 * i, 12);

	GL_CHECK_ERRORS

}

void DrawSliceFromLightPointOfView(const int i) {

	glDrawBuffer(attachIDs[0]);

	glViewport(0, 0, imgWidth, imgHeight);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDrawArrays(GL_TRIANGLES, 12 * i, 12);
}

void OnResize(int w, int h) {
	
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	
	P = glm::perspective(glm::radians(60.0f), (float)w / h, 0.1f, 1000.0f);
}


void OnMouseDown(int button, int s, int x, int y)
{
	if (s == GLUT_DOWN)
	{
		prevX = x;
		prevY = y;
	}

	if (button == GLUT_MIDDLE_BUTTON)
		camState = 0;
	else if (button == GLUT_RIGHT_BUTTON)
		camState = 2;
	else
		camState = 1;
	if (s == GLUT_UP)
		rotateView = false;
}

void OnMouseMove(int x, int y)
{
	if (camState == 0) {
		camDis += (y - prevY) / 50.0f;
	}
	else if (camState == 2) {
		theta += (prevX - x) / 60.0f;
		phi += (y - prevY) / 60.0f;

		lightPos.x = radius * cos(theta) * sin(phi);
		lightPos.y = radius * cos(phi);
		lightPos.z = radius * sin(theta) * sin(phi);

		lightMV = glm::lookAt(lightPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		lightShadow = lightBP * lightMV;
	}
	else {
		rotX += (y - prevY) / 5.0f;
		rotY += (x - prevX) / 5.0f;
		rotateView = true;
	}
	prevX = x;
	prevY = y;

	glutPostRedisplay();
}

void OnMouseWheel(int button, int dir, int x, int y) {

	if (dir > 0)
	{
		radius += 0.1f;
	}
	else
	{
		radius -= 0.1f;
	}

	radius = max(radius, 0.0f);

	lightPos.x = radius * cos(theta) * sin(phi);
	lightPos.y = radius * cos(phi);
	lightPos.z = radius * sin(theta) * sin(phi);

	lightMV = glm::lookAt(lightPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	lightShadow = lightBP * lightMV;
	glutPostRedisplay();
}

void OnKey(unsigned char key, int x, int y) {
	switch (key) {
	case '-':
		num_slices--;
		break;

	case 'p':
		num_slices++;
		break;
	}
	
	num_slices = min(maxSlices, max(num_slices, 3));

	SliceVolume();
	
	glutPostRedisplay();
}

int BuildPopupMenu(void)
{
	int subMenu3 = glutCreateMenu(assetMenu);
	glutAddMenuEntry("BostonTeapot256", 0);
	glutAddMenuEntry("Engine256", 1);
	glutAddMenuEntry("head256", 2);

	int subMenu2 = glutCreateMenu(polyMenu);
	glutAddMenuEntry("Polygon", 0);
	glutAddMenuEntry("Lines", 1);
	glutAddMenuEntry("Points", 2);

	int subMenu = glutCreateMenu(subMenuHandler);
	glutAddMenuEntry("Default", 0);
	glutAddMenuEntry("Red", 1);
	glutAddMenuEntry("Green", 2);
	glutAddMenuEntry("Blue", 3);
	glutAddMenuEntry("Yellow", 4);
	glutAddMenuEntry("Purple", 5);

	int menu;
	menu = glutCreateMenu(SelectFromMenu);
	
	glutAddMenuEntry("Toggle lighting", MENU_Lighting);
	glutAddSubMenu("Change Color", subMenu);
	glutAddSubMenu("Toggle polygon fill", subMenu2);
	glutAddSubMenu("Toggle Model", subMenu3);
	glutAddMenuEntry("Exit demo Esc", MENU_EXIT);
	return menu;
}

void subMenuHandler(int choice) {
	lightColor = colors[choice];
	glutPostRedisplay();
}

void polyMenu(int choice) {
	if (choice == 0) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		
	}

	//Line
	if (choice == 1) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		
	}

	//Solid
	if (choice == 2) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		
	}
	glutPostRedisplay();
}

void assetMenu(int choice) {
	filepath = assetpath[choice];
	glutPostRedisplay();
}

void SelectFromMenu(int idCommand)
{
	switch (idCommand)
	{
	case MENU_Lighting:
		enableLight = !enableLight;
		if (enableLight)
			glEnable(GL_LIGHTING);
		else
			glDisable(GL_LIGHTING);
		break;
	/*case MENU_Polymode:
		fillPolygon = !fillPolygon;
		glPolygonMode(GL_FRONT_AND_BACK, fillPolygon ? GL_FILL : GL_LINE);
		break;
	case MENU_Asset:
		toggleAsset = !toggleAsset;
		if (toggleAsset) {
			filepath = "assets/Engine256.raw";
		}
		else {
			filepath = "assets/BostonTeapot.raw";
		}
		break;
	case MENU_Color:
		toggleColor = !toggleColor;
		if (toggleColor)
			lightColor = glm::vec4(1.0, 1.0, 1.0, 1.0);
		else
			lightColor = glm::vec4(0.7, 0.3, 1.0, 1.0);
		break;*/
	case MENU_EXIT:
		exit(0);
		break;
	}
	glutPostRedisplay();
}
