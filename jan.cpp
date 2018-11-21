#include <iostream>

#define GLEW_VERSION_3_3

#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>




EGLDisplay display;
EGLSurface pBuffer;
EGLContext ctx;

const char *vertexShaderSource = "#version 300 es\n"
    "layout (location = 0) in vec3 aPos;\n"
    "out vec2 TexCoord;\n"
    "uniform vec2 a_rnd;\n"
    "void main()\n"
    "{\n"
    "   TexCoord = vec2(a_rnd.x, a_rnd.y);\n"
    "   gl_Position = vec4(aPos, 1.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 300 es\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;"
	"uniform sampler2D u_texture;\n"
	"uniform vec2 a_rnd;\n"
    "void main()\n"
    "{\n"
	// "   FragColor = texture(u_texture, TexCoord);\n"
    // "   FragColor = vec4(1, 1, 1, 1.0);\n"
    "      FragColor = texelFetch(u_texture, ivec2(a_rnd), 0);\n"
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
	float vertices[] = {
            // positions         // colors
            -0.5f, 0.5f, 0.0f,    // bottom right
            0.5f, 0.5, 0.0f,    // bottom left
            0.5f,  -0.5f, 0.0f,    // top
    };

    unsigned int VBO, VAO;
    GLuint fbo;
    GLuint tex, tex2;
    GLuint mProgram;

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    int  success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    const char* versionStr = (const char*)glGetString(GL_VERSION);
    // ALOGE("MY_VERSION:\n%s\n", versionStr);
    std::cout<<"WORKS?\n";
    if(!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }



    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);


    mProgram = glCreateProgram();
    glAttachShader(mProgram, vertexShader);
    glAttachShader(mProgram, fragmentShader);
    glLinkProgram(mProgram);

    glGetProgramiv(mProgram, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(mProgram, 512, NULL, infoLog);
        printf("mProgram couldn't be compiled:\n%s\n", infoLog);
    }
    printf("All shaders and mProgram works\n");


    /* * * * SHADERS COMPILED * * * * * */


    unsigned int* textures_data = (unsigned int*) malloc(3200*3200);
    GLint tex_uniform_location;
    uint32_t rndX = rand() & 0b11111, rndY = rand() & 0b11111;
    uint32_t rnd_sum = rndX + rndY;
    memset(textures_data, 0x41, 32*32*sizeof(unsigned int));
    printf("rndX + rndY = 0x%x\n", rnd_sum);
    
    // textures_data[rndX * 32 + rndY] = 0x55555555;
    // printf("Generated value: 0x%08x\n", 0x55555555);




    /* * * * SET VAO and VBO * * * * */


    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);


    /* * * Textures * * */
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, textures_data);
    glTexSubImage2D(GL_TEXTURE_2D, 0, rndX, rndY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &rnd_sum);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    


    /* * * Framebuffer * * * */
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &tex2);
    glBindTexture(GL_TEXTURE_2D, tex2);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex2, 0);




    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE){
        printf("Framebuffer completed\n");
    }

    /* draw */

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(mProgram);

    // bond a_rnd
    tex_uniform_location = glGetUniformLocation(mProgram, "a_rnd");
	glUniform2f(tex_uniform_location, rndX,  rndY);

    // bind Texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);


    unsigned int *frame = (unsigned int *) malloc(32 * 32 * sizeof(unsigned int));
    memset(frame, 0, 32 * 32 * 4);
    glReadPixels(0, 0, 32, 32, GL_RGBA, GL_UNSIGNED_BYTE, frame);
    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 32; ++j) {
            printf("%x ", frame[i * 32 + j]);
        }
        std::cout << std::endl;
    }
    return 0;












    // tex_uniform_location = glGetUniformLocation(mProgram, "a_rnd");
    // glUniform2f(tex_uniform_location, rndX,  rndY);

   
    // unsigned int readval;

    // /* Drawing */
    // glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

    // glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // glUseProgram(mProgram);
    // glBindVertexArray(VAO);

    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, tex);

    // glDrawArrays(GL_TRIANGLES, 0, 3);

    
    // // // glReadBuffer(GL_COLOR_ATTACHMENT0);
    
    // // 8. Extract rndX + rndY from framebuffer
    // // unsigned int *frame;
    // frame = (unsigned int *) malloc(32 * 32 * 4 * sizeof(unsigned int));
    // glReadPixels(0, 0, 32, 32, GL_RGBA, GL_UNSIGNED_BYTE, frame);
    // for (int i = 0; i < 32; ++i) {
    //     for (int j = 0; j < 32; ++j) {
    //         printf("%8x ", frame[i * 128 + j]);
    //     }
    //     std::cout << std::endl;
    // }
    
	
    // // glReadBuffer(GL_BACK);
    
	
	// return 0;

}