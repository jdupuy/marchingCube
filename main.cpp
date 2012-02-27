////////////////////////////////////////////////////////////////////////////////
// \author   Jonathan Dupuy
//
////////////////////////////////////////////////////////////////////////////////

// enable gui
#define _ANT_ENABLE

// GL libraries
#include "glew.hpp"
#include "GL/freeglut.h"

#ifdef _ANT_ENABLE
#	include "AntTweakBar.h"
#endif // _ANT_ENABLE

// Standard librabries
#include <iostream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <cmath>

// Custom libraries
#include "Algebra.hpp"      // Basic algebra library
#include "Transform.hpp"    // Basic transformations
#include "Framework.hpp"    // utility classes/functions

#include "MarchingCubeTables.hpp" // tables of the marching cube


////////////////////////////////////////////////////////////////////////////////
// Global variables
//
////////////////////////////////////////////////////////////////////////////////

// Constants
const float PI   = 3.14159265;
const float FOVY = PI*0.25f;

enum {
	// buffers
	BUFFER_CUBE_VERTICES = 0,
	BUFFER_CUBE_INDEXES,
	BUFFER_CASE_TO_FACE_COUNT,
	BUFFER_EDGE_CONNECT_LIST,
	BUFFER_COUNT,

	// vertex arrays
	VERTEX_ARRAY_CUBE = 0,
	VERTEX_ARRAY_EMPTY,
	VERTEX_ARRAY_COUNT,

	// textures
	TEXTURE_EDGE_CONNECT_LIST = 0,
	TEXTURE_COUNT,

	// programs
	PROGRAM_CUBE = 0,
	PROGRAM_MARCHING_CUBE,
	PROGRAM_COUNT
};

// OpenGL objects
GLuint *buffers      = NULL;
GLuint *vertexArrays = NULL;
GLuint *textures     = NULL;
GLuint *programs     = NULL;

// Tools
Affine cameraInvWorld       = Affine::Translation(Vector3(0,0,-2.5));
Projection cameraProjection = Projection::Perspective(FOVY,
                                                      1.0f,
                                                      0.1f,
                                                      40.0f);

bool mouseLeft  = false;
bool mouseRight = false;

GLfloat deltaTicks = 0.0f;
GLint marchingCubeCase = 0;

#ifdef _ANT_ENABLE
GLfloat speed = 0.0f; // app speed (in ms)
#endif


////////////////////////////////////////////////////////////////////////////////
// Functions
//
////////////////////////////////////////////////////////////////////////////////

// convert nvidia indexes to compressed index
static GLint voxel_edge_to_vertices(GLint edge) {
	GLint edges = 0;
	switch(edge) {
		case 0: edges =        0x1<<3; break;
		case 1: edges =  0x1 | 0x2<<3; break;
		case 2: edges =  0x2 | 0x3<<3; break;
		case 3: edges =        0x3<<3; break;
		case 4: edges =  0x4 | 0x5<<3; break;
		case 5: edges =  0x5 | 0x6<<3; break;
		case 6: edges =  0x6 | 0x7<<3; break;
		case 7: edges =  0x4 | 0x7<<3; break;
		case 8: edges =        0x4<<3; break;
		case 9: edges =  0x1 | 0x5<<3; break;
		case 10: edges = 0x2 | 0x6<<3; break;
		case 11: edges = 0x3 | 0x7<<3; break;
		default: break;
	}
	return edges;
}

#ifdef _ANT_ENABLE

static void TW_CALL toggle_fullscreen(void *data) {
	// toggle fullscreen
	glutFullScreenToggle();
}

#endif // _USE_GUI

////////////////////////////////////////////////////////////////////////////////
// on init cb
void on_init() {
//	GLint maxUniformBlockSize = 0;
//	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
//	std::cout << "GL_MAX_UNIFORM_BLOCK_SIZE : " << maxUniformBlockSize << std::endl;
	const GLfloat CUBE_VERTICES[] = { -0.5f, -0.5f,  0.5f, 1,   // 0 
	                                  -0.5f,  0.5f,  0.5f, 1,   // 1
	                                   0.5f,  0.5f,  0.5f, 1,   // 2
	                                   0.5f, -0.5f,  0.5f, 1,   // 3
	                                  -0.5f, -0.5f, -0.5f, 1,   // 4
	                                  -0.5f,  0.5f, -0.5f, 1,   // 5
	                                   0.5f,  0.5f, -0.5f, 1,   // 6
	                                   0.5f, -0.5f, -0.5f, 1 }; // 7
	const GLushort CUBE_INDEXES[] = { 2,1,1,0,0,3,     // front
	                                  6,2,2,3,3,7,     // right
	                                  5,6,6,7,7,4,     // back
	                                  1,5,5,4,4,0 };   // left
	GLint* edgeList = new GLint[2048];
	GLint compressedVertexIndex = 0;

	for(GLint i = 0; i<256*5*4; i+=4) {
		compressedVertexIndex = voxel_edge_to_vertices(EDGE_CONNECT_LIST[i]);
		compressedVertexIndex|= voxel_edge_to_vertices(EDGE_CONNECT_LIST[i+1])
		                      << 6;
		compressedVertexIndex|= voxel_edge_to_vertices(EDGE_CONNECT_LIST[i+2])
		                      << 12;
		// drop fourth component
		edgeList[i/4] = compressedVertexIndex;
	}

	// alloc names
	buffers      = new GLuint[BUFFER_COUNT];
	vertexArrays = new GLuint[VERTEX_ARRAY_COUNT];
	textures     = new GLuint[TEXTURE_COUNT];
	programs     = new GLuint[PROGRAM_COUNT];

	// gen names
	glGenBuffers(BUFFER_COUNT, buffers);
	glGenVertexArrays(VERTEX_ARRAY_COUNT, vertexArrays);
	glGenTextures(TEXTURE_COUNT, textures);
	for(GLuint i=0; i<PROGRAM_COUNT;++i)
		programs[i] = glCreateProgram();

	// configure buffers
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_CUBE_VERTICES]);
		glBufferData(GL_ARRAY_BUFFER,
		             sizeof(CUBE_VERTICES),
		             CUBE_VERTICES,
		             GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[BUFFER_CUBE_INDEXES]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		             sizeof(CUBE_INDEXES),
		             CUBE_INDEXES,
		             GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, buffers[BUFFER_CASE_TO_FACE_COUNT]);
		glBufferData(GL_UNIFORM_BUFFER,
		             sizeof(CASE_TO_FACE_COUNT),
		             CASE_TO_FACE_COUNT,
		             GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, buffers[BUFFER_EDGE_CONNECT_LIST]);
		glBufferData(GL_UNIFORM_BUFFER,
		             sizeof(GLint)*2048,
		             edgeList,
		             GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	delete[] edgeList;

	// bind bases
	glBindBufferBase(GL_UNIFORM_BUFFER,
	                 BUFFER_CASE_TO_FACE_COUNT,
	                 buffers[BUFFER_CASE_TO_FACE_COUNT]);
	glBindBufferBase(GL_UNIFORM_BUFFER,
	                 BUFFER_EDGE_CONNECT_LIST,
	                 buffers[BUFFER_EDGE_CONNECT_LIST]);

	// configure textures
//	glActiveTexture(GL_TEXTURE0 + TEXTURE_EDGE_CONNECT_LIST);
//	glBindTexture(GL_TEXTURE_BUFFER, textures[TEXTURE_EDGE_CONNECT_LIST]);
//		glTexBuffer(GL_TEXTURE_BUFFER,
//		            GL_R32I,
//		            buffers[BUFFER_EDGE_CONNECT_LIST]);

	// vertex arrays
	glBindVertexArray(vertexArrays[VERTEX_ARRAY_CUBE]);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_CUBE_VERTICES]);
		glVertexAttribPointer(0,4,GL_FLOAT,0,0,FW_BUFFER_OFFSET(0));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[BUFFER_CUBE_INDEXES]);
	glBindVertexArray(vertexArrays[VERTEX_ARRAY_EMPTY]);
	glBindVertexArray(0);

	// configure programs
	fw::build_glsl_program(programs[PROGRAM_CUBE],
	                       "cube.glsl",
	                       "",
	                       GL_TRUE);

	fw::build_glsl_program(programs[PROGRAM_MARCHING_CUBE],
	                       "marchingCube.glsl",
	                       "",
	                       GL_TRUE);
//	glProgramUniform1i(programs[PROGRAM_MARCHING_CUBE],
//	                   glGetUniformLocation(programs[PROGRAM_MARCHING_CUBE],
//	                                         "sEdgeConnectList"),
//	                   TEXTURE_EDGE_CONNECT_LIST);
	glUniformBlockBinding(programs[PROGRAM_MARCHING_CUBE],
	                      glGetUniformBlockIndex(programs[PROGRAM_MARCHING_CUBE],
	                                             "CaseToNumPolys"),
	                      BUFFER_CASE_TO_FACE_COUNT);
	glUniformBlockBinding(programs[PROGRAM_MARCHING_CUBE],
	                      glGetUniformBlockIndex(programs[PROGRAM_MARCHING_CUBE],
	                                             "EdgeConnectList"),
	                      BUFFER_EDGE_CONNECT_LIST);

	// set global state
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glClearColor(0.13,0.13,0.15,1.0);

#ifdef _ANT_ENABLE
	// start ant
	TwInit(TW_OPENGL, NULL);
	// send the ''glutGetModifers'' function pointer to AntTweakBar
	TwGLUTModifiersFunc(glutGetModifiers);

	// Create a new bar
	TwBar* menuBar = TwNewBar("menu");
	TwDefine("menu size='200 100'");

	TwAddButton(menuBar,
	            "fullscreen",
	            &toggle_fullscreen,
	            NULL,
	            "label='toggle fullscreen'");
	TwAddVarRO(menuBar,
	           "speed (ms)",
	           TW_TYPE_FLOAT,
	           &speed,
	           "");
	TwAddVarRW(menuBar,
	           "case",
	           TW_TYPE_INT32,
	           &marchingCubeCase,
	           "min=0 max=255 step=1");

#endif // _ANT_ENABLE
	fw::check_gl_error();
}


////////////////////////////////////////////////////////////////////////////////
// on clean cb
void on_clean() {
	// delete objects
	glDeleteBuffers(BUFFER_COUNT, buffers);
	glDeleteVertexArrays(VERTEX_ARRAY_COUNT, vertexArrays);
	glDeleteTextures(TEXTURE_COUNT, textures);
	for(GLuint i=0; i<PROGRAM_COUNT;++i)
		glDeleteProgram(programs[i]);

	// release memory
	delete[] buffers;
	delete[] vertexArrays;
	delete[] textures;
	delete[] programs;

#ifdef _ANT_ENABLE
	TwTerminate();
#endif // _ANT_ENABLE

	fw::check_gl_error();
}


////////////////////////////////////////////////////////////////////////////////
// on update cb
void on_update() {
	// Variables
	static fw::Timer deltaTimer;
	GLint windowWidth  = glutGet(GLUT_WINDOW_WIDTH);
	GLint windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

	// stop timing and set delta
	deltaTimer.Stop();
	deltaTicks = deltaTimer.Ticks();
#ifdef _ANT_ENABLE
	speed = deltaTicks*1000.0f;
#endif

	// update transformations
	Matrix4x4 mvp = cameraProjection.ExtractTransformMatrix()
	              * cameraInvWorld.ExtractTransformMatrix();

	// update uniforms
	glProgramUniformMatrix4fv(programs[PROGRAM_CUBE],
	                          glGetUniformLocation(programs[PROGRAM_CUBE],
	                                         "uModelViewProjection"),
	                          1,
	                          0,
	                          reinterpret_cast<float*>(&mvp));
	glProgramUniformMatrix4fv(programs[PROGRAM_MARCHING_CUBE],
	                          glGetUniformLocation(programs[PROGRAM_MARCHING_CUBE],
	                                         "uModelViewProjection"),
	                          1,
	                          0,
	                          reinterpret_cast<float*>(&mvp));
	glProgramUniform1i(programs[PROGRAM_MARCHING_CUBE],
	                   glGetUniformLocation(programs[PROGRAM_MARCHING_CUBE],
	                                         "uCase"),
	                   marchingCubeCase);

	// set viewport
	glViewport(0,0,windowWidth, windowHeight);

	// clear back buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// render the cube
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(programs[PROGRAM_CUBE]);
	glBindVertexArray(vertexArrays[VERTEX_ARRAY_CUBE]);
	glDrawElements(GL_LINES,
	               24,
	               GL_UNSIGNED_SHORT,
	               FW_BUFFER_OFFSET(0));

	// run marching cube
	glUseProgram(programs[PROGRAM_MARCHING_CUBE]);
	glBindVertexArray(vertexArrays[VERTEX_ARRAY_CUBE]); // hack
		glDrawArrays(GL_POINTS, 0, 1);

	glBindVertexArray(0);

#ifdef _ANT_ENABLE
	// back to default vertex array
	TwDraw();
#endif // _ANT_ENABLE

	fw::check_gl_error();

	// start ticking
	deltaTimer.Start();

	glutSwapBuffers();
	glutPostRedisplay();
}


////////////////////////////////////////////////////////////////////////////////
// on resize cb
void on_resize(GLint w, GLint h) {
#ifdef _ANT_ENABLE
	TwWindowSize(w, h);
#endif
	// update projection
	cameraProjection.FitWidthToAspect(float(w)/float(h));
}


////////////////////////////////////////////////////////////////////////////////
// on key down cb
void on_key_down(GLubyte key, GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1==TwEventKeyboardGLUT(key, x, y))
		return;
#endif
	if (key==27) // escape
		glutLeaveMainLoop();
	if(key=='f')
		glutFullScreenToggle();
	if(key=='p')
		fw::save_gl_front_buffer(0,
		                         0,
		                         glutGet(GLUT_WINDOW_WIDTH),
		                         glutGet(GLUT_WINDOW_HEIGHT));

}


////////////////////////////////////////////////////////////////////////////////
// on mouse button cb
void on_mouse_button(GLint button, GLint state, GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1 == TwEventMouseButtonGLUT(button, state, x, y))
		return;
#endif // _ANT_ENABLE
	if(state==GLUT_DOWN)
	{
		mouseLeft  |= button == GLUT_LEFT_BUTTON;
		mouseRight |= button == GLUT_RIGHT_BUTTON;
	}
	else
	{
		mouseLeft  &= button == GLUT_LEFT_BUTTON ? false : mouseLeft;
		mouseRight  &= button == GLUT_RIGHT_BUTTON ? false : mouseRight;
	}
	if(button == 3)
		cameraInvWorld.TranslateWorld(Vector3(0,0,0.15f));
	if(button == 4)
		cameraInvWorld.TranslateWorld(Vector3(0,0,-0.15f));
}


////////////////////////////////////////////////////////////////////////////////
// on mouse motion cb
void on_mouse_motion(GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1 == TwEventMouseMotionGLUT(x,y))
		return;
#endif // _ANT_ENABLE

	static GLint sMousePreviousX = 0;
	static GLint sMousePreviousY = 0;
	const GLint MOUSE_XREL = x-sMousePreviousX;
	const GLint MOUSE_YREL = y-sMousePreviousY;
	sMousePreviousX = x;
	sMousePreviousY = y;

	if(mouseLeft)
	{
		cameraInvWorld.RotateAboutWorldX(5.0f*MOUSE_YREL*deltaTicks);
		cameraInvWorld.RotateAboutLocalY(5.0f*MOUSE_XREL*deltaTicks);
	}
	if(mouseRight)
	{
		cameraInvWorld.TranslateWorld(deltaTicks*Vector3( 2.0f*MOUSE_XREL,
		                                                 -2.0f*MOUSE_YREL,
		                                                  0));
	}
}


////////////////////////////////////////////////////////////////////////////////
// on mouse wheel cb
void on_mouse_wheel(GLint wheel, GLint direction, GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1 == TwMouseWheel(wheel))
		return;
#endif // _ANT_ENABLE
	cameraInvWorld.TranslateWorld(Vector3(0,0,float(direction)*0.15f));
}


////////////////////////////////////////////////////////////////////////////////
// Main
//
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
	const GLuint CONTEXT_MAJOR = 4;
	const GLuint CONTEXT_MINOR = 1;

	// init glut
	glutInit(&argc, argv);
	glutInitContextVersion(CONTEXT_MAJOR ,CONTEXT_MINOR);
#ifdef _ANT_ENABLE
	glutInitContextFlags(GLUT_DEBUG);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
#else
	glutInitContextFlags(GLUT_DEBUG | GLUT_FORWARD_COMPATIBLE);
	glutInitContextProfile(GLUT_CORE_PROFILE);
#endif

	// build window
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("texture filtering");

	// init glew
	glewExperimental = GL_TRUE; // segfault on GenVertexArrays on Nvidia otherwise
	GLenum err = glewInit();
	if(GLEW_OK != err)
	{
		std::stringstream ss;
		ss << err;
		std::cerr << "glewInit() gave error " << ss.str() << std::endl;
		return 1;
	}

	// glewInit generates an INVALID_ENUM error for some reason...
	glGetError();

	// set callbacks
	glutCloseFunc(&on_clean);
	glutReshapeFunc(&on_resize);
	glutDisplayFunc(&on_update);
	glutKeyboardFunc(&on_key_down);
	glutMouseFunc(&on_mouse_button);
	glutPassiveMotionFunc(&on_mouse_motion);
	glutMotionFunc(&on_mouse_motion);
	glutMouseWheelFunc(&on_mouse_wheel);

	// run
	try
	{
		// run demo
		on_init();
		glutMainLoop();
	}
	catch(std::exception& e)
	{
		std::cerr << "Fatal exception: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}


