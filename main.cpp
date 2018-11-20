#include <iostream>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>




EGLDisplay display;
EGLSurface pBuffer;
EGLContext ctx;

const char *vertexShaderSource = "#version 300 es\n"
    // "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(0.0, 0.0, 0.0, 0.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 300 es\n"
    "out vec4 FragColor;\n"
	"uniform sampler2D u_texture;\n"
	"uniform vec2 a_rnd;\n"
    "void main()\n"
    "{\n"
	"vec2 texcoord = vec2(a_rnd.x, a_rnd.y);\n"
	"vec4 res = texelFetch(u_texture, ivec2(texcoord), 0);"
    "FragColor =  vec4(1., 0., 0., 0.);\n"
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
        EGL_WIDTH, 128,
        EGL_HEIGHT, 1024,
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
	
	unsigned int* textures_data = (unsigned int*) malloc(4096 * 3);
	int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	int shaderProgram = glCreateProgram();
	GLuint VAO;
	GLuint FBF;
	GLint tex_uniform_location;
	unsigned int readval[32][32];
   

	uint32_t rndX = rand() & 0b11111, rndY = rand() & 0b11111;

	memset(textures_data, 0x41, 4096 * 3);
	memset(readval, 0xaa, 32 * 32 * sizeof(unsigned int));
	textures_data[rndX * 32 + rndY] = rndX + rndY;
	GLuint VBO, tex, tex2;
	// while(1){
	int success;
    char infoLog[512];

	std::cout<<"Generated value " << rndX + rndY << "\n";



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
    }

	
	


	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

 	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);


	float some_data[] = {-1.0, 1.0, 0,0,
						1.0, 1.0, 0.0,
						1.0, -1, 0.0};
						// -1.0, -1.0, 0.0};

	glGenFramebuffers(1, &FBF);
	glBindFramebuffer(GL_FRAMEBUFFER, FBF);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, readval);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0); 
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!"
				<< std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, FBF);

	glGenTextures(1, &tex2);
	glBindTexture(GL_TEXTURE_2D, tex2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, 32, 32, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, textures_data);

	
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), some_data, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	//glActiveTexture(GL_TEXTURE0);
	
	
	
	// glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// tex_uniform_location = glGetUniformLocation(shaderProgram, "u_texture");
	// glUniform1fv(tex_uniform_location, 4096 / sizeof(float), textures_data);

	tex_uniform_location = glGetUniformLocation(shaderProgram, "a_rnd");
	glUniform2f(tex_uniform_location, rndX,  rndY);
	// }
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(shaderProgram);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
	glDrawArrays(GL_TRIANGLES, 0, 3);
     std::cout <<readval[0][0] <<" Hello world\n";
	// glReadBuffer(GL_COLOR_ATTACHMENT0);
	memset(readval, 0x00, 32 * 32 * sizeof(unsigned int));
	glReadPixels(0, 0, 32, 32, GL_RGBA,GL_UNSIGNED_INT, &readval);
	// glReadBuffer(GL_BACK);
	printf("READVALS: \n");
	// glfwSwapBuffers(display);
	for(int  i = 0; i < 32; ++i){
		for( int j = 0; j < 32; ++j){
			printf("0x%6x\n", readval[i][j]);
		}
		printf("=======================\n");
	}
    // printf("0x%x\n", readval);
	
	return 0;

}
