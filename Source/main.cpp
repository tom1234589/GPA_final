#include "../Include/Common.h"

#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define NUM_OF_OBJ 2

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;
float pace = 2.0f;

using namespace glm;
using namespace std;

mat4 view;
mat4 projection;
mat4 model;
mat4 object_modeling[NUM_OF_OBJ];

GLint um4p;
GLint um4mv;
GLint state;

GLuint program;
typedef struct
{
	GLuint vao;
	GLuint vbo_position;
	GLuint vbo_normal;
	GLuint vbo_texcoord;
	GLuint ibo;
	int drawCount;
	int materialId;
} Shape;

Shape *shape[NUM_OF_OBJ];
Shape ter_shape;
int NumOfParts[NUM_OF_OBJ];

GLuint skybox_prog;
GLuint skybox_tex;
GLuint skybox_vao;
GLint skybox_view_matrix;

GLuint terrain_tex;
GLuint terrain_vao;
GLuint terrain_tex_uniform;
mat4 terrain_model;

int current_x, current_y;
typedef struct
{
	vec3 center;
	vec3 eye;
	vec3 up_vector;
	float yaw;
	float pitch;
} camera;

camera cam;
char *fName[NUM_OF_OBJ];
char *Dir[NUM_OF_OBJ];

struct Material
{
	GLuint ambient_tex;
	GLuint diffuse_tex;
	GLuint specular_tex;
	float ka[4];
	float kd[4];
	float ks[4];
};

Material *myMaterial[NUM_OF_OBJ];

struct iLocMaterialInfo
{
	GLuint ambient;
	GLuint diffuse;
	GLuint specular;
}iLocMaterialInfo;

float lightPosition[4];
GLuint iLoclightPosition;
GLuint ambient_tex;
GLuint diffuse_tex;
GLuint specular_tex;

char** loadShaderSource(const char* file)
{
    FILE* fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *src = new char[sz + 1];
    fread(src, sizeof(char), sz, fp);
    src[sz] = '\0';
    char **srcp = new char*[1];
    srcp[0] = src;
    return srcp;
}

void freeShaderSource(char** srcp)
{
    delete srcp[0];
    delete srcp;
}

void set_float4(float f[4], float a, float b, float c, float d)
{
	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;
}

void color4_to_float4(const aiColor4D *c, float f[4])
{
	f[0] = c->r;
	f[1] = c->g;
	f[2] = c->b;
	f[3] = c->a;
}

void MyLoadObject(int ObjectNum)
{
	const aiScene *scene = aiImportFile(fName[ObjectNum], aiProcessPreset_TargetRealtime_MaxQuality);
	//const aiScene *scene = aiImportFile(fName, aiProcessPreset_TargetRealtime_Fast);

	if (!scene)
	{
		std::cerr << "Could not load file " << fName << std::endl;
		return;
	}

	NumOfParts[ObjectNum] = scene->mNumMeshes;
	shape[ObjectNum] = new Shape[scene->mNumMeshes];

	printf("Load Models Success ! Shapes size %d Maerial size %d\n", NumOfParts[ObjectNum], scene->mNumMaterials);

	for (unsigned int i = 0; i < NumOfParts[ObjectNum]; i++)
	{
		aiMesh *mesh = scene->mMeshes[i];
		unsigned int *faceArray = new unsigned int[3 * mesh->mNumFaces];
		unsigned int faceIndex = 0;

		for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
			const aiFace* face = &mesh->mFaces[t];

			memcpy(&faceArray[faceIndex], face->mIndices, 3 * sizeof(unsigned int));
			faceIndex += 3;
		}

		// generate Vertex Array for mesh
		glGenVertexArrays(1, &shape[ObjectNum][i].vao);
		glBindVertexArray(shape[ObjectNum][i].vao);

		// buffer for faces
		glGenBuffers(1, &shape[ObjectNum][i].ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape[ObjectNum][i].ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh->mNumFaces * 3, faceArray, GL_STATIC_DRAW);

		//buffer for vertex positions
		if (mesh->HasPositions()) {
			glGenBuffers(1, &shape[ObjectNum][i].vbo_position);
			glBindBuffer(GL_ARRAY_BUFFER, shape[ObjectNum][i].vbo_position);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		}

		// buffer for vertex normals
		if (mesh->HasNormals()) {
			glGenBuffers(1, &shape[ObjectNum][i].vbo_normal);
			glBindBuffer(GL_ARRAY_BUFFER, shape[ObjectNum][i].vbo_normal);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mNormals, GL_STATIC_DRAW);
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		}

		// buffer for vertex texture coordinates
		if (mesh->HasTextureCoords(0)) {
			float *texCoords = (float *)malloc(sizeof(float) * 2 * mesh->mNumVertices);
			for (unsigned int k = 0; k < mesh->mNumVertices; ++k) {

				texCoords[k * 2] = mesh->mTextureCoords[0][k].x;
				texCoords[k * 2 + 1] = mesh->mTextureCoords[0][k].y;

			}
			glGenBuffers(1, &shape[ObjectNum][i].vbo_texcoord);
			glBindBuffer(GL_ARRAY_BUFFER, shape[ObjectNum][i].vbo_texcoord);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * mesh->mNumVertices, texCoords, GL_STATIC_DRAW);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		}

		// unbind buffers
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		shape[ObjectNum][i].materialId = mesh->mMaterialIndex;
		shape[ObjectNum][i].drawCount = 3 * mesh->mNumFaces;
	}

	myMaterial[ObjectNum] = new Material[scene->mNumMaterials];

	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		aiMaterial *material = scene->mMaterials[i];
		aiString texturePath;

		if (material->GetTexture(aiTextureType_AMBIENT, 0, &texturePath) == aiReturn_SUCCESS)
		{
			// load width, height and data from texturePath.C_Str();
			string s(Dir[ObjectNum]);
			s.append(texturePath.C_Str());
			printf("Texture Path: %s\n", s.c_str());
			texture_data tdata = load_png(s.c_str());

			glGenTextures(1, &myMaterial[ObjectNum][i].ambient_tex);
			glBindTexture(GL_TEXTURE_2D, myMaterial[ObjectNum][i].ambient_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS)
		{
			// load width, height and data from texturePath.C_Str();
			string s(Dir[ObjectNum]);
			s.append(texturePath.C_Str());
			printf("Texture Path: %s\n", s.c_str());
			texture_data tdata = load_png(s.c_str());
			
			glGenTextures(1, &myMaterial[ObjectNum][i].diffuse_tex);
			glBindTexture(GL_TEXTURE_2D, myMaterial[ObjectNum][i].diffuse_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		if (material->GetTexture(aiTextureType_SPECULAR, 0, &texturePath) == aiReturn_SUCCESS)
		{
			// load width, height and data from texturePath.C_Str();
			string s(Dir[ObjectNum]);
			s.append(texturePath.C_Str());
			printf("Texture Path: %s\n", s.c_str());
			texture_data tdata = load_png(s.c_str());

			glGenTextures(1, &myMaterial[ObjectNum][i].specular_tex);
			glBindTexture(GL_TEXTURE_2D, myMaterial[ObjectNum][i].specular_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		float c[4];
		aiColor4D ambient;
		aiColor4D diffuse;
		aiColor4D specular;

		set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
		if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &ambient)) {
			color4_to_float4(&ambient, c);
		}
		set_float4(myMaterial[ObjectNum][i].ka, c[0], c[1], c[2], c[3]);

		set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
		if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse)) {
			color4_to_float4(&diffuse, c);
		}
		set_float4(myMaterial[ObjectNum][i].kd, c[0], c[1], c[2], c[3]);

		set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
		if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &specular)) {
			color4_to_float4(&specular, c);
		}
		set_float4(myMaterial[ObjectNum][i].ks, c[0], c[1], c[2], c[3]);
	}

	aiReleaseImport(scene);
}

void MySkybox()
{
	glGenVertexArrays(1, &skybox_vao);
	string skybox_file[6] = { "right", "left", "top", "bottom", "front", "back" };

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &skybox_tex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_tex);
	for (int i = 0; i < 6; i++) {
		string path = "skybox/" + skybox_file[i] + ".png";
		texture_data tdata = load_png(path.c_str());
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, GL_RGBA, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void MyLoadPlane()
{
	const char *groundName = "plane.obj";
	const aiScene *scene = aiImportFile(groundName, aiProcessPreset_TargetRealtime_MaxQuality);

	if (!scene)
	{
		std::cerr << "Could not load file " << groundName << std::endl;
		return;
	}

	printf("Load Plane Success ! Shapes size %d Maerial size %d\n", 1, scene->mNumMaterials);

	aiMesh *mesh = scene->mMeshes[0];
	unsigned int *faceArray = new unsigned int[3 * mesh->mNumFaces];
	unsigned int faceIndex = 0;

	for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
		const aiFace* face = &mesh->mFaces[t];
		memcpy(&faceArray[faceIndex], face->mIndices, 3 * sizeof(unsigned int));
		faceIndex += 3;
	}

	// generate Vertex Array for mesh
	glGenVertexArrays(1, &ter_shape.vao);
	glBindVertexArray(ter_shape.vao);

	// buffer for faces
	glGenBuffers(1, &ter_shape.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ter_shape.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh->mNumFaces * 3, faceArray, GL_STATIC_DRAW);

	//buffer for vertex positions
	if (mesh->HasPositions()) {
		glGenBuffers(1, &ter_shape.vbo_position);
		glBindBuffer(GL_ARRAY_BUFFER, ter_shape.vbo_position);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}
	// buffer for vertex normals
	if (mesh->HasNormals()) {
		glGenBuffers(1, &ter_shape.vbo_normal);
		glBindBuffer(GL_ARRAY_BUFFER, ter_shape.vbo_normal);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mNormals, GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}

	// buffer for vertex texture coordinates
	if (mesh->HasTextureCoords(0)) {
		float *texCoords = (float *)malloc(sizeof(float) * 2 * mesh->mNumVertices);

		for (unsigned int k = 0; k < mesh->mNumVertices; ++k) {
			texCoords[k * 2] = mesh->mTextureCoords[0][k].x;
			texCoords[k * 2 + 1] = mesh->mTextureCoords[0][k].y;
		}

		glGenBuffers(1, &ter_shape.vbo_texcoord);
		glBindBuffer(GL_ARRAY_BUFFER, ter_shape.vbo_texcoord);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * mesh->mNumVertices, texCoords, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	}

	// unbind buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	ter_shape.drawCount = 3 * mesh->mNumFaces;

	texture_data tdata = load_png("ground.png");

	glGenTextures(1, &terrain_tex);
	glBindTexture(GL_TEXTURE_2D, terrain_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	aiReleaseImport(scene);
}

void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	program = glCreateProgram();
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** vertexShaderSource = loadShaderSource("vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("fragment.fs.glsl");
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);
	shaderLog(vertexShader);
	shaderLog(fragmentShader);
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	um4p = glGetUniformLocation(program, "um4p");
	um4mv = glGetUniformLocation(program, "um4mv");
	ambient_tex = glGetUniformLocation(program, "ambient_tex");
	diffuse_tex = glGetUniformLocation(program, "diffuse_tex");
	specular_tex = glGetUniformLocation(program, "specular_tex");
	terrain_tex_uniform = glGetUniformLocation(program, "terrain_tex_uniform");
	state = glGetUniformLocation(program, "state");
	iLocMaterialInfo.ambient = glGetUniformLocation(program, "material.ambient");
	iLocMaterialInfo.diffuse = glGetUniformLocation(program, "material.diffuse");
	iLocMaterialInfo.specular = glGetUniformLocation(program, "material.specular");
	iLoclightPosition = glGetUniformLocation(program, "lightPosition");

	glUseProgram(program);

	skybox_prog = glCreateProgram();
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	char** skybox_vs_src = loadShaderSource("skybox_vs.vs.glsl");
	char** skybox_fs_src = loadShaderSource("skybox_fs.fs.glsl");
	glShaderSource(vs, 1, skybox_vs_src, NULL);
	glShaderSource(fs, 1, skybox_fs_src, NULL);
	freeShaderSource(skybox_vs_src);
	freeShaderSource(skybox_fs_src);
	glCompileShader(vs);
	glCompileShader(fs);
	shaderLog(vs);
	shaderLog(fs);
	glAttachShader(skybox_prog, vs);
	glAttachShader(skybox_prog, fs);
	glLinkProgram(skybox_prog);
	skybox_view_matrix = glGetUniformLocation(skybox_prog, "view_matrix");

	glUseProgram(skybox_prog);

	fName[0] = "Castle/Castle_OBJ.obj";
	Dir[0] = "Castle/";
	fName[1] = "Farmhouse/farmhouse_obj.obj";
	Dir[1] = "Farmhouse/";
	//fName[2] = "WoodHouse/WoodHouse.obj";
	//Dir[2] = "WoodHouse/";

	cam.center = vec3(0.0f, 2.0f, 0.0f);
	cam.eye = vec3(0.0f, 2.0f, 9.0f);
	cam.up_vector = vec3(0.0f, 1.0f, 0.0f);
	cam.yaw = 0.0f;
	cam.pitch = 0.0f;
	terrain_model = mat4();
	set_float4(lightPosition, 1.0f, 1.0f, 1.0f, 0.0f);

	for (int i = 0; i < NUM_OF_OBJ; i++) {
		MyLoadObject(i);
		if (i == 0) {
			object_modeling[i] = translate(mat4(), vec3(0.0f, 0.00000001f, -250.0f));
			//object_modeling[i] = mat4();
		}
		else if (i == 1) {
			object_modeling[i] = translate(mat4(), vec3(100.0f, 0.00000001f, 0.0f));
		}
		else if (i == 2) {
			object_modeling[i] = translate(mat4(), vec3(-100.0f, 0.00000001f, 0.0f));
		}
	}

	MyLoadPlane();
	MySkybox();
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(skybox_prog);

	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_tex);
	glBindVertexArray(skybox_vao);
	glUniformMatrix4fv(skybox_view_matrix, 1, GL_FALSE, &view[0][0]);
	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(program);

	glUniform4fv(iLoclightPosition, 1, lightPosition);

	glBindVertexArray(ter_shape.vao);
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * terrain_model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, terrain_tex);
	glUniform1i(terrain_tex_uniform, 3);
	glUniform1i(state, 0);

	glDrawElementsInstanced(GL_TRIANGLES, ter_shape.drawCount, GL_UNSIGNED_INT, 0, 99 * 99);

	for(int ObjectNum = 0; ObjectNum < NUM_OF_OBJ; ObjectNum++)
	{
		glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * object_modeling[ObjectNum]));
		glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
		//glActiveTexture(GL_TEXTURE0);
		if (ObjectNum == 0) glUniform1i(state, 1);
		else glUniform1i(state, 2);
		for (unsigned int i = 0; i < NumOfParts[ObjectNum]; i++)
		{
			glBindVertexArray(shape[ObjectNum][i].vao);
			int materialID = shape[ObjectNum][i].materialId;

			glUniform4fv(iLocMaterialInfo.ambient, 1, myMaterial[ObjectNum][materialID].ka);
			glUniform4fv(iLocMaterialInfo.diffuse, 1, myMaterial[ObjectNum][materialID].kd);
			glUniform4fv(iLocMaterialInfo.specular, 1, myMaterial[ObjectNum][materialID].ks);

			glUniform1i(ambient_tex, 0);
			glUniform1i(diffuse_tex, 1);
			glUniform1i(specular_tex, 2);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, myMaterial[ObjectNum][materialID].ambient_tex);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, myMaterial[ObjectNum][materialID].diffuse_tex);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, myMaterial[ObjectNum][materialID].specular_tex);
			
			glDrawElements(GL_TRIANGLES, shape[ObjectNum][i].drawCount, GL_UNSIGNED_INT, 0);
		}
	}
    glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	float viewportAspect = (float)width / (float)height;
	projection = perspective(radians(60.0f), viewportAspect, 0.1f, 4000.0f);
	view = lookAt(cam.eye, cam.center, cam.up_vector);
}

void My_Timer(int val)
{
	timer_cnt++;
	glutPostRedisplay();
	if(timer_enabled)
	{
		glutTimerFunc(timer_speed, My_Timer, val);
	}
}

void My_Mouse(int button, int state, int x, int y)
{
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		current_x = x;
		current_y = y;
		break;
	case GLUT_RIGHT_BUTTON:
		current_x = x;
		current_y = y;
		break;
	default:
		break;
	}
	//printf("0x%02X          ", button);
}

void My_MouseMotion(int x, int y)
{
	int diff_x = x - current_x;
	int diff_y = y - current_y;
	float x_offset = diff_x * 1.0f / 2.0f;
	float y_offset = diff_y * 1.0f / 2.0f;
	current_x = x;
	current_y = y;

	cam.yaw -= x_offset;
	cam.pitch += y_offset;
	if (cam.pitch > 89.0f) cam.pitch = 89.0f;
	if (cam.pitch < -89.0f) cam.pitch = -89.0f;
	vec3 lookVec;
	lookVec.x = cos(radians(cam.yaw)) * cos(radians(cam.pitch));
	lookVec.y = sin(radians(cam.pitch));
	lookVec.z = sin(radians(cam.yaw)) * cos(radians(cam.pitch));
	lookVec = normalize(lookVec);
	lookVec *= 3.0f;

	cam.center = cam.eye + lookVec;
	view = lookAt(cam.eye, cam.center, cam.up_vector);
}

void My_Keyboard(unsigned char key, int x, int y)
{
	vec3 lookVec = normalize(cam.center - cam.eye);
	lookVec *= pace;
	vec3 leftVec = normalize(cross(cam.up_vector, lookVec));
	leftVec *= pace;
	vec3 upVec = vec3(0.0f, 1.0f, 0.0f) * pace;
	switch (key) 
	{
	case 'w':
		cam.center += lookVec;
		cam.eye += lookVec;
		break;
	case 's':
		cam.center -= lookVec;
		cam.eye -= lookVec;
		break;
	case 'd':
		cam.center -= leftVec;
		cam.eye -= leftVec;
		break;
	case 'a':
		cam.center += leftVec;
		cam.eye += leftVec;
		break;
	case 'z':
		cam.center += upVec;
		cam.eye += upVec;
		break;
	case 'x':
		cam.center -= upVec;
		cam.eye -= upVec;
		break;
	case 'r':
		pace += 2.0f;
		if (pace > 10.0f) pace = 10.0f;
		break;
	case 'f':
		pace -= 2.0f;
		if (pace < 2.0f) pace = 2.0f;
		break;
	default:
		break;
	}
	view = lookAt(cam.eye, cam.center, cam.up_vector);
	printf("Key %c is pressed at (%d, %d)\n", key, x, y);
}

void My_SpecialKeys(int key, int x, int y)
{
	switch(key)
	{
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	switch(id)
	{
	case MENU_TIMER_START:
		if(!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	default:
		break;
	}
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
    // Change working directory to source code path
    chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(700, 700);
	glutCreateWindow("Final project"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	dumpInfo();
	My_Init();

	// Create a menu and bind it to mouse right button.
	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutMotionFunc(My_MouseMotion);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0); 

	// Enter main event loop.
	glutMainLoop();

	return 0;
}