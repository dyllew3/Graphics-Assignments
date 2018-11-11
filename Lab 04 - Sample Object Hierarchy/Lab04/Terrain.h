
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

#include <iostream>
#include <stdlib.h>


#include <GL/glut.h>

#include"imageloader.h"

using namespace std;
using namespace glm;

//Represents a terrain, by storing a set of heights and normals at 2D locations
class Terrain {
private:
	int w; //Width
	int l; //Length
	float** hs; //Heights
	vec3** normals;
	bool computedNormals; //Whether normals is up-to-date
public:
	Terrain(int w2, int l2) {
		w = w2;
		l = l2;

		hs = new float*[l];
		for (int i = 0; i < l; i++) {
			hs[i] = new float[w];
		}

		normals = new vec3*[l];
		for (int i = 0; i < l; i++) {
			normals[i] = new vec3[w];
		}

		computedNormals = false;
	}

	~Terrain() {
		for (int i = 0; i < l; i++) {
			delete[] hs[i];
		}
		delete[] hs;

		for (int i = 0; i < l; i++) {
			delete[] normals[i];
		}
		delete[] normals;
	}

	int width() {
		return w;
	}

	int length() {
		return l;
	}

	//Sets the height at (x, z) to y
	void setHeight(int x, int z, float y) {
		hs[z][x] = y;
		computedNormals = false;
	}

	//Returns the height at (x, z)
	float getHeight(int x, int z) {
		return hs[z][x];
	}

	//Computes the normals, if they haven't been computed yet
	void computeNormals() {
		if (computedNormals) {
			return;
		}

		//Compute the rough version of the normals
		vec3** normals2 = new vec3*[l];
		for (int i = 0; i < l; i++) {
			normals2[i] = new vec3[w];
		}

		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				vec3 sum(0.0f, 0.0f, 0.0f);

				vec3 out;
				if (z > 0) {
					out = vec3(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
				}
				vec3 in;
				if (z < l - 1) {
					in = vec3(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
				}
				vec3 left;
				if (x > 0) {
					left = vec3(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
				}
				vec3 right;
				if (x < w - 1) {
					right = vec3(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
				}

				if (x > 0 && z > 0) {
					sum += normalize(cross(left, out));
				}
				if (x > 0 && z < l - 1) {
					sum += normalize(cross(left,in));
				}
				if (x < w - 1 && z < l - 1) {
					sum += normalize(cross(in,right));
				}
				if (x < w - 1 && z > 0) {
					sum += normalize(cross(right,out));
				}

				normals2[z][x] = sum;
			}
		}

		//Smooth out the normals
		const float FALLOUT_RATIO = 0.5f;
		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				vec3 sum = normals2[z][x];

				if (x > 0) {
					sum += normals2[z][x - 1] * FALLOUT_RATIO;
				}
				if (x < w - 1) {
					sum += normals2[z][x + 1] * FALLOUT_RATIO;
				}
				if (z > 0) {
					sum += normals2[z - 1][x] * FALLOUT_RATIO;
				}
				if (z < l - 1) {
					sum += normals2[z + 1][x] * FALLOUT_RATIO;
				}

				if (sum.length() == 0) {
					sum = vec3(0.0f, 1.0f, 0.0f);
				}
				normals[z][x] = sum;
			}
		}

		for (int i = 0; i < l; i++) {
			delete[] normals2[i];
		}
		delete[] normals2;

		computedNormals = true;
	}

	//Returns the normal at (x, z)
	vec3 getNormal(int x, int z) {
		if (!computedNormals) {
			computeNormals();
		}
		return normals[z][x];
	}
};

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			unsigned char color =
				(unsigned char)image->pixels[3 * (y * image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}

	delete image;
	t->computeNormals();
	return t;
}

float _angle = 60.0f;
Terrain* _terrain;

//void cleanup() {
//	delete _terrain;
//}
//
//void handleKeypress(unsigned char key, int x, int y) {
//	switch (key) {
//	case 27: //Escape key
//		cleanup();
//		exit(0);
//	}
//}
//
//void initRendering() {
//	glEnable(GL_DEPTH_TEST);
//	glEnable(GL_COLOR_MATERIAL);
//	glEnable(GL_LIGHTING);
//	glEnable(GL_LIGHT0);
//	glEnable(GL_NORMALIZE);
//	glShadeModel(GL_SMOOTH);
//}
//
//void handleResize(int w, int h) {
//	glViewport(0, 0, w, h);
//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
//	gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);
//}
//
//void drawScene() {
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();
//	glTranslatef(0.0f, 0.0f, -10.0f);
//	glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
//	glRotatef(-_angle, 0.0f, 1.0f, 0.0f);
//
//	GLfloat ambientColor[] = { 0.4f, 0.4f, 0.4f, 1.0f };
//	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);
//
//	GLfloat lightColor0[] = { 0.6f, 0.6f, 0.6f, 1.0f };
//	GLfloat lightPos0[] = { -0.5f, 0.8f, 0.1f, 0.0f };
//	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
//	glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
//
//	float scale = 5.0f / glm::max(_terrain->width() - 1, _terrain->length() - 1);
//	glScalef(scale, scale, scale);
//	glTranslatef(-(float)(_terrain->width() - 1) / 2,
//		0.0f,
//		-(float)(_terrain->length() - 1) / 2);
//
//	glColor3f(0.3f, 0.9f, 0.0f);
//	for (int z = 0; z < _terrain->length() - 1; z++) {
//		//Makes OpenGL draw a triangle at every three consecutive vertices
//		glBegin(GL_TRIANGLE_STRIP);
//		for (int x = 0; x < _terrain->width(); x++) {
//			vec3 normal = _terrain->getNormal(x, z);
//			glNormal3f(normal[0], normal[1], normal[2]);
//			glVertex3f(x, _terrain->getHeight(x, z), z);
//			normal = _terrain->getNormal(x, z + 1);
//			glNormal3f(normal[0], normal[1], normal[2]);
//			glVertex3f(x, _terrain->getHeight(x, z + 1), z + 1);
//		}
//		glEnd();
//	}
//
//	glutSwapBuffers();
//}
//
//void update(int value) {
//	_angle += 1.0f;
//	if (_angle > 360) {
//		_angle -= 360;
//	}
//
//	glutPostRedisplay();
//	glutTimerFunc(25, update, 0);
//}
//
//int main(int argc, char** argv) {
//	glutInit(&argc, argv);
//	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
//	glutInitWindowSize(400, 400);
//
//	glutCreateWindow("Terrain - videotutorialsrock.com");
//	initRendering();
//
//	_terrain = loadTerrain("heightmap.bmp", 20);
//
//	glutDisplayFunc(drawScene);
//	glutKeyboardFunc(handleKeypress);
//	glutReshapeFunc(handleResize);
//	glutTimerFunc(25, update, 0);
//
//	glutMainLoop();
//	return 0;
//}
//








