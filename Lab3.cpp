
//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>

#include "teapot.h" // teapot mesh
#include <string> 
#include <fstream>
#include <iostream>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#define NUM_POTS 4

using namespace std;
GLuint shaderProgramID;

unsigned int teapot_vao = 0;
int width = 800.0;
int height = 600.0;
GLuint loc1;
GLuint loc2;

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

std::string readShaderSource(const std::string& fileName)
{
	std::ifstream file(fileName.c_str());
	if (file.fail()) {
		cout << "error loading shader called " << fileName;
		exit(1);
	}

	std::stringstream stream;
	stream << file.rdbuf();
	file.close();

	return stream.str();
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	std::string outShader = readShaderSource(pShaderText);
	const char* pShaderSource = outShader.c_str();

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, "../Shaders/simpleVertexShader.vs", GL_VERTEX_SHADER);
	AddShader(shaderProgramID, "../Shaders/simpleFragmentShader.fs", GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };


	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);

	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS





void generateObjectBufferTeapot() {
	GLuint vp_vbo = 0;

	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normals");

	glGenBuffers(1, &vp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * teapot_vertex_count * sizeof(float), teapot_vertex_points, GL_STATIC_DRAW);
	GLuint vn_vbo = 0;
	glGenBuffers(1, &vn_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * teapot_vertex_count * sizeof(float), teapot_normals, GL_STATIC_DRAW);

	glGenVertexArrays(1, &teapot_vao);
	glBindVertexArray(teapot_vao);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}


#pragma endregion VBO_FUNCTIONS

float Next = 0.0f;
float ANGLE = 0.0f;
float direction = 1.0f;
float oldtime = 0;

void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);


	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");


	//Here is where the code for the viewport lab will go, to get you started I have drawn a t-pot in the bottom left
	//The model transform rotates the object by 45 degrees, the view transform sets the camera at -40 on the z-axis, and the perspective projection is setup using Antons method

	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, -40.0f));
	glm::mat4 persp_proj = glm::perspective(45.0f, (float)width / (float)height, 0.01f, 100.0f);
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(ANGLE), glm::vec3(0.0f, 0.0f, -1.0f));
	float newTime = (float)(glutGet(GLUT_ELAPSED_TIME));
	float delta = newTime - oldtime;
	oldtime = newTime;
	if (ANGLE >= 1680.0f) {
		direction = -1.0f;
	}
	else if(ANGLE <= 0.0f)
	{
		direction = 1.0f;
	}
	ANGLE += delta * direction;

	// bottom left
	glViewport(0, 0, width / 2, height / 2);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(model));
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// bottom-right
	persp_proj = glm::perspective(90.0f, (float)width / (float)height, 0.01f, 100.0f);
	view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -20.0f));
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	//model = glm::rotate(model, glm::radians(-4000.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glViewport(width / 2, 0, width / 2, height / 2);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(model));
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);
	
	// top-left
	persp_proj = glm::ortho(-20.0f, 20.0f, -15.0f, 15.0f, 0.0f, 100.0f);
	view = glm::translate(glm::mat4(1.0f), glm::vec3(00.0f, 00.0f, -40.0f));
	//view = glm::translate(glm::mat4(1.0f), glm::vec3(-100.0f, -30.0f, -30.0f));
	glViewport(0, height / 2, width / 2, height / 2);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(model));
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// top-right
	float radius = 15.0f;
	float camX = sin(Next) * radius ;
	float camZ = cos(Next) * radius;
	Next += 0.01f;
	view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 6.0f, -60.0f));
	view = glm::lookAt(glm::vec3(camX, 0.0, camZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	glViewport(width / 2, height / 2, width / 2, height / 2);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, glm::value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, glm::value_ptr(model));
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	glutSwapBuffers();
}


void updateScene() {

	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.3f)
		delta = 0.3f;
	last_time = curr_time;

	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Create 3 vertices that make up a triangle that fits on the viewport 
	GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0,
		1.0f, -1.0f, 0.0f, 1.0,
		0.0f, 1.0f, 0.0f, 1.0 };
	// Create a color array that identfies the colors of each vertex (format R, G, B, A)
	GLfloat colors[] = { 0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f };
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();

	// load teapot mesh into a vertex buffer array
	generateObjectBufferTeapot();

}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Viewport Teapots");
	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);

	// A call to glewInit() must be done after glut is initialized!
	glewExperimental = GL_TRUE; //for non-lab machines, this line gives better modern GL support
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	getchar();
	return 0;
}

