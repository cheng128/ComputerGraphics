#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

bool isDrawWireframe = false;
bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	ViewCenter = 3,
	ViewEye = 4,
	ViewUp = 5,
};

GLint iLocMVP;

vector<string> filenames; // .obj filename list

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;


typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	int materialId;
	int indexCount;
	GLuint m_texture;
} Shape;
Shape quad;
Shape m_shape;
vector<Shape> m_shape_list;
int cur_idx = 0; // represent which model should be rendered now

static GLvoid Normalize(GLfloat v[3])
{
    GLfloat l;

    l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
}


static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

    n[0] = u[1] * v[2] - u[2] * v[1];
    n[1] = u[2] * v[0] - u[0] * v[2];
    n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(1.0f, 0.0f, 0.0f, vec.x,
                  0.0f, 1.0f, 0.0f, vec.y,
                  0.0f, 0.0f, 1.0f, vec.z,
                  0.0f, 0.0f, 0.0f, 1.0f);

	return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(vec.x, 0.0f, 0.0f, 0.0f,
                  0.0f, vec.y, 0.0f, 0.0f,
                  0.0f, 0.0f, vec.z, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);

	return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, cosf(val), -sinf(val), 0.0f,
                  0.0f, sinf(val), cos(val), 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(cosf(val), 0.0f, sinf(val), 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                  -sinf(val), 0.0f, cosf(val), 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(cosf(val), -sinf(val), 0.0f, 0.0f,
                  sinf(val), cosf(val), 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);

	return mat;
}

// multiply three rotation matrix
Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
// note: transforms world coordinates to view space
void setViewingMatrix()
{
	// view_matrix[...] = ...
    
    Vector3 f = main_camera.center - main_camera.position; //f
    GLfloat fp[] = {f.x, f.y, f.z}; //f'
    Normalize(fp);

    Vector3 u = main_camera.up_vector;
    GLfloat up[] = {u.x, u.y, u.z};
    Normalize(up); //u'
    
    GLfloat s[3];
    Cross(fp, up, s); // s

    GLfloat upp[3];  // upp
    Cross(s, fp, upp);
    
    Matrix4 R;
    R = Matrix4(s[0], s[1], s[2], 0.0f,
                 upp[0], upp[1], upp[2], 0.0f,
                 -fp[0], -fp[1], -fp[2], 0.0f,
                 0.0f, 0.0f, 0.0f, 1.0f);
    
    Matrix4 T;
    T = Matrix4(1.0f, 0.0f, 0.0f, -main_camera.position.x,
                  0.0f, 1.0f, 0.0f, -main_camera.position.y,
                  0.0f, 0.0f, 1.0f, -main_camera.position.z,
                  0.0f, 0.0f, 0.0f, 1.0f);
    
    view_matrix = R * T;
}

// [TODO] compute orthogonal projection matrix
// note: transforms view coordinates to clip space
void setOrthogonal()
{
	cur_proj_mode = Orthogonal;
	// project_matrix [...] = ...
    GLfloat X_0 = 2.0f / (proj.right - proj.left);
    GLfloat Y_1 = 2.0f / (proj.top - proj.bottom);
    GLfloat Z_2 = -2.0f / (proj.farClip - proj.nearClip);
    project_matrix = Matrix4(X_0, 0.0f, 0.0f, - (proj.right + proj.left) / (proj.right - proj.left),
                             0.0f, Y_1, 0.0f, - (proj.top + proj.bottom) / (proj.top - proj.bottom),
                             0.0f, 0.0f, Z_2, - (proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip),
                             0.0f, 0.0f, 0.0f, 1.0f);
}

// [TODO] compute persepective projection matrix
// note: transforms view coordinates to clip space
void setPerspective()
{
	cur_proj_mode = Perspective;
	// project_matrix [...] = ...
    GLfloat f = tanf((proj.fovy / 2.0) / 180.0 * acosf(-1.0));
    GLfloat Z_2 = (proj.farClip + proj.nearClip)/(proj.nearClip - proj.farClip);
    GLfloat Z_3 = (2 * proj.farClip * proj.nearClip) / (proj.nearClip - proj.farClip);
    project_matrix = Matrix4(f/proj.aspect, 0, 0, 0,
                             0, f, 0, 0,
                             0, 0, Z_2, Z_3,
                             0, 0, -1, 0);
}


// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	// [TODO] change your aspect ratio in both perspective and orthogonal view
    proj.aspect = (float) width / (float) height;
    if (cur_proj_mode == Perspective) {
        setPerspective();
    } else if (cur_proj_mode == Orthogonal){
        proj.left = -proj.aspect;
        proj.right = proj.aspect;
        setOrthogonal();
    }
}

void drawPlane()
{
	GLfloat vertices[18]{ 1.0, -0.9, -1.0,
		1.0, -0.9,  1.0,
		-1.0, -0.9, -1.0,
		1.0, -0.9,  1.0,
		-1.0, -0.9,  1.0,
		-1.0, -0.9, -1.0 };

	GLfloat colors[18]{ 0.0,1.0,0.0,
		0.0,0.5,0.8,
		0.0,1.0,0.0,
		0.0,0.5,0.8,
		0.0,0.5,0.8,
		0.0,1.0,0.0 };

	// [TODO] draw the plane with above vertices and color
	Matrix4 MVP = project_matrix * view_matrix;
	GLfloat mvp[16];
	// [TODO] multiply all the matrix
	// [TODO] row-major ---> column-major
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];

	//please refer to LoadModels function
	//glGenVertexArrays..., glBindVertexArray...
	//glGenBuffers..., glBindBuffer..., glBufferData...
    // note: Based on previous SOP to draw the plane
    
    Shape plane;
    glGenVertexArrays(1, &plane.vao);
    glBindVertexArray(plane.vao);

    glGenBuffers(1, &plane.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, plane.vbo);
    glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(GL_FLOAT), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    plane.vertex_count = 18 / 3;

    glGenBuffers(1, &plane.p_color);
    glBindBuffer(GL_ARRAY_BUFFER, plane.p_color);
    glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(GL_FLOAT), colors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    
    glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
    glBindVertexArray(plane.vao);
    glDrawArrays(GL_TRIANGLES, 0, plane.vertex_count);
}


// Render function for display rendering
void RenderScene(void) {
    
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Matrix4 T, R, S;
	// [TODO] update translation, rotation and scaling
    // note: rotation on every model will be independent
    T = translate(models[cur_idx].position);
    R = rotate(models[cur_idx].rotation);
    S = scaling(models[cur_idx].scale);
    
	Matrix4 MVP;
	GLfloat mvp[16];

	// [TODO] multiply all the matrix
    MVP = project_matrix * view_matrix * T * R * S;
    
	// [TODO] row-major ---> column-major
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];
	// use uniform to send mvp to vertex shader
	// [TODO] draw 3D model in solid or in wireframe mode here, and draw plane
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
	glBindVertexArray(m_shape_list[cur_idx].vao);
    glPolygonMode(GL_FRONT_AND_BACK, !isDrawWireframe ? GL_FILL : GL_LINE);
	glDrawArrays(GL_TRIANGLES, 0, m_shape_list[cur_idx].vertex_count);
	glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	drawPlane();
}

void print_info(){
    cout << "Model: " << cur_idx << endl;
    cout << "Trans Mode: " << cur_trans_mode << endl;
    cout << "Translation Matrix:\n" << translate(models[cur_idx].position)<<endl;
    cout << "Rotation Matrix:\n" << rotate(models[cur_idx].rotation) << endl;
    cout << "Scaling Matrix:\n" << scaling(models[cur_idx].scale) << endl;
    cout << "Viewing Matrix:\n" << view_matrix << endl;
    cout << "Projection Matrix:\n" << project_matrix << endl;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard
    if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        cur_idx = (cur_idx + 1) % models.size();
        isDrawWireframe = false;
    } else if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        cur_idx = (cur_idx - 1 + 5) % 5;
        isDrawWireframe = false;
    } else if (key == GLFW_KEY_W && action == GLFW_PRESS) {
        isDrawWireframe = !isDrawWireframe;
    } else if (key == GLFW_KEY_O && action == GLFW_PRESS) {
        setOrthogonal();
    } else if (key == GLFW_KEY_P && action == GLFW_PRESS){
        setPerspective();
    } else if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        cur_trans_mode = GeoTranslation;
    } else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        cur_trans_mode = GeoScaling;
    } else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        cur_trans_mode = GeoRotation;
    } else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        cur_trans_mode = ViewEye;
    } else if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        cur_trans_mode = ViewCenter;
    } else if (key == GLFW_KEY_U && action == GLFW_PRESS) {
        cur_trans_mode = ViewUp;
    } else if (key == GLFW_KEY_I && action == GLFW_PRESS) {
        print_info();
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
    if (cur_trans_mode == GeoTranslation) {
        models[cur_idx].position += Vector3(0.0f, 0.0f, yoffset);
    } else if (cur_trans_mode == GeoScaling) {
        models[cur_idx].scale += Vector3(0.0f, 0.0f, yoffset);
    } else if (cur_trans_mode == GeoRotation) {
        models[cur_idx].rotation += Vector3(0.0f, 0.0f, yoffset);
    } else if (cur_trans_mode == ViewEye) {
        main_camera.position += Vector3(0.0f, 0.0f, yoffset);
        setViewingMatrix();
    } else if (cur_trans_mode == ViewCenter) {
        main_camera.center += Vector3(0.0f, 0.0f, yoffset);
        setViewingMatrix();
    } else if (cur_trans_mode == ViewUp) {
        main_camera.up_vector += Vector3(0.0f, 0.0f, yoffset);
        setViewingMatrix();
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// [TODO] Call back function for mouse
    if (action == GLFW_PRESS) {
        mouse_pressed = true;
    } else if (action == GLFW_RELEASE) {
        mouse_pressed = false;
    }
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] Call back function for cursor position
    double diff_x = xpos - starting_press_x;
    double diff_y = ypos - starting_press_y;
    starting_press_x = xpos;
    starting_press_y = ypos;

    if (mouse_pressed) {
        switch(cur_trans_mode) {
            case GeoTranslation:
                models[cur_idx].position += Vector3(diff_x / WINDOW_WIDTH, -diff_y / WINDOW_HEIGHT, 0);
                break;
            case GeoRotation:
                models[cur_idx].rotation -= Vector3(diff_y, diff_x, 0)/150;
                break;
            case GeoScaling:
                models[cur_idx].scale += Vector3(diff_x / WINDOW_WIDTH, - diff_y / WINDOW_HEIGHT, 0);
                break;
            case ViewEye:
                main_camera.position += Vector3(diff_x / WINDOW_WIDTH, - diff_y / WINDOW_HEIGHT, 0);
                setViewingMatrix();
                break;
            case ViewCenter:
                main_camera.center += Vector3(- diff_x / WINDOW_WIDTH,  diff_y / WINDOW_HEIGHT, 0);
                setViewingMatrix();
                break;
            case ViewUp:
                main_camera.up_vector += Vector3(diff_x / WINDOW_WIDTH, - diff_y / WINDOW_HEIGHT, 0);
                setViewingMatrix();
                break;
        }
    }

}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p,f);
	glAttachShader(p,v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);
    
    // note: get the uniform variable location in a shader
	iLocMVP = glGetUniformLocation(p, "mvp");

	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i)/ scale;
	}
	size_t index_offset = 0;
	vertices.reserve(shape->mesh.num_face_vertices.size() * 3);
	colors.reserve(shape->mesh.num_face_vertices.size() * 3);
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
		}
		index_offset += fv;
	}
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;

	string err;
	string warn;
    
    // note: according to the model_path to read the shape and materials
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Maerial size %d\n", shapes.size(), materials.size());
	
    // note: between -1 to 1
	normalization(&attrib, vertices, colors, &shapes[0]);

	Shape tmp_shape;
	glGenVertexArrays(1, &tmp_shape.vao);
	glBindVertexArray(tmp_shape.vao);

	glGenBuffers(1, &tmp_shape.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	tmp_shape.vertex_count = vertices.size() / 3;

	glGenBuffers(1, &tmp_shape.p_color);
	glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	m_shape_list.push_back(tmp_shape);
	model tmp_model;
	models.push_back(tmp_model);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	shapes.clear();
	materials.clear();
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders (note: do not need to modify this part)
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
    
    // note: five models
	vector<string> model_list{ "../ColorModels/bunny5KC.obj", "../ColorModels/dragon10KC.obj", "../ColorModels/lucy25KC.obj", "../ColorModels/teapot4KC.obj", "../ColorModels/dolphinC.obj"};
    
	// [TODO] Load five model at here
    for (int i=0; i<5; i++){
        LoadModels(model_list[i]);
    }
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "109062633 HW1", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
	// register glfw callback functions (note:implement these functions)
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event (note: keys pressed/released, mouse moved etc.)
        glfwPollEvents();
    }
	
	// just for compatibiliy purposes
	return 0;
}
