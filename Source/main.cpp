#include "../Include/Common.h"

#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define FOG 4
#define NUM_OF_OBJ 1
#define SHADOW_MAP_SIZE 4096

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

int timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;
float pace = 2.0f;
float deg90 = 3.1415926 / 2;
float deg180 = 3.1415926;
float deg270 = 3.1415926 * 1.5;
float seaDep = 0; //seaDep must be less than a constant(0 now) or it will be strange

using namespace glm;
using namespace std;

mat4 view;
mat4 projection;
mat4 model;
mat4 object_modeling[NUM_OF_OBJ];

GLint um4p;
GLint um4mv;
GLint um4shadow;
GLint shadowmap;
GLint state;
GLuint time_uniform;

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
float modelcenter[NUM_OF_OBJ][3];
float modelheight[NUM_OF_OBJ];
Shape ter_shape;
int NumOfParts[NUM_OF_OBJ];

GLuint skybox_prog;
GLuint skybox_tex;
GLuint skybox_vao;
GLuint skybox_vp_matrix;
GLuint skybox_eye_pos;

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
float light_center[4];
float light_upVec[4];
GLuint iLoclightPosition;
GLuint ambient_tex;
GLuint diffuse_tex;
GLuint specular_tex;

bool fogEnabled;
GLuint fogUniform;

GLuint reflectProgram;
struct _ReflectInfo
{
	GLuint um4mv;
	GLuint um4p;
	GLuint tex;
	GLuint seaDep;
	GLuint Ka;
	GLuint Kd;
	GLuint Ks;
	GLuint um4shadow;
	GLuint shadowmap;
	GLuint lightPos;
} ReflectInfo;

GLuint depth_prog;
GLuint depth_fbo;
GLuint depth_tex;
GLuint depth_prog_mvp;
struct
{
	int width;
	int height;
}viewportSize;

int last_time;
int nframes;

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
		std::cerr << "Could not load file " << fName[ObjectNum] << std::endl;
		return;
	}

	NumOfParts[ObjectNum] = scene->mNumMeshes;
	shape[ObjectNum] = new Shape[scene->mNumMeshes];

	printf("Load Models Success ! Shapes size %d Maerial size %d\n", NumOfParts[ObjectNum], scene->mNumMaterials);

	float Vmin[3];
	float Vmax[3];

	Vmin[0] = scene->mMeshes[0]->mVertices[0].x;
	Vmin[1] = scene->mMeshes[0]->mVertices[0].y;
	Vmin[2] = scene->mMeshes[0]->mVertices[0].z;
	Vmax[0] = scene->mMeshes[0]->mVertices[0].x;
	Vmax[1] = scene->mMeshes[0]->mVertices[0].y;
	Vmax[2] = scene->mMeshes[0]->mVertices[0].z;

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
			
			// Update the max and min value of the object
			for (int j = 0; j < mesh->mNumVertices; j++) {
				Vmin[0] = min(Vmin[0], mesh->mVertices[j].x);
				Vmin[1] = min(Vmin[1], mesh->mVertices[j].y);
				Vmin[2] = min(Vmin[2], mesh->mVertices[j].z);
				Vmax[0] = max(Vmax[0], mesh->mVertices[j].x);
				Vmax[1] = max(Vmax[1], mesh->mVertices[j].y);
				Vmax[2] = max(Vmax[2], mesh->mVertices[j].z);
			}
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

	modelcenter[ObjectNum][0] = (Vmax[0] + Vmin[0]) / 2;
	modelcenter[ObjectNum][1] = (Vmax[1] + Vmin[1]) / 2;
	modelcenter[ObjectNum][2] = (Vmax[2] + Vmin[2]) / 2;
	modelheight[ObjectNum] = Vmax[1] - Vmin[1];
	cout << "Object Name : " << fName[ObjectNum] << '\n';
	printf("max value (%f, %f, %f)\n", Vmax[0], Vmax[1], Vmax[2]);
	printf("min value (%f, %f, %f)\n", Vmin[0], Vmin[1], Vmin[2]);

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

		set_float4(c, 0.3f, 0.3f, 0.3f, 1.0f);
		if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &ambient)) {
			color4_to_float4(&ambient, c);
		}
		if (c[0] == 0.0f && c[1] == 0.0f && c[2] == 0.0f) {
			set_float4(c, 0.3f, 0.3f, 0.3f, 1.0f);
		}
		set_float4(myMaterial[ObjectNum][i].ka, c[0], c[1], c[2], c[3]);

		set_float4(c, 0.75f, 0.75f, 0.75f, 1.0f);
		if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse)) {
			color4_to_float4(&diffuse, c);
		}
		if (c[0] == 0.0f && c[1] == 0.0f && c[2] == 0.0f) {
			set_float4(c, 0.75f, 0.75f, 0.75f, 1.0f);
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

	texture_data tdata = load_png("water.jpg");

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

void MyShadowMap()
{
	glGenFramebuffers(1, &depth_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, depth_fbo);

	glGenTextures(1, &depth_tex);
	glBindTexture(GL_TEXTURE_2D, depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex, 0);
}

void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Model
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
	fogUniform = glGetUniformLocation(program, "fogUniform");
	time_uniform = glGetUniformLocation(program, "time");
	um4shadow = glGetUniformLocation(program, "um4shadow");
	shadowmap = glGetUniformLocation(program, "shadowmap");

	glUseProgram(program);

	// Skybox
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
	skybox_vp_matrix = glGetUniformLocation(skybox_prog, "vp_matrix");
	skybox_eye_pos = glGetUniformLocation(skybox_prog, "eye_position");

	glUseProgram(skybox_prog);

	// Reflection
	reflectProgram = glCreateProgram();
	GLuint reflectvs = glCreateShader(GL_VERTEX_SHADER);
	GLuint reflectfs = glCreateShader(GL_FRAGMENT_SHADER);
	char** reflect_vs_src = loadShaderSource("reflect_vs.glsl");
	char** reflect_fs_src = loadShaderSource("reflect_fs.glsl");
	glShaderSource(reflectvs, 1, reflect_vs_src, NULL);
	glShaderSource(reflectfs, 1, reflect_fs_src, NULL);
	freeShaderSource(reflect_vs_src);
	freeShaderSource(reflect_fs_src);
	glCompileShader(reflectvs);
	glCompileShader(reflectfs);
	shaderLog(reflectvs);
	shaderLog(reflectfs);
	glAttachShader(reflectProgram, reflectvs);
	glAttachShader(reflectProgram, reflectfs);
	glLinkProgram(reflectProgram);
	ReflectInfo.um4mv = glGetUniformLocation(reflectProgram, "um4mv");
	ReflectInfo.um4p = glGetUniformLocation(reflectProgram, "um4p");
	ReflectInfo.tex = glGetUniformLocation(reflectProgram, "tex");
	ReflectInfo.seaDep = glGetUniformLocation(reflectProgram, "seaDep");
	ReflectInfo.Ka = glGetUniformLocation(reflectProgram, "Ka");
	ReflectInfo.Kd = glGetUniformLocation(reflectProgram, "Kd");
	ReflectInfo.Ks = glGetUniformLocation(reflectProgram, "Ks");
	ReflectInfo.um4shadow = glGetUniformLocation(reflectProgram, "um4shadow");
	ReflectInfo.shadowmap = glGetUniformLocation(reflectProgram, "shadowmap");
	ReflectInfo.lightPos = glGetUniformLocation(reflectProgram, "lightPos");

	glUseProgram(reflectProgram);

	// Shadow program
	depth_prog = glCreateProgram();
	GLuint depthvs = glCreateShader(GL_VERTEX_SHADER);
	GLuint depthfs = glCreateShader(GL_FRAGMENT_SHADER);
	char** depth_vs_src = loadShaderSource("depth_vs.glsl");
	char** depth_fs_src = loadShaderSource("depth_fs.glsl");
	glShaderSource(depthvs, 1, depth_vs_src, NULL);
	glShaderSource(depthfs, 1, depth_fs_src, NULL);
	freeShaderSource(depth_vs_src);
	freeShaderSource(depth_fs_src);
	glCompileShader(depthvs);
	glCompileShader(depthfs);
	shaderLog(depthvs);
	shaderLog(depthfs);
	glAttachShader(depth_prog, depthvs);
	glAttachShader(depth_prog, depthfs);
	glLinkProgram(depth_prog);
	depth_prog_mvp = glGetUniformLocation(depth_prog, "mvp");

	fName[0] = "Scifi/Scifi downtown city.obj";
	Dir[0] = "Scifi/";

	cam.center = vec3(0.0f, 5.0f, 0.0f);
	cam.eye = vec3(0.0f, 5.0f, 9.0f);
	cam.up_vector = vec3(0.0f, 1.0f, 0.0f);
	cam.yaw = 0.0f;
	cam.pitch = 0.0f;
	terrain_model = mat4();
	set_float4(lightPosition, -250.0f, 250.0f, 250.0f, 0.0f);
	set_float4(light_center, 0.0f, 0.0f, 0.0f, 0.0f);
	set_float4(light_upVec, 0.0f, 1.0f, 0.0f, 0.0f);
	fogEnabled = true;

	for (int i = 0; i < NUM_OF_OBJ; i++) {
		MyLoadObject(i);
		if (i == 0) {
			object_modeling[i] = translate(mat4(), vec3(0, modelheight[i]/2-10.0f, 0));
			object_modeling[i] = object_modeling[i] * translate(mat4(), vec3(-modelcenter[i][0], -modelcenter[i][1], -modelcenter[i][2]));
		}
	}

	MyLoadPlane();
	MySkybox();
	MyShadowMap();

	//last_time = glutGet(GLUT_ELAPSED_TIME);
	//nframes = 0;
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	/*int current_time = glutGet(GLUT_ELAPSED_TIME);
	nframes++;
	if (current_time - last_time >= 1000) { // If last prinf() was more than 1 sec ago
										 // printf and reset timer
		printf("%f frames per second\n", double(nframes*1000) / double(current_time - last_time));
		nframes = 0;
		last_time += 1000;
	}*/

	const float shadow_range = 1000.0f;
	mat4 scale_bias_matrix = translate(mat4(), vec3(0.5f, 0.5f, 0.5f));
	scale_bias_matrix = scale(scale_bias_matrix, vec3(0.5f, 0.5f, 0.5f));

	mat4 light_proj_matrix = ortho(-shadow_range, shadow_range, -shadow_range, shadow_range, 0.0f, 1000.0f);
	mat4 light_view_matrix = lookAt(vec3(lightPosition[0], lightPosition[1], lightPosition[2]), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 light_vp_matrix = light_proj_matrix * light_view_matrix;
	
	mat4 shadow_sbpv_matrix = scale_bias_matrix * light_proj_matrix* light_view_matrix;
	mat4 shadow_matrix;

	// Shadow Map
	glUseProgram(depth_prog);
	glBindFramebuffer(GL_FRAMEBUFFER, depth_fbo);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4.0f, 4.0f);
	for (int ObjectNum = 0; ObjectNum < NUM_OF_OBJ; ObjectNum++)
	{
		glUniformMatrix4fv(depth_prog_mvp, 1, GL_FALSE, value_ptr(light_vp_matrix * object_modeling[ObjectNum]));
		for (unsigned int i = 0; i < NumOfParts[ObjectNum]; i++)
		{
			glBindVertexArray(shape[ObjectNum][i].vao);
			glDrawElements(GL_TRIANGLES, shape[ObjectNum][i].drawCount, GL_UNSIGNED_INT, 0);
		}
	}
	glDisable(GL_POLYGON_OFFSET_FILL);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, viewportSize.width, viewportSize.height);

	// Draw skybox
	glUseProgram(skybox_prog);

	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_tex);
	glBindVertexArray(skybox_vao);
	glUniformMatrix4fv(skybox_vp_matrix, 1, GL_FALSE, value_ptr(projection * view));
	float skybox_eye[3] = { cam.eye.x, cam.eye.y, cam.eye.z };
	glUniform3fv(skybox_eye_pos, 1, skybox_eye);
	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_DEPTH_TEST);

	// Draw Reflection
	glUseProgram(reflectProgram);

	glUniform4fv(ReflectInfo.lightPos, 1, lightPosition);
	glUniform1f(ReflectInfo.seaDep, seaDep);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(ReflectInfo.shadowmap, 0);
	glBindTexture(GL_TEXTURE_2D, depth_tex);

	glActiveTexture(GL_TEXTURE1);
	glUniform1i(ReflectInfo.tex, 1);
	
	for (int ObjectNum = 0; ObjectNum < NUM_OF_OBJ; ObjectNum++)
	{
		shadow_matrix = shadow_sbpv_matrix * object_modeling[ObjectNum];
		glUniformMatrix4fv(ReflectInfo.um4shadow, 1, GL_FALSE, value_ptr(shadow_matrix));
		glUniformMatrix4fv(ReflectInfo.um4mv, 1, GL_FALSE, value_ptr(view * object_modeling[ObjectNum]));
		glUniformMatrix4fv(ReflectInfo.um4p, 1, GL_FALSE, value_ptr(projection));
		for (unsigned int i = 0; i < NumOfParts[ObjectNum]; i++)
		{
			glBindVertexArray(shape[ObjectNum][i].vao);
			int materialID = shape[ObjectNum][i].materialId;

			glUniform4fv(ReflectInfo.Ka, 1, myMaterial[ObjectNum][materialID].ka);
			glUniform4fv(ReflectInfo.Kd, 1, myMaterial[ObjectNum][materialID].kd);
			glUniform4fv(ReflectInfo.Ks, 1, myMaterial[ObjectNum][materialID].ks);

			glBindTexture(GL_TEXTURE_2D, myMaterial[0][materialID].diffuse_tex);

			glDrawElements(GL_TRIANGLES, shape[ObjectNum][i].drawCount, GL_UNSIGNED_INT, 0);
		}
	}

	// Draw model
	glUseProgram(program);

	glUniform1i(fogUniform, fogEnabled ? 1 : 0);
	glUniform1i(time_uniform, timer_cnt);
	glUniform4fv(iLoclightPosition, 1, lightPosition);

	glActiveTexture(GL_TEXTURE4);
	glUniform1i(shadowmap, 4);
	glBindTexture(GL_TEXTURE_2D, depth_tex);
	
	for(int ObjectNum = 0; ObjectNum < NUM_OF_OBJ; ObjectNum++)
	{
		shadow_matrix = shadow_sbpv_matrix * object_modeling[ObjectNum];
		glUniformMatrix4fv(um4shadow, 1, GL_FALSE, value_ptr(shadow_matrix));
		glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * object_modeling[ObjectNum]));
		glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
		glUniform1i(state, 0);
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

	// Draw water
	glBindVertexArray(ter_shape.vao);
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * terrain_model));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, terrain_tex);
	glUniform1i(terrain_tex_uniform, 3);
	glUniform1i(state, 99);

	glDrawElementsInstanced(GL_TRIANGLES, ter_shape.drawCount, GL_UNSIGNED_INT, 0, 99 * 99);

    glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	viewportSize.width = width;
	viewportSize.height = height;
	glViewport(0, 0, width, height);

	float viewportAspect = (float)width / (float)height;
	projection = perspective(radians(60.0f), viewportAspect, 0.1f, 1000.0f);
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
	case 'k':
		printf("camera position: %f, %f, %f\n", cam.eye.x, cam.eye.y, cam.eye.z);
	default:
		break;
	}
	view = lookAt(cam.eye, cam.center, cam.up_vector);
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
	case FOG:
		fogEnabled = !fogEnabled;
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
	glutAddMenuEntry("Fog", FOG);
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