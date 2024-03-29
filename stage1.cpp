#include <iostream>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

EGLDisplay display;
EGLSurface pBuffer;
EGLContext ctx;

const char *vertexShaderSource = "#version 300 es\n"
    "layout (location = 0) in vec3 aPos;\n"
	"uniform vec2 a_rnd;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(a_rnd.x, a_rnd.y, 0.f, 1.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 300 es\n"
    "out vec4 FragColor;\n"
	"uniform sampler2D u_texture;\n"
	"uniform vec2 a_rnd2;\n"
    "void main()\n"
    "{\n"
    "FragColor =  texelFetch(u_texture, ivec2(a_rnd2), 0);\n"
    "}\n\0";


void egl_setup() {
    int maj, min;
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    if (display == EGL_NO_DISPLAY) {
        printf("EGL_NO_DISPLAY");
        exit(-1);
    }

    if (!eglInitialize(display, &maj, &min)) {
        printf("eglinfo: eglInitialize failed\n");
        exit(-1);
    }

    printf("EGL v%i.%i initialized\n", maj, min);

    EGLint attribs[] =
    {
        EGL_NONE, 
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        
    };
    
    EGLint pBuffer_attribs[] =
    {
        EGL_WIDTH, 32,
        EGL_HEIGHT, 32,
        EGL_TEXTURE_TARGET, EGL_NO_TEXTURE,
        EGL_TEXTURE_FORMAT, EGL_NO_TEXTURE,
        EGL_NONE
    };

    EGLint ctx_attribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    EGLint num_configs;
    EGLConfig config;

    if (!eglChooseConfig(display, attribs, &config, 1, &num_configs) || (num_configs < 1))
    {
        printf("Could not find config for %s (perhaps this API is unsupported?)\n", "GLES3");
    }
    EGLint vid;
    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &vid))
    {
        printf("Could not get native visual ID from chosen config\n");
    }
    eglBindAPI(EGL_OPENGL_ES_API);

    pBuffer = eglCreatePbufferSurface(display, config, pBuffer_attribs);
    ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attribs);
    if (ctx == EGL_NO_CONTEXT) {
        printf("Context not created!\n");
    }

    if(!eglMakeCurrent(display, pBuffer, pBuffer, ctx))
    { 
        printf("eglMakeCurrent() failed\n");

    }

}

int main(){
	egl_setup();
	unsigned int* textures_data = (unsigned int*) malloc(4096);
	int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint shaderProgram = glCreateProgram();
	GLuint VAO;
	GLuint FBF;
	GLint tex_uniform_location;
	unsigned int readval[32][32];
	int success;
    char infoLog[512];
	uint32_t rndX = rand() & 0b11111, rndY = rand() & 0b11111;

	memset(textures_data, 0x41, 4096);
	memset(readval, 0xaa, 32 * 32 * sizeof(unsigned int));
	GLuint VBO, tex, tex2;
	// while(1){
	
	std::cout<<"Generated value sum==" << rndX + rndY << " ; rndX==" << rndX << "; rndY==" << rndY << "\n";

	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

 	if (success != GL_TRUE)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);   	
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit(1);
    }

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

 	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	
	// FRAMEBUFFER
	glGenFramebuffers(1, &FBF);
	glBindFramebuffer(GL_FRAMEBUFFER, FBF);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, readval);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";
	}

	// TEXTURE
	glGenTextures(1, &tex2);
	glBindTexture(GL_TEXTURE_2D, tex2);
	uint32_t ttmp = rndX + rndY;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, textures_data);
	glTexSubImage2D(GL_TEXTURE_2D, 0, rndX, rndY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &ttmp);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// DRAWING
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgram);
	tex_uniform_location = glGetUniformLocation(shaderProgram, "a_rnd");
	glUniform2f(tex_uniform_location, rndX / 32.f,  rndY / 32.f);
	tex_uniform_location = glGetUniformLocation(shaderProgram, "a_rnd2");
	glUniform2f(tex_uniform_location, rndX,  rndY);
	glBindTexture(GL_TEXTURE_2D, tex2);
	glDrawArrays(GL_POINTS, 0, 1);

	unsigned int* frame  = (unsigned int*)malloc(sizeof(unsigned int) * 32 * 32);
	memset(frame, 0x00, 32 * 32 * sizeof(unsigned int));
	glReadPixels(0, 0, 32, 32, GL_RGBA,GL_UNSIGNED_BYTE, frame);
	printf("READVALS: \n");
	for(int  i = 0; i < 32; ++i){
		for( int j = 0; j < 32; ++j){
			printf("%2d ", frame[i * 32 + j]);
				
		}
		printf("\n");
	}
	return 0;

}