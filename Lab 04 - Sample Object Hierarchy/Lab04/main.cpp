// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.

// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

// Project includes
//#include "maths_funcs.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define MESH_NAME "monkeyhead_smooth.dae"
/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

#define SEC_MESH "Muscle car .dae"


#pragma region SimpleTypes
typedef struct
{
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
} ModelData;
#pragma endregion SimpleTypes

using namespace std;
GLuint shader_programID;

ModelData mesh_data[2];
unsigned int mesh_vao = 0;
unsigned int vao[2];
int width = 800;
int height = 600;

GLuint loc1, loc2, loc3;
GLfloat rotate_y = 0.0f;
vec3 move_vec = vec3(0.0f, 0.0f, 0.0f);

#pragma region MESH LOADING
/*----------------------------------------------------------------------------
MESH LOADING FUNCTION
----------------------------------------------------------------------------*/

ModelData load_mesh(const char* file_name) {
	ModelData modelData;

	/* Use assimp to read the model file, forcing it to be read as    */
	/* triangles. The second flag (aiProcess_PreTransformVertices) is */
	/* relevant if there are multiple meshes in the model file that   */
	/* are offset from the origin. This is pre-transform them so      */
	/* they're in the right position.                                 */
	const aiScene* scene = aiImportFile(
		file_name, 
		aiProcess_Triangulate | aiProcess_PreTransformVertices
	); 

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		modelData.mPointCount += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
			}
			if (mesh->HasTangentsAndBitangents()) {
				/* You can extract tangents and bitangents here              */
				/* Note that you might need to make Assimp generate this     */
				/* data for you. Take a look at the flags that aiImportFile  */
				/* can take.                                                 */
			}
		}
	}

	aiReleaseImport(scene);
	return modelData;
}

#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
char* readShaderSource(const char* shaderFile) {
	FILE* fp;
	fopen_s(&fp, shaderFile, "rb");

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}


static void add_shader(GLuint shader_program, const char* p_shader_text, GLenum shader_type)
{
	// create a shader object
	GLuint shader_obj = glCreateShader(shader_type);

	if (shader_obj == 0) {
		std::cerr << "Error creating shader..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	const char* pShaderSource = readShaderSource(p_shader_text);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(shader_obj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(shader_obj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(shader_obj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024] = { '\0' };
		glGetShaderInfoLog(shader_obj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling "
			<< (shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment")
			<< " shader program: " << InfoLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(shader_program, shader_obj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shader_programID = glCreateProgram();
	if (shader_programID == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	add_shader(shader_programID, "simpleVertexShader.txt", GL_VERTEX_SHADER);
	add_shader(shader_programID, "simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

	GLint success = 0;
	GLchar ErrorLog[1024] = { '\0' };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shader_programID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shader_programID, GL_LINK_STATUS, &success);
	if (success == 0) {
		glGetProgramInfoLog(shader_programID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shader_programID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shader_programID, GL_VALIDATE_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shader_programID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shader_programID);
	return shader_programID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
void gen_buffer_mesh() {
	/*----------------------------------------------------------------------------
	LOAD MESH HERE AND COPY INTO BUFFERS
	----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

	mesh_data[0] = load_mesh(SEC_MESH);
	glGenVertexArrays(1, vao);
	glBindVertexArray(vao[0]);

	unsigned int vp_vbo = 0;
	loc1 = glGetAttribLocation(shader_programID, "vertex_position");
	loc2 = glGetAttribLocation(shader_programID, "vertex_normal");
	loc3 = glGetAttribLocation(shader_programID, "vertex_texture");




	glGenBuffers(1, &vp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh_data[0].mPointCount * sizeof(vec3), &mesh_data[0].mVertices[0], GL_STATIC_DRAW);
	unsigned int vn_vbo = 0;
	glGenBuffers(1, &vn_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh_data[0].mPointCount * sizeof(vec3), &mesh_data[0].mNormals[0], GL_STATIC_DRAW);


	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	//	This is for texture coordinates which you don't currently need, so I have commented it out
	//	unsigned int vt_vbo = 0;
	//	glGenBuffers (1, &vt_vbo);
	//	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	//	glBufferData (GL_ARRAY_BUFFER, monkey_head_data.mTextureCoords * sizeof (vec2), &monkey_head_data.mTextureCoords[0], GL_STATIC_DRAW);




	//	This is for texture coordinates which you don't currently need, so I have commented it out
	//	glEnableVertexAttribArray (loc3);
	//	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	//	glVertexAttribPointer (loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	mesh_data[1] = load_mesh(MESH_NAME);
	glBindVertexArray(vao[1]);

	unsigned int vp_vbo2 = 0;
	loc1 = glGetAttribLocation(shader_programID, "vertex_position");
	loc2 = glGetAttribLocation(shader_programID, "vertex_normal");
	loc3 = glGetAttribLocation(shader_programID, "vertex_texture");




	glGenBuffers(1, &vp_vbo2);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo2);
	glBufferData(GL_ARRAY_BUFFER, mesh_data[1].mPointCount * sizeof(vec3), &mesh_data[1].mVertices[0], GL_STATIC_DRAW);
	unsigned int vn_vbo2 = 0;
	glGenBuffers(1, &vn_vbo2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo2);
	glBufferData(GL_ARRAY_BUFFER, mesh_data[1].mPointCount * sizeof(vec3), &mesh_data[1].mNormals[0], GL_STATIC_DRAW);


	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo2);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo2);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}
#pragma endregion VBO_FUNCTIONS


void updateShaderMatrix(mat4 view, mat4 persp_proj, mat4 model) {
	int matrix_location = glGetUniformLocation(shader_programID, "model");
	int view_mat_location = glGetUniformLocation(shader_programID, "view");
	int proj_mat_location = glGetUniformLocation(shader_programID, "proj");

	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, value_ptr(model));

}


mat4 draw_child(mat4 parent, mat4 child, ModelData &mesh) {

	int matrix_location = glGetUniformLocation(shader_programID, "model");
	mat4 new_child = mat4(1.0f);


	// Apply the root matrix to the child matrix
	new_child = parent * child;

	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, value_ptr(new_child));
	glDrawArrays(GL_TRIANGLES, 0, mesh.mPointCount);

	return new_child;
}

void rightChild(mat4 root_model, int matrix_location, ModelData mesh) {
	mat4 right_child = mat4(1.0f);
	right_child = translate(right_child, vec3(6.0f, 0.0f, 0.0f));
	right_child = rotate(right_child, -90.0f, vec3(0.0f, 0.0f, -1.0f));
	right_child = rotate(right_child, rotate_y, vec3(0.0f, -1.0f, 0.0f));


	// Apply the root matrix to the child matrix
	right_child = root_model * right_child;

	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, value_ptr(right_child));
	glDrawArrays(GL_TRIANGLES, 0, mesh.mPointCount);
}

void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shader_programID);

	glBindVertexArray(vao[0]);

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shader_programID, "model");
	int view_mat_location = glGetUniformLocation(shader_programID, "view");
	int proj_mat_location = glGetUniformLocation(shader_programID, "proj");


	// Root of the Hierarchy
	mat4 view = mat4(1.0f);
	mat4 persp_proj = perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);
	mat4 model = mat4(1.0f);
	model = translate(model, move_vec);
	model = rotate(model, rotate_y, vec3(0.0f, 0.0f, -1.0f));
	model = scale(model, vec3(0.4f, 0.4f, 0.4f));
	view = translate(view, vec3(0.0f, 0.0, -10.0f));

	// update uniforms & draw
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, value_ptr(view));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, value_ptr(model));
	glDrawArrays(GL_TRIANGLES, 0, mesh_data[0].mPointCount);

	{
		// Set up the child matrix
		mat4 modelChild = mat4(1.0f);
		modelChild = translate(modelChild, vec3(0.0f, 11.0f, 0.0f));
		modelChild = rotate(modelChild, 180.0f, vec3(0.0f, 0.0f, -1.0f));
		modelChild = rotate(modelChild, rotate_y, vec3(0.0f, 1.0f, 0.0f));


		// Apply the root matrix to the child matrix
		modelChild = draw_child(model, modelChild, mesh_data[0]);

		// Update the appropriate uniform and draw the mesh again
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, value_ptr(modelChild));
		glDrawArrays(GL_TRIANGLES, 0, mesh_data[0].mPointCount);

		mat4 modelChild2 = mat4(1.0f);
		modelChild2 = translate(modelChild2, vec3(0.0f, 5.0f, 0.0f));
		modelChild2 = rotate(modelChild2, 90.0f, vec3(0.0f, 0.0f, -1.0f));
		//modelChild2 = rotate(modelChild2, rotate_y, vec3(1.0f, 0.0f, 0.0f));


		// Apply the root matrix to the child matrix
		draw_child(modelChild, modelChild2, mesh_data[0]);
	}
	{
		mat4 modelLeftChild = mat4(1.0f);
		modelLeftChild = translate(modelLeftChild, vec3(-7.0f, 0.0f, 0.0f));
		modelLeftChild = rotate(modelLeftChild, 90.0f, vec3(0.0f, 0.0f, -1.0f));
		modelLeftChild = rotate(modelLeftChild, rotate_y, vec3(0.0f, -1.0f, 0.0f));


		// Apply the root matrix to the child matrix
		modelLeftChild = model * modelLeftChild;

		// Update the appropriate uniform and draw the mesh again
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, value_ptr(modelLeftChild));
		glDrawArrays(GL_TRIANGLES, 0, mesh_data[0].mPointCount);
	}
	glBindVertexArray(vao[1]);
	rightChild(model, matrix_location, mesh_data[1]);

	glutSwapBuffers();
}


void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	// Rotate the model slowly around the y axis at 20 degrees per second
	rotate_y += 20.0f * delta;
	rotate_y = fmodf(rotate_y, 360.0f);
	// Draw the next frame
	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
		case 'q':
			move_vec.z += 0.1f;
			break;

		case 'w':
			move_vec.y += 0.1f;
			break;
		
		case 's':
			move_vec.y -= 0.1f;
			break;

		case 'a':
			move_vec.x -= 0.1f;
			break;


		case 'd':
			move_vec.x += 0.1f;
			break;

		case 'e':
			move_vec.z -= 0.1f;
			break;

		case 'r':
			move_vec.x = 0.0f;
			move_vec.y = 0.0f;
			move_vec.z = 0.0f;
			break;

		default:
			break;

	}
	glutPostRedisplay(); /* this redraws the scene without
						 waiting for the display callback so that any changes appear
						 instantly */
}


void init()
{
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	// load mesh into a vertex buffer array
	gen_buffer_mesh();

}

// Placeholder code for the keypress
int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Hello Triangle");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
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
