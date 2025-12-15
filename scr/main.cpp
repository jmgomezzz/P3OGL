#include "BOX.h"
#include "auxiliar.h"
#include "ModelLoader.h"
#include <vector>

#include <gl/glew.h>
#define SOLVE_FGLUT_WARNING
#include <gl/freeglut.h> 

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>


// === Datos que se almacenan en la memoria de la CPU ===

//Matrices
glm::mat4 proj = glm::mat4(1.0f);
glm::mat4 view = glm::mat4(1.0f);
glm::mat4 model = glm::mat4(1.0f);

//Variables de camara
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 6.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float fov = 60.0f;



// === Variables que nos dan acceso a Objetos OpenGL ===


//Shaders
unsigned int vshader;
unsigned int fshader;
unsigned int program;

//Variables Uniform
int uModelViewMat;
int uModelViewProjMat;
int uNormalMat;

//Atributos
int inPos;
int inColor;
int inNormal;
int inTexCoord;

//Texturas
unsigned int colorTexId;
unsigned int emiTexId;
unsigned int floorTexId;

//Texturas Uniform
int uColorTex;
int uEmiTex;
int uTexScale; //Escala de las coordenadas de textura

//VAO
unsigned int vao;

//VBOs que forman parte del objeto
unsigned int posVBO;
unsigned int colorVBO;
unsigned int normalVBO;
unsigned int texCoordVBO;
unsigned int triangleIndexVBO;

//UBOs
unsigned int uboLight; //ID del UBO
const unsigned int LIGHT_BINDING_INDEX = 0; //binding point

//Control de ratón
bool rightMouseDown = false;
int lastMouseX = 0;
int lastMouseY = 0;

//Structs
//Light data
struct lightData {
	glm::vec3 Ia; float pad0;
	glm::vec3 Id; float pad1;
	glm::vec3 Is; float pad2;
	glm::vec3 lpos;
};

lightData scenelight;

//Struct para varios objetos

struct Car {
	float radius;
	float speed;
	float angle;
	float driftAngle;
	float yOffset;
	float driftPhase;
};

std::vector<Car> cars; //Array de coches


//Variables para nuevo modelo
unsigned int modelVAO;
unsigned int modelVBOs[3]; //0: pos, 1: texCoord, 2: normal
std::vector<glm::vec3> m_vertices;
std::vector<glm::vec2> m_uvs;
std::vector<glm::vec3> m_normals;


// === Funciones auxiliares ===
//!!Por implementar

//Declaración de CB
void renderFunc();
void resizeFunc(int width, int height);
void timerFunc(int value); //Sustituye a IdleFunc para fijar fps
void keyboardFunc(unsigned char key, int x, int y);
void mouseFunc(int button, int state, int x, int y);
void motionFunc(int x, int y);
void mouseWheelFunc(int button, int direction, int x, int y);

// === Funciones de inicialización y destrucción ===
void initContext(int argc, char** argv);
void initOGL();
void initShader(const char* vname, const char* fname);
void initObj();
void destroy();
void initUBOs();


// === Carga el shader indicado, devuele el ID del shader ===
//!Por implementar
GLuint loadShader(const char* fileName, GLenum type);

//Crea una textura, la configura, la sube a OpenGL, 
//y devuelve el identificador de la textura 
//!!Por implementar
unsigned int loadTex(const char* fileName);






int main(int argc, char** argv)
{


	//Si lo pongo en español por alguna razón jode los números de donde deberian de estar los vertices y convierte los objetos que importo en literalmente una linea recta en el infinito :)
	//std::locale::global(std::locale("spanish"));// acentos ;)

	initContext(argc, argv);
	initOGL();
	initShader("../shaders_P3/shader.v1.vert", "../shaders_P3/shader.v1.frag");

	//UBOs
	initUBOs();

	initObj();

	glutMainLoop();

	destroy();

	return 0;
}


// === Funciones auxiliares ===
void initContext(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitContextVersion(3, 3);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	//glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(500, 500);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("Prácticas GLSL");

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		std::cout << "Error: " << glewGetErrorString(err) << std::endl;
		exit(-1);
	}
	const GLubyte* oglVersion = glGetString(GL_VERSION);
	std::cout << "This system supports OpenGL Version: " << oglVersion << std::endl;

	//Callbacks
	glutReshapeFunc(resizeFunc);
	glutDisplayFunc(renderFunc);
	//glutIdleFunc(idleFunc);
	glutTimerFunc(0, timerFunc, 0); //Sustituye a IdleFunc para fijar fps
	glutKeyboardFunc(keyboardFunc);
	glutMouseFunc(mouseFunc);
	glutMotionFunc(motionFunc);
	glutMouseWheelFunc(mouseWheelFunc);

}
void initOGL() {
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
	glDisable(GL_CULL_FACE);

	//glFrontFace(GL_CCW);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glEnable(GL_CULL_FACE);

	proj = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 1000.0f);
	view = glm::mat4(1.0f);
	view[3].z = -6; //inversa de la posición de la cámara (se mueve el mundo)



}
void destroy() {
	glDetachShader(program, vshader);
	glDetachShader(program, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);
	glDeleteProgram(program);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	if (inPos != -1) glDeleteBuffers(1, &posVBO);
	if (inColor != -1) glDeleteBuffers(1, &colorVBO);
	if (inNormal != -1) glDeleteBuffers(1, &normalVBO);
	if (inTexCoord != -1) glDeleteBuffers(1, &texCoordVBO);
	glDeleteBuffers(1, &triangleIndexVBO);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);

	glDeleteTextures(1, &colorTexId);
	glDeleteTextures(1, &emiTexId);
	glDeleteTextures(1, &floorTexId);
}
void initShader(const char* vname, const char* fname) {
	vshader = loadShader(vname, GL_VERTEX_SHADER);
	fshader = loadShader(fname, GL_FRAGMENT_SHADER);

	program = glCreateProgram();
	glAttachShader(program, vshader);
	glAttachShader(program, fshader);

	//Identificadores atributos
	glBindAttribLocation(program, 0, "inPos");
	glBindAttribLocation(program, 1, "inColor");
	glBindAttribLocation(program, 2, "inNormal");
	glBindAttribLocation(program, 3, "inTexCoord");

	glLinkProgram(program);

	int linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		//Calculamos una cadena de error
		GLint logLen;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
		char* logString = new char[logLen];
		glGetProgramInfoLog(program, logLen, NULL, logString);
		std::cout << "Error: " << logString << std::endl;
		delete logString;
		glDeleteProgram(program);
		program = 0;
		exit(-1);
	}

	//Identificadores variables uniformes
	uNormalMat = glGetUniformLocation(program, "normal");
	uModelViewMat = glGetUniformLocation(program, "modelView");
	uModelViewProjMat = glGetUniformLocation(program, "modelViewProj");

	//Texturas
	uColorTex = glGetUniformLocation(program, "colorTex");
	uEmiTex = glGetUniformLocation(program, "emiTex");
	uTexScale = glGetUniformLocation(program, "texScale");

	//CREAR identificadores de atributos 
	inPos = glGetAttribLocation(program, "inPos");
	inColor = glGetAttribLocation(program, "inColor");
	inNormal = glGetAttribLocation(program, "inNormal");
	inTexCoord = glGetAttribLocation(program, "inTexCoord");

	//Enlace light UBO
	GLuint lightBlockIndex = glGetUniformBlockIndex(program, "lightBlock"); //Obtenemos el índice del bloque
	if (lightBlockIndex == GL_INVALID_INDEX) {
		std::cerr << "No se ha encontrado el bloque de luces" << std::endl;
	}
	else {
		glUniformBlockBinding(program, lightBlockIndex, LIGHT_BINDING_INDEX);
	}

	//DEBUG PARA VERIFICAR QUE TODO ESTÁ BIEN
	std::cout << "inPos: " << inPos
		<< " inNormal: " << inNormal
		<< " inTexCoord: " << inTexCoord
		<< " inColor: " << inColor << std::endl;

	std::cout << "uModelView: " << uModelViewMat
		<< " uMVP: " << uModelViewProjMat
		<< " uNormal: " << uNormalMat << std::endl;


}
void initObj() {
	//Cargar nuevo modelo
	if (!loadOBJ("../models/cacharro.obj", m_vertices, m_uvs, m_normals)) {
		std::cerr << "Error cargando el modelo OBJ" << std::endl;
	}
	std::cout << "m_vertices: " << m_vertices.size()
		<< " m_uvs: " << m_uvs.size()
		<< " m_normals: " << m_normals.size() << std::endl;
	//--
	glm::vec3 mn(1e9f), mx(-1e9f);
	int bad = 0;
	for (auto& v : m_vertices) {
		if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) bad++;
		mn = glm::min(mn, v);
		mx = glm::max(mx, v);
	}
	std::cout << "BBox min: " << mn.x << "," << mn.y << "," << mn.z
		<< "  max: " << mx.x << "," << mx.y << "," << mx.z
		<< "  badVerts: " << bad << "\n";



	// 1. Si faltan normales, el objeto sería invisible por error de iluminación (NaN)
	if (m_normals.empty()) {
		std::cout << "AVISO: El modelo no tiene normales. Generando normales por defecto (Arriba)." << std::endl;
		// Rellenamos con un vector hacia arriba (0,1,0) para que la luz le afecte
		m_normals.resize(m_vertices.size(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	// 2. Si faltan UVs, la textura fallaría
	if (m_uvs.empty()) {
		std::cout << "AVISO: El modelo no tiene UVs. Generando UVs por defecto (0,0)." << std::endl;
		m_uvs.resize(m_vertices.size(), glm::vec2(0.0f, 0.0f));
	}


	//Confugurar el VAO
	glGenVertexArrays(1, &modelVAO);
	glBindVertexArray(modelVAO);
	glGenBuffers(3, modelVBOs);

	//Posiciones
	glBindBuffer(GL_ARRAY_BUFFER, modelVBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(glm::vec3), &m_vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(inPos, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(inPos);

	//UVs
	if (inTexCoord != -1 && !m_uvs.empty()) {
		glBindBuffer(GL_ARRAY_BUFFER, modelVBOs[1]);
		glBufferData(GL_ARRAY_BUFFER, m_uvs.size() * sizeof(glm::vec2), &m_uvs[0], GL_STATIC_DRAW);
		glVertexAttribPointer(inTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inTexCoord);
	}
	//Normales
	if (inNormal != -1 && !m_normals.empty()) {
		glBindBuffer(GL_ARRAY_BUFFER, modelVBOs[2]);
		glBufferData(GL_ARRAY_BUFFER, m_normals.size() * sizeof(glm::vec3), &m_normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer(inNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inNormal);
	}
	glBindVertexArray(0); // Desvincular


	//VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//Posiciones
	if (inPos != -1)
	{
		glGenBuffers(1, &posVBO);
		glBindBuffer(GL_ARRAY_BUFFER, posVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex * sizeof(float) * 3,
			cubeVertexPos, GL_STATIC_DRAW);
		glVertexAttribPointer(inPos, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inPos);
	}
	//Colores
	if (inColor != -1)
	{
		glGenBuffers(1, &colorVBO);
		glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex * sizeof(float) * 3,
			cubeVertexColor, GL_STATIC_DRAW);
		glVertexAttribPointer(inColor, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inColor);
	}
	//Normales
	if (inNormal != -1)
	{
		glGenBuffers(1, &normalVBO);
		glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex * sizeof(float) * 3,
			cubeVertexNormal, GL_STATIC_DRAW);
		glVertexAttribPointer(inNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inNormal);
	}
	//Texturas
	if (inTexCoord != -1)
	{
		glGenBuffers(1, &texCoordVBO);
		glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex * sizeof(float) * 2,
			cubeVertexTexCoord, GL_STATIC_DRAW);
		glVertexAttribPointer(inTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inTexCoord);
	}

	//Indices de los triángulos (Index Buffer)
	glGenBuffers(1, &triangleIndexVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleIndexVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		cubeNTriangleIndex * sizeof(unsigned int) * 3, cubeTriangleIndex,
		GL_STATIC_DRAW);

	//Model matrix
	model = glm::mat4(1.0f);

	//Dirección de las texturas
	colorTexId = loadTex("../img/Orange_Texture.png");
	emiTexId = loadTex("../img/emissive.png");
	floorTexId = loadTex("../img/suelo.jpg");

	//Inicializar coches
	cars.clear();
	// Coche 1
	cars.push_back({ 5.0f, 0.03f, 0.0f, 45.0f, -1.9f, 0.0f });

	// Coche 2
	cars.push_back({ 10.0f, 0.02f, 2.0f, 30.0f, -1.9f, 1.0f });

	// Coche 3
	cars.push_back({ 15.0f, 0.015f, 4.0f, 20.0f, -1.9f, 2.0f });

	// Coche 4
	cars.push_back({ 25.0f, 0.04f, 1.0f, 60.0f, -1.9f, 3.0f });
}

GLuint loadShader(const char* fileName, GLenum type) {
	unsigned int fileLen;
	char* source = loadStringFromFile(fileName, fileLen);
	//Creación y compilación del Shader
	GLuint shader;
	shader = glCreateShader(type);
	glShaderSource(shader, 1,
		(const GLchar**)&source, (const GLint*)&fileLen);
	glCompileShader(shader);
	delete source;

	//Comprobamos que se compiló bien
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		//Calculamos una cadena de error
		GLint logLen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);

		char* logString = new char[logLen];
		glGetShaderInfoLog(shader, logLen, NULL, logString);
		std::cout << "Error: " << logString << std::endl;
		delete logString;

		glDeleteShader(shader);
		exit(-1);
	}

	return shader;
}
unsigned int loadTex(const char* fileName) {

	unsigned char* map;
	unsigned int w, h;
	map = loadTexture(fileName, w, h);
	if (!map)
	{
		std::cout << "Error cargando el fichero: "
			<< fileName << std::endl;
		exit(-1);
	}

	unsigned int texId;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, (GLvoid*)map);

	delete[] map;

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); //Cambiamos a GL_REPEAT para permitir repetición (tiling)
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); //Cambiamos a GL_REPEAT para permitir repetición (tiling)

	return texId;
}

void renderFunc() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(program);
	//View matrix
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

	// === CORRECCIÓN DE LUZ: Transformar posición de luz a View Space ===
	// Creamos una copia temporal para no modificar la posición global del mundo
	lightData lightView = scenelight;
	// Multiplicamos la posición del mundo por la matriz de vista
	glm::vec4 lposV = view * glm::vec4(scenelight.lpos, 1.0f);
	lightView.lpos = glm::vec3(lposV);

	// Subimos la luz transformada a la GPU
	glBindBuffer(GL_UNIFORM_BUFFER, uboLight);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lightData), &lightView);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	// ===================================================================

	//Nuevo modelo
	glm::mat4 modelView = view * model;
	glm::mat4 modelViewProj = proj * view * model;
	glm::mat4 normalMat = glm::transpose(glm::inverse(modelView));

	if (uModelViewMat != -1) glUniformMatrix4fv(uModelViewMat, 1, GL_FALSE, &(modelView[0][0]));
	if (uModelViewProjMat != -1) glUniformMatrix4fv(uModelViewProjMat, 1, GL_FALSE, &(modelViewProj[0][0]));
	if (uNormalMat != -1)  glUniformMatrix4fv(uNormalMat, 1, GL_FALSE, &normalMat[0][0]);


	//Texturas del modelo
	if (uColorTex != -1) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorTexId);
		glUniform1i(uColorTex, 0);
	}
	if (uEmiTex != -1) {
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	if (uTexScale != -1) glUniform1f(uTexScale, 1.0f);

	// Usar el VAO del coche
	glBindVertexArray(modelVAO);

	// === BUCLE PARA DIBUJAR CADA COCHE ===
	for (const auto& car : cars) {
		glm::mat4 carModel = glm::mat4(1.0f);

		//Drift
		float driftNowDeg = car.driftAngle * sin(car.driftPhase);   // driftAngle = máximo en grados

		//Posición
		float x = cos(car.angle) * car.radius;
		float z = sin(car.angle) * car.radius;

		//Patinada
		glm::vec3 tangent(-sin(car.angle), 0.0f, cos(car.angle));   // dirección de movimiento
		float lateral = sin(glm::radians(driftNowDeg)) * 0.7f;      // ajusta 0.2 .. 1.5
		x += tangent.x * lateral;
		z += tangent.z * lateral;

		//Translacion
		carModel = glm::translate(carModel, glm::vec3(x, car.yOffset, z));

		//Rotación
		float rotation = -car.angle + glm::radians(180.0f) + glm::radians(driftNowDeg);
		carModel = glm::rotate(carModel, rotation, glm::vec3(0.0f, 1.0f, 0.0f));;

		// Calcular matrices derivadas
		glm::mat4 carModelView = view * carModel;
		glm::mat4 carModelViewProj = proj * carModelView;  // <-- ESTE ES EL MVP DEL COCHE
		glm::mat4 carNormalMat = glm::transpose(glm::inverse(carModelView));

		if (uModelViewMat != -1)
			glUniformMatrix4fv(uModelViewMat, 1, GL_FALSE, &carModelView[0][0]);

		if (uModelViewProjMat != -1)
			glUniformMatrix4fv(uModelViewProjMat, 1, GL_FALSE, &carModelViewProj[0][0]);  // <-- AQUÍ LO MANDAS

		if (uNormalMat != -1)
			glUniformMatrix4fv(uNormalMat, 1, GL_FALSE, &carNormalMat[0][0]);

		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertices.size());
	}

	//SUELO
	glm::mat4 modelFloor = glm::mat4(1.0f);
	modelFloor = glm::translate(modelFloor, glm::vec3(0.0f, -2.0f, 0.0f));
	modelFloor = glm::scale(modelFloor, glm::vec3(500.0f, 0.1f, 500.0f));

	glm::mat4 floorModelView = view * modelFloor;
	glm::mat4 floorModelViewProj = proj * view * modelFloor;
	glm::mat4 floorNormalMat = glm::transpose(glm::inverse(floorModelView));

	if (uModelViewMat != -1) glUniformMatrix4fv(uModelViewMat, 1, GL_FALSE, &(floorModelView[0][0]));
	if (uModelViewProjMat != -1) glUniformMatrix4fv(uModelViewProjMat, 1, GL_FALSE, &(floorModelViewProj[0][0]));
	if (uNormalMat != -1) glUniformMatrix4fv(uNormalMat, 1, GL_FALSE, &floorNormalMat[0][0]);

	if (uTexScale != -1) glUniform1f(uTexScale, 1000.0f);

	if (uColorTex != -1) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, floorTexId);
		glUniform1i(uColorTex, 0);
	}

	// USAR EL VAO DEL CUBO
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, cubeNTriangleIndex * 3, GL_UNSIGNED_INT, (void*)0);

	glUseProgram(NULL);
	glutSwapBuffers();
}

void resizeFunc(int width, int height) {
	glViewport(0, 0, width, height);
	glutPostRedisplay();
}
void timerFunc(int value) {
	// Actualizar el ángulo de CADA coche en la lista
	for (auto& car : cars) {
		car.angle += car.speed;
		if (car.angle > 6.28318f) car.angle -= 6.28318f;

		car.driftPhase += 0.10f + car.speed * 2.0f;
	}


	glutPostRedisplay();

	// Volver a llamar a esta función en 16ms (aprox 60 FPS)
	glutTimerFunc(1000 / 60, timerFunc, 0);
}

void keyboardFunc(unsigned char key, int x, int y) {
	float cameraSpeed = 0.1f;
	float lightSpeed = 0.2f;
	bool updateLight = false;
	//Movimiento hacia delante
	if (key == 'w' || key == 'W')
	{
		cameraPos += cameraSpeed * cameraFront;
	}
	//Movimiento hacia atrás
	else if (key == 's' || key == 'S')
	{
		cameraPos -= cameraSpeed * cameraFront;
	}
	//Movimiento izquierda
	else if (key == 'a' || key == 'A')
	{
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}
	//Movimiento derecha
	else if (key == 'd' || key == 'D')
	{
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}



	//CONTROL DE LUZ
	//Movimiento
	if (key == 'i' || key == 'I') { scenelight.lpos.z -= lightSpeed; updateLight = true; }
	if (key == 'k' || key == 'K') { scenelight.lpos.z += lightSpeed; updateLight = true; }
	if (key == 'j' || key == 'J') { scenelight.lpos.x -= lightSpeed; updateLight = true; }
	if (key == 'l' || key == 'L') { scenelight.lpos.x += lightSpeed; updateLight = true; }
	if (key == 'u' || key == 'U') { scenelight.lpos.y += lightSpeed; updateLight = true; }
	if (key == 'o' || key == 'O') { scenelight.lpos.y -= lightSpeed; updateLight = true; }

	//Intensidad (B: brighter, V:Darker)
	if (key == 'b' || key == 'B') {
		scenelight.Id += glm::vec3(0.1f);
		scenelight.Is += glm::vec3(0.1f);
		updateLight = true;
	}
	if (key == 'v' || key == 'V') {
		scenelight.Id -= glm::vec3(0.1f);
		scenelight.Is -= glm::vec3(0.1f);
		// Evitar valores negativos
		if (scenelight.Id.x < 0.0f) scenelight.Id = glm::vec3(0.0f);
		if (scenelight.Is.x < 0.0f) scenelight.Is = glm::vec3(0.0f);
		updateLight = true;
	}
	if (updateLight) {
		glBindBuffer(GL_UNIFORM_BUFFER, uboLight);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lightData), &scenelight);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glutPostRedisplay();
	}

}
void mouseFunc(int button, int state, int x, int y) {
	if (button == GLUT_RIGHT_BUTTON)
	{
		if (state == GLUT_DOWN) {
			rightMouseDown = true;
			lastMouseX = x;
			lastMouseY = y;
		}
		else {
			rightMouseDown = false;
		}

	}

}

void motionFunc(int x, int y) {
	if (rightMouseDown) {
		float xoffset = x - lastMouseX;
		float yoffset = lastMouseY - y;
		lastMouseX = x;
		lastMouseY = y;

		float sensitivity = 0.2f;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		yaw += xoffset;
		pitch += yoffset;

		// Restricciones para no dar la vuelta completa
		if (pitch > 89.0f) pitch = 89.0f;
		if (pitch < -89.0f) pitch = -89.0f;

		// Calcular el nuevo vector frontal
		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraFront = glm::normalize(front);

		glutPostRedisplay();
	}
}
void mouseWheelFunc(int wheel, int direction, int x, int y) {
	// direction: +1 (hacia adelante/arriba), -1 (hacia atrás/abajo)
	if (direction > 0)
		fov -= 2.0f; // Zoom in (reducir FOV)
	else
		fov += 2.0f; // Zoom out (aumentar FOV)

	// Limitar el zoom para evitar que se invierta o sea demasiado grande
	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 90.0f)
		fov = 90.0f;

	// Recalcular la matriz de proyección con el nuevo FOV
	proj = glm::perspective(glm::radians(fov), 1.0f, 0.1f, 1000.0f);

	glutPostRedisplay();
}
void initUBOs() {
	scenelight.Ia = glm::vec3(0.3f);
	scenelight.Id = glm::vec3(1.0f);
	scenelight.Is = glm::vec3(1.0f);
	scenelight.lpos = glm::vec3(0.0f, 5.0f, 0.0f);

	//=== CREAR EL UBO ===

	//Generar el buffer
	glGenBuffers(1, &uboLight);
	glBindBuffer(GL_UNIFORM_BUFFER, uboLight);

	//Asignar espacio y copiar los datos
	glBufferData(GL_UNIFORM_BUFFER, sizeof(lightData), &scenelight, GL_STATIC_DRAW);

	//Enlazar el buffer al binding point
	glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BINDING_INDEX, uboLight);


	//Desenlazar el buffer
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}