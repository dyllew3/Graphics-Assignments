
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#define NUM_VERTICES 3

#define NUM_VERT_ARRAYS 2
#define NUM_TRANSFORMS 4

using namespace std;

struct transformation {
	glm::vec3 angles;
	glm::vec3 rotation;
	glm::vec3 translation;
	glm::vec3 scale;

};

struct transformation transformations[NUM_TRANSFORMS];

GLuint VAO[NUM_VERT_ARRAYS];
GLuint VBO;
GLuint shaders[NUM_VERT_ARRAYS];


// Vertex Shader (for convenience, it is defined in the main here, but we will be using text files for shaders in future)
// Note: Input to this shader is the vertex positions that we specified for the triangle. 
// Note: gl_Position is a special built-in variable that is supposed to contain the vertex position (in X, Y, Z, W)
// Since our triangle vertices were specified as vec3, we just set W to 1.0.

static const char* pVS = "                                                    \n\
#version 330                                                                  \n\
                                                                              \n\
in vec3 vPosition;															  \n\
in vec4 vColor;																  \n\
out vec4 color;																 \n\
                                                                              \n\
uniform mat4 transform;																 \n\
                                                                               \n\
void main()                                                                     \n\
{                                                                                \n\
    gl_Position = transform * vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);  \n\
	color = vColor;							\n\
}";

// Fragment Shader
// Note: no input in this shader, it just outputs the colour of all fragments, in this case set to red (format: R, G, B, A).
static const char* pFS = "                                              \n\
#version 330                                                            \n\
                                                                        \n\
in vec4 color;                                                          \n\
out vec4 FragColor;                                                      \n\
                                                                          \n\
void main()                                                               \n\
{                                                                          \n\
FragColor = vec4(color.x, color.y, color.z, 1.0);									 \n\
}";


// Fragment Shader 2
// Note: no input in this shader, it just outputs the colour of all fragments, in this case set to red (format: R, G, B, A).
static const char* pFS2 = "                                              \n\
#version 330                                                            \n\
                                                                        \n\
out vec4 FragColor;                                                      \n\
                                                                          \n\
void main()                                                               \n\
{                                                                          \n\
FragColor = vec4(1.0, 1.0, 0.0, 1.0);									 \n\
}";


// Generate transformations
static void generateTransformations() {
	for (int i = 0; i < NUM_TRANSFORMS; i++)
	{
		transformations[i] = {
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(1.0f, 1.0f, 1.0f)
		};
		if (i == 1)
		{
			transformations[i].translation = glm::vec3(-0.3f, 0.3f, 0.0f);
		}
		if (i == 2) {
			transformations[i].translation = glm::vec3(0.3f, 0.3f, 0.0f);
		}
		if (i == 3)
		{
			transformations[i].rotation = glm::vec3(1.0f, 1.0f, -1.0f);
			transformations[i].translation = glm::vec3(-0.8f, 0.8f, 0.0f);
		}
	}
}

static glm::mat4 rotate(int index, glm::mat4 matrix){

	glm::mat4 newMat = matrix;
	if (transformations[index].rotation.x != 0.0f)
	{
		newMat = glm::rotate(newMat, glm::radians(transformations[index].angles.x), glm::vec3(transformations[index].rotation.x, 0.0f, 0.0f));
	}
	if (transformations[index].rotation.y != 0.0f)
	{
		newMat = glm::rotate(newMat, glm::radians(transformations[index].angles.y), glm::vec3(0.0f, transformations[index].rotation.y,  0.0f));
	}
	if (transformations[index].rotation.z != 0.0f)
	{
		newMat = glm::rotate(newMat, glm::radians(transformations[index].angles.z), glm::vec3(0.0f,  0.0f, transformations[index].rotation.z));
	}
	return newMat;
}

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderText, NULL);
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

GLuint createShader(const char * vertex_shader, const char * frag_shader) {
	GLuint shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, vertex_shader, GL_VERTEX_SHADER);
	AddShader(shaderProgramID, frag_shader, GL_FRAGMENT_SHADER);

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
	return shaderProgramID;
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
    GLuint shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

	// Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, pVS, GL_VERTEX_SHADER);
    AddShader(shaderProgramID, pFS, GL_FRAGMENT_SHADER);

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
GLuint generateObjectBuffer(GLfloat vertices[], GLfloat colors[]) {
	GLuint numVertices = 3;
	// Genderate 1 generic buffer object, called VBO

 	glGenBuffers(1, &VBO);
	// In OpenGL, we bind (make active) the handle to a target name and then execute commands on that target
	// Buffer will contain an array of vertices 
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// After binding, we now fill our object with data, everything in "Vertices" goes to the GPU
	glBufferData(GL_ARRAY_BUFFER, numVertices*7*sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	// if you have more data besides vertices (e.g., vertex colours or normals), use glBufferSubData to tell the buffer when the vertices array ends and when the colors start
	glBufferSubData (GL_ARRAY_BUFFER, 0, numVertices*3*sizeof(GLfloat), vertices);
	glBufferSubData (GL_ARRAY_BUFFER, numVertices*3*sizeof(GLfloat), numVertices*4*sizeof(GLfloat), colors);
return VBO;
}

void linkCurrentBuffertoShader(GLuint shaderProgramID){
	GLuint numVertices = 3;
	// find the location of the variables that we will be using in the shader program
	GLuint positionID = glGetAttribLocation(shaderProgramID, "vPosition");
	GLuint colorID = glGetAttribLocation(shaderProgramID, "vColor");
	// Have to enable this
	glEnableVertexAttribArray(positionID);
	// Tell it where to find the position data in the currently active buffer (at index positionID)
    glVertexAttribPointer(positionID, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// Similarly, for the color data.
	glEnableVertexAttribArray(colorID);
	glVertexAttribPointer(colorID, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(numVertices*3*sizeof(GLfloat)));
}
#pragma endregion VBO_FUNCTIONS


void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
		case 'q':
			transformations[0].rotation.z = -1.0f;
			transformations[0].angles.z += 40.0f;
			// call a function
			break;

		case 'w':
			transformations[0].rotation.y = 1.0f;
			transformations[0].angles.y += 40.0f;
			break;

		case 'e':
			transformations[0].rotation.x = 1.0f;
			transformations[0].angles.x += 40.0f;

			break;
		case 'i':
			// call a function
			transformations[1].translation.y += (float)0.01;
			break;

		case 'k':
			transformations[1].translation.y -= (float)0.01;
			break;

		case 'j':
			transformations[1].translation.x -= (float)0.01;
			break;

		case 'l':
			transformations[1].translation.x += (float)0.01;
			break;

		case 'u':
			transformations[1].translation.z += (float)0.01;
			break;

		case 'o':
			transformations[1].translation.z -= (float)0.01;
			break;
		case 'r':
			generateTransformations();
			break;

		case 'z':
			transformations[2].scale += glm::vec3(1.0f, 1.0f, 1.0f);
			break;

		case 'x':
			transformations[2].scale += glm::vec3(0.3f, -0.1f, 1.0f);
			break;

		case 'c':
			transformations[3].angles += glm::vec3(40.0f);
			transformations[3].translation  += glm::vec3(0.01f, -0.01 ,0.0f);
			break;

		default:
			break;

	}
	glutPostRedisplay(); /* this redraws the scene without
						 waiting for the display callback so that any changes appear
						 instantly */
}


void display(){

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
	glm::mat4 transform;
	// NB: Make the call to draw the geometry in the currently activated vertex buffer. This is where the GPU starts to work!
	glBindVertexArray(VAO[0]);
	glUseProgram(shaders[0]);
	for ( int i = 0; i < NUM_TRANSFORMS; i++)
	{
		transform = glm::mat4();
		transform = glm::translate(transform, transformations[i].translation);

		transform = glm::scale(transform, transformations[i].scale);
		transform = rotate(i, transform);
		unsigned int transformLoc = glGetUniformLocation(shaders[0], "transform");
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	glutSwapBuffers();
	glutPostRedisplay();

}


void init(){

	glGenVertexArrays(2, VAO);
	glBindVertexArray(VAO[0]);
	// Create 6 vertices that make up a triangle that fits on the viewport 
	GLfloat vertices[] = {
		-0.2f, -0.2f, 0.0f,
		0.2f, -0.2f, 0.0f,
		0.0f,  0.2f, 0.0f
	};
	shaders[0] = createShader(pVS, pFS);
	shaders[1] = createShader(pVS, pFS2);
	// Create a color array that identfies the colors of each vertex (format R, G, B, A)
	GLfloat colors[] = {
		0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,

	};
	// Set up the shaders

	// Put the vertices and colors into a vertex buffer object
	generateObjectBuffer(vertices, colors);
	// Link the current buffer to the shader

	linkCurrentBuffertoShader(shaders[0]);

	GLfloat vertices2[] = {
		-0.25f, -0.5f, 0.0f,
		0.25f, -0.5f, 0.0f,
		0.00f, 0.0f, 0.0f
	};

	glBindVertexArray(VAO[1]);

	// Put the vertices and colors into a vertex buffer object
	generateObjectBuffer(vertices2, colors);
	// Link the current buffer to the shader

	linkCurrentBuffertoShader(shaders[1]);
}

int main(int argc, char** argv){

	generateTransformations();

	// Set up the window
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Hello Triangle");
	// Tell glut where the display function is
	glutDisplayFunc(display);

	 // A call to glewInit() must be done after glut is initialized!
    GLenum res = glewInit();
	// Check for any errors
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }
	// Set up your objects and shaders
	init();
	glutKeyboardFunc(&keyboard);
	// Begin infinite event loop
	glutMainLoop();
    return 0;
}