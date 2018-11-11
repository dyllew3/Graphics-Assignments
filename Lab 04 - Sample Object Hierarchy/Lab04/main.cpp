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

#include <map>
#include <string>
#include"cube.h"
#include"camera.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
unsigned int skybox;
unsigned int skyboxVAO, skyboxVBO;

using namespace glm;

/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define MESH_NAME "monkeyhead_smooth.dae"
/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

float lastX = 800 / 2.0f;
float lastY = 600 / 2.0f;
bool firstMouse = true;
float delta = 0.0f;
unsigned int diffuseMap;
unsigned int specularMap;

#define SEC_MESH "Muscle car .dae"
#define TRAIN "steyerdorf.obj"
#define BOX_CAR "x1014_boxcar.obj"


vec3 lightPos(1.2f, 1.0f, 2.0f);
GLuint shader_programID;
std::map<std::string, GLuint> shaders;


#pragma region SimpleTypes
typedef struct
{
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
} ModelData;

typedef struct {
	float degrees;
	vec3 rotate;

} rotating;

typedef struct
{
	vec3 translate;
	std::vector<rotating> rotate;
	vec3 scale;

} transforms;


typedef struct
{
	vec3 position;

	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;

	float bounds;
	float outerBounds;
} PhongLight;

#pragma endregion SimpleTypes

class ModelObject {

public:

	ModelObject() {

	}

	ModelObject(ModelData mod) {
		model = mod;
		transform = mat4(1.0f);
		children = std::vector<ModelObject>();
	}

	ModelObject(ModelData mod, mat4 mat) {
		model = mod;
		transform = mat;
		children = std::vector<ModelObject>();
	}

	ModelObject(ModelData mod, transforms tran) {
		model = mod;
		setTransformation(tran);
		children = std::vector<ModelObject>();
	}

	void createChild(mat4 child_mat) {
		children.push_back(ModelObject(model, child_mat));
	}


	void createChild(transforms child) {
		children.push_back(ModelObject(model, child));
	}

	ModelObject& getChild(int index) {
		return (children[index]);
	};

	void display(mat4 &parent) {
		int matrix_location = glGetUniformLocation(shader_programID, "model");
		mat4 result = parent * transform;
		// Update the appropriate uniform and draw the mesh again
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, value_ptr(result));
		glDrawArrays(GL_TRIANGLES, 0, model.mPointCount);
		for (std::vector<ModelObject>::iterator i = children.begin(); i != children.end(); i++)
		{
			i->display(result);
		}
	}

	void display() {

		int matrix_location = glGetUniformLocation(shader_programID, "model");

		// Update the appropriate uniform and draw the mesh again
		mat4 local = transform;
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, value_ptr(local));
		glDrawArrays(GL_TRIANGLES, 0, this->model.mPointCount);
		for (std::vector<ModelObject>::iterator i = this->children.begin(); i != children.end(); i++)
		{
			i->display(transform);
		}
	}


	void setTransformation(transforms update) {
		mat4 new_mat(1.0f);
		new_mat = translate(new_mat, update.translate);
		for (std::vector<rotating>::iterator i = update.rotate.begin(); i != update.rotate.end(); i++)
		{
			new_mat = rotate(new_mat, (i->degrees), i->rotate);
		}
		
		new_mat = scale(new_mat, update.scale);
		transform = new_mat;
	};


	private:
		ModelData model;
		mat4 transform;
		std::vector<ModelObject> children;
};

using namespace std;


ModelData mesh_data[2];

unsigned int vao[2];
unsigned int lightVAO;

int width = 800;
int height = 600;
ModelObject root;
GLuint loc1, loc2, loc3;
GLfloat rotate_y = 0.0f;
vec3 move_vec = vec3(0.0f, 0.0f, 0.0f);

vec3 pointLightPositions[] = {
	vec3(0.7f,  0.2f,  2.0f),
	vec3(2.3f, -3.3f, -4.0f),
	vec3(-4.0f,  2.0f, -12.0f),
	vec3(0.0f,  0.0f, -3.0f)
};

unsigned int VBO, cubeVAO;

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


unsigned int loadTexture(char const * path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

unsigned int loadCubemap(vector<string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}


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


void validate_shaders(GLuint shader) {
	GLint success = 0;
	GLchar ErrorLog[1024] = { '\0' };
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shader, GL_LINK_STATUS, &success);
	if (success == 0) {
		glGetProgramInfoLog(shader, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shader);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shader, GL_VALIDATE_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shader, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

}

void set_float(GLuint shader_id, const char * var, float val) {
	int loc = glGetUniformLocation(shader_id, var);
	glUniform1f(loc, val);
}

void set_int(GLuint shader_id, const char * var, int val) {
	int loc = glGetUniformLocation(shader_id, var);
	glUniform1i(loc, val);
}

void set_vec3(GLuint shader_id, const char * var, vec3 vect){
	int loc = glGetUniformLocation(shader_id, var);
	glUniform3fv(loc, 1, value_ptr(vect));
}

void set_mat4(GLuint shader_id, const char * var, mat4 matr) {
	int loc = glGetUniformLocation(shader_id, var);
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(matr));
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
	add_shader(shader_programID, "lightConeFragmentShader.txt", GL_FRAGMENT_SHADER);
	shaders["simple"] = shader_programID;


	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shader_programID);

	validate_shaders(shader_programID);

	// light on object
	GLuint light_shader = glCreateProgram();
	add_shader(light_shader, "lightingVertex.txt", GL_VERTEX_SHADER);
	add_shader(light_shader, "lightingFragment.txt", GL_FRAGMENT_SHADER);
	shaders["light"] = light_shader;

	glLinkProgram(light_shader);

	validate_shaders(light_shader);

	// lamp source
	GLuint light_source = glCreateProgram();

	add_shader(light_source, "lampVertex.txt", GL_VERTEX_SHADER);
	add_shader(light_source, "lampFragment.txt", GL_FRAGMENT_SHADER);
	shaders["lamp"] = light_source;



	
	glLinkProgram(light_source);
	validate_shaders(light_source);


	// cone of light
	GLuint light_cone = glCreateProgram();

	add_shader(light_cone, "lightingVertex.txt", GL_VERTEX_SHADER);
	add_shader(light_cone, "lightConeFragmentShader.txt", GL_FRAGMENT_SHADER);
	shaders["lightcone"] = light_cone;
	
	glLinkProgram(light_cone);
	validate_shaders(light_cone);

	// skybox shader
	GLuint skybox_shader = glCreateProgram();

	add_shader(skybox_shader, "skyboxVertex.txt", GL_VERTEX_SHADER);
	add_shader(skybox_shader, "skyboxFragment.txt", GL_FRAGMENT_SHADER);
	shaders["skybox"] = skybox_shader;

	glLinkProgram(skybox_shader);
	validate_shaders(skybox_shader);

	// multilight shader
	GLuint multilight_shader = glCreateProgram();

	add_shader(multilight_shader, "lightingVertex.txt", GL_VERTEX_SHADER);
	add_shader(multilight_shader, "MultiLightFragment.txt", GL_FRAGMENT_SHADER);
	shaders["multilight"] = multilight_shader;

	glLinkProgram(multilight_shader);
	validate_shaders(multilight_shader);

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

	mesh_data[0] = load_mesh(TRAIN);
	mesh_data[1] = load_mesh(BOX_CAR);
	glGenVertexArrays(2, vao);
	glBindVertexArray(vao[0]);

	unsigned int vp_vbo = 0;
	unsigned int shader = shaders["light"];
	loc1 = glGetAttribLocation(shader, "aPos");
	loc2 = glGetAttribLocation(shader, "aNormal");
	loc3 = glGetAttribLocation(shader, "aTexCoords");


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
	unsigned int vt_vbo;
	glGenBuffers (1, &vt_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	glBufferData (GL_ARRAY_BUFFER, mesh_data[0].mTextureCoords.size() * sizeof (vec2), &mesh_data[0].mTextureCoords[0], GL_STATIC_DRAW);




	//	This is for texture coordinates which you don't currently need, so I have commented it out
	glEnableVertexAttribArray (loc3);
	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	glVertexAttribPointer (loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	{

		glBindVertexArray(vao[1]);

		unsigned int vp_vbo = 0;
		unsigned int shader = shaders["light"];
		loc1 = glGetAttribLocation(shader, "aPos");
		loc2 = glGetAttribLocation(shader, "aNormal");
		loc3 = glGetAttribLocation(shader, "aTexCoords");


		glGenBuffers(1, &vp_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
		glBufferData(GL_ARRAY_BUFFER, mesh_data[1].mPointCount * sizeof(vec3), &mesh_data[1].mVertices[0], GL_STATIC_DRAW);
		unsigned int vn_vbo = 0;
		glGenBuffers(1, &vn_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
		glBufferData(GL_ARRAY_BUFFER, mesh_data[1].mPointCount * sizeof(vec3), &mesh_data[1].mNormals[0], GL_STATIC_DRAW);


		glEnableVertexAttribArray(loc1);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
		glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(loc2);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
		glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		//	This is for texture coordinates which you don't currently need, so I have commented it out
		unsigned int vt_vbo;
		glGenBuffers(1, &vt_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
		glBufferData(GL_ARRAY_BUFFER, mesh_data[1].mTextureCoords.size() * sizeof(vec2), &mesh_data[1].mTextureCoords[0], GL_STATIC_DRAW);




		//	This is for texture coordinates which you don't currently need, so I have commented it out
		glEnableVertexAttribArray(loc3);
		glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
		glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	}

	// cube

	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);


	unsigned int lightVBO;


	glGenBuffers(1, &lightVBO);

	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// note that we update the lamp's position attribute's stride to reflect the updated buffer data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// skybox gen
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}
#pragma endregion VBO_FUNCTIONS




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


vec3 trans(0.0f);

void multi_light(GLuint shader) {
	set_vec3(shader, "dirLight.direction",vec3( -0.2f, -1.0f, -0.3f));
	set_vec3(shader, "dirLight.ambient", vec3(0.05f, 0.05f, 0.05f));
	set_vec3(shader, "dirLight.diffuse", vec3(0.4f, 0.4f, 0.4f));
	set_vec3(shader, "dirLight.specular", vec3(0.5f, 0.5f, 0.5f));
	// point light 1
	set_vec3(shader, "pointLights[0].position", pointLightPositions[0]);
	set_vec3(shader, "pointLights[0].ambient", vec3(0.05f, 0.05f, 0.05f));
	set_vec3(shader, "pointLights[0].diffuse", vec3(0.8f, 0.8f, 0.8f));
	set_vec3(shader, "pointLights[0].specular", vec3(1.0f, 1.0f, 1.0f));
	set_float(shader, "pointLights[0].constant", 1.0f);
	set_float(shader, "pointLights[0].linear", 0.09);
	set_float(shader, "pointLights[0].quadratic", 0.032);
	// point light 2
	set_vec3(shader, "pointLights[1].position", pointLightPositions[1]);
	set_vec3(shader, "pointLights[1].ambient", vec3(0.05f, 0.05f, 0.05f));
	set_vec3(shader, "pointLights[1].diffuse", vec3(0.8f, 0.8f, 0.8f));
	set_vec3(shader, "pointLights[1].specular", vec3(1.0f, 1.0f, 1.0f));
	set_float(shader, "pointLights[1].constant", 1.0f);
	set_float(shader, "pointLights[1].linear", 0.09);
	set_float(shader, "pointLights[1].quadratic", 0.032);
	// point light 3
	set_vec3(shader, "pointLights[2].position", pointLightPositions[2]);
	set_vec3(shader, "pointLights[2].ambient", vec3(0.05f, 0.05f, 0.05f));
	set_vec3(shader, "pointLights[2].diffuse", vec3(0.8f, 0.8f, 0.8f));
	set_vec3(shader, "pointLights[2].specular", vec3(1.0f, 1.0f, 1.0f));
	set_float(shader, "pointLights[2].constant", 1.0f);
	set_float(shader, "pointLights[2].linear", 0.09);
	set_float(shader, "pointLights[2].quadratic", 0.032);
	// point light 4
	set_vec3(shader, "pointLights[3].position", pointLightPositions[3]);
	set_vec3(shader, "pointLights[3].ambient", vec3(0.05f, 0.05f, 0.05f));
	set_vec3(shader, "pointLights[3].diffuse", vec3(0.8f, 0.8f, 0.8f));
	set_vec3(shader, "pointLights[3].specular", vec3(1.0f, 1.0f, 1.0f));
	set_float(shader, "pointLights[3].constant", 1.0f);
	set_float(shader, "pointLights[3].linear", 0.09);
	set_float(shader, "pointLights[3].quadratic", 0.032);
	// spotLight
	set_vec3(shader, "spotLight.position", camera.Position);
	set_vec3(shader, "spotLight.direction", camera.Front);
	set_vec3(shader, "spotLight.ambient", vec3(0.0f, 0.0f, 0.0f));
	set_vec3(shader, "spotLight.diffuse", vec3(1.0f, 1.0f, 1.0f));
	set_vec3(shader, "spotLight.specular", vec3(1.0f, 1.0f, 1.0f));
	set_float(shader, "spotLight.constant", 1.0f);
	set_float(shader, "spotLight.linear", 0.09);
	set_float(shader, "spotLight.quadratic", 0.032);
	set_float(shader, "spotLight.bounds", glm::cos(glm::radians(12.5f)));
	set_float(shader, "spotLight.outerBounds", glm::cos(glm::radians(15.0f)));
}

void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	// Root of the Hierarchy
	mat4 view = mat4(1.0f);
	view = camera.GetViewMatrix();
	//view = translate(view, vec3(0.0f, 0.0f, 0.0f));a

	mat4 persp_proj = perspective(90.0f, (float)width / (float)height, 0.1f, 100.0f);
	mat4 model = mat4(1.0f);
	model = translate(model, trans + vec3(0.0f,0.0f, 3.0f));
	model = rotate(model, 90.0f, vec3(0.0f, 1.0f, 0.0f));
	model = scale(model, vec3(0.001f, 0.001f, 0.001f));



	//glBindVertexArray(cubeVAO);
	glBindVertexArray(vao[0]);

	//Declare your uniform variables that will be used in your shader
	int matrix_location;
	int view_mat_location;
	int proj_mat_location ;

	int light_colour;


	vec3 light(1.0f, 1.0f, 1.0f);

	vec3 viewPos = camera.Position;

	GLuint shader = shaders["multilight"];

	glUseProgram(shader);
	matrix_location = glGetUniformLocation(shader, "model");
	view_mat_location = glGetUniformLocation(shader, "view");
	proj_mat_location = glGetUniformLocation(shader, "projection");
	int view_pos = glGetUniformLocation(shader, "viewPos");

	// get material locations
	int diffuse_loc = glGetUniformLocation(shader, "material.diffuse");
	int specular_loc = glGetUniformLocation(shader, "material.specular");
	int shine_loc = glGetUniformLocation(shader, "material.shininess");

	//// set material values
	vec3 ambient(1.0f, 0.5f, 0.31f);
	vec3 diff(1.0f, 0.5f, 0.31f);
	vec3 specular(0.5f, 0.5f, 0.5f);
	float shininess = 64.0f;

	glUniform1i(diffuse_loc, 0);
	glUniform1i(specular_loc, 1);
	//
	set_float(shader, "material.shininess", shininess);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuseMap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, specularMap);


	//glUniform1f(shine_loc, shininess);

	//// set light values
	//vec3 light_amb(0.1f, 0.1f, 0.1f);
	//vec3 light_diff(0.5f, 0.5f, 0.5f);
	//vec3 dir(-0.2f, -1.0f, -0.3f);

	//float constant = 1.0f;
	//float linear = 0.09f;
	//float quad = 0.032f;
	//float bounds = cos(radians(12.5f));
	//float outerBounds = cos(radians(17.5f));

	//
	//set_vec3(shader, "light.ambient", light_amb);
	//set_vec3(shader, "light.diffuse", light_diff);
	//set_vec3(shader, "light.specular", light);
	//set_vec3(shader, "light.position", lightPos);
	//set_vec3(shader, "light.direction", dir);

	//set_float(shader, "light.constant", constant);
	//set_float(shader, "light.linear", linear);
	//set_float(shader, "light.quadratic", quad);
	//set_float(shader, "light.bounds", bounds);
	//set_float(shader, "light.outerBounds", outerBounds);

	multi_light(shader);

	vec3 obj_color(1.0f, 0.5f, 0.31f);

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, value_ptr(model));
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, value_ptr(view));



	glUniform3fv(view_pos, 1, value_ptr(viewPos));

	glDrawArrays(GL_TRIANGLES, 0, mesh_data[0].mPointCount);
	//glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindVertexArray(vao[1]);
	mat4 childModel(1.0f);
	childModel = translate(childModel, vec3(-170.0f, -10.0f, -500.0f));
	childModel = model * childModel;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, value_ptr(childModel));
	glDrawArrays(GL_TRIANGLES, 0, mesh_data[1].mPointCount);


	// draw light source
	glBindVertexArray(lightVAO);

	shader = shaders["lamp"];
	glUseProgram(shader);

	view_mat_location = glGetUniformLocation(shader, "view");
	proj_mat_location = glGetUniformLocation(shader, "projection");
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, value_ptr(view));
	
	for (unsigned int i = 0; i < 1; i++)
	{
		model = glm::mat4(1.0f);
		model = glm::translate(model, pointLightPositions[i]);
		model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
		set_mat4(shader, "model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	// draw skybox as last
	glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
	shader = shaders["skybox"];
	glUseProgram(shader);


	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	view = glm::mat4(glm::mat3(camera.GetViewMatrix()));

	view_mat_location = glGetUniformLocation(shader, "view");
	proj_mat_location = glGetUniformLocation(shader, "projection");
	int skybox_loc = glGetUniformLocation(shader, "skybox");

	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, value_ptr(persp_proj));
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, value_ptr(view));
	glUniform1i(skybox_loc, 0);

	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS); // set depth function back to default

	glutSwapBuffers();
}



void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	// Rotate the model slowly around the y axis at 20 degrees per second
	rotate_y += 20.0f * delta;
	rotate_y = fmodf(rotate_y, 360.0f);
	// Draw the next frame
	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
	if (firstMouse)
	{
		lastX = x;
		lastY = y;
		firstMouse = false;
	}

	float xoffset = x - lastX;
	float yoffset = lastY - y; // reversed since y-coordinates go from bottom to top
	switch (key) {
		case 'q':
			camera.ProcessKeyboard(UP, delta);
			break;

		case 'w':
			camera.ProcessKeyboard(FORWARD, delta);
			break;
		
		case 's':
			camera.ProcessKeyboard(BACKWARD, delta);
			break;

		case 'a':
			camera.ProcessKeyboard(LEFT, delta);
			break;


		case 'd':
			camera.ProcessKeyboard(RIGHT, delta);
			break;

		case 'e':
			camera.ProcessKeyboard(DOWN, delta);
			break;

		case 'i':

			lastX = x;
			lastY = y;

			camera.ProcessMouseMovement(xoffset, yoffset);
			break;

		case 'r':
			move_vec.x = 0.0f;
			move_vec.y = 0.0f;
			move_vec.z = 0.0f;
			trans.x = 0.0f;
			trans.y = 0.0f;
			trans.z = 0.0f;
			break;

		case '8':
			trans.y += delta * 2;
			break;

		case '2':
			trans.y -= delta * 2;
			break;

		case '4':
			trans.x -= delta * 2;
			break;

		case '6':
			trans.x += delta * 2;
			break;

		case '7':
			trans.z -= delta * 2;
			break;

		case '9':
			trans.z += delta * 2;
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
	vector<string> faces
	{
			"C:\\Users\\Dylan\\Documents\\Graphics\\Lab 04 - Sample Object Hierarchy\\build\\executables\\Debug\\sor_hills\\hills_lf.JPG",
			"C:\\Users\\Dylan\\Documents\\Graphics\\Lab 04 - Sample Object Hierarchy\\build\\executables\\Debug\\sor_hills\\hills_rt.JPG",
			"C:\\Users\\Dylan\\Documents\\Graphics\\Lab 04 - Sample Object Hierarchy\\build\\executables\\Debug\\sor_hills\\hills_up.JPG",
			"C:\\Users\\Dylan\\Documents\\Graphics\\Lab 04 - Sample Object Hierarchy\\build\\executables\\Debug\\sor_hills\\hills_dn.JPG",
			"C:\\Users\\Dylan\\Documents\\Graphics\\Lab 04 - Sample Object Hierarchy\\build\\executables\\Debug\\sor_hills\\hills_ft.JPG",
			"C:\\Users\\Dylan\\Documents\\Graphics\\Lab 04 - Sample Object Hierarchy\\build\\executables\\Debug\\sor_hills\\hills_bk.JPG"
	};
	diffuseMap = loadTexture("C:\\Users\\Dylan\\Documents\\Graphics\\Lab 04 - Sample Object Hierarchy\\hat.jpg");
	specularMap = loadTexture("C:\\Users\\Dylan\\Documents\\Graphics\\Lab 04 - Sample Object Hierarchy\\container2.jpg");
	skybox = loadCubemap(faces);
	//root.createChild(left_child);

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
