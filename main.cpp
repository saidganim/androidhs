#include <iostream>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

EGLDisplay display;
EGLSurface pBuffer;
EGLContext ctx;


struct pagemap_entry{
    uint64_t pfn : 54;
    unsigned int soft_dirty : 1;
    unsigned int file_page : 1;
    unsigned int swapped : 1;
    unsigned int present : 1;
};


struct kgsl_entry {
	void* kgsl_gpu_va;	// the same as virtual address for embedded device
	void* kgsl_va;		// no comments...
	void* kgsl_pa;		// will be filled by us during one-way lookup
	size_t kgsl_size;	// no comments...
	uint64_t kgsl_id;	// id of GL object
	char kgsl_flags[50];	// permissions mode
	char kgsl_type[50];	// in our case everything is gpumem
	char kgsl_usage[50];	// no comments...
	size_t kgsl_sglen;	// number of pages taken by entry
};


uint64_t read_entry(int fd, void* va){
	uint64_t data;
	struct pagemap_entry res;
	uint64_t pfn;
	if(sizeof(data) > pread(fd, &data, sizeof(data), (uint64_t)va / sysconf(_SC_PAGESIZE) * sizeof(uint64_t))){
		printf("COULDN'T READ FROM PAGEMAP %p\n", va);
		exit(1);
	};
	pfn = data & (((uint64_t)1 << 54) - 1);
	
	// res.pfn = data & (((uint64_t)1 << 54) - 1);
    // res.soft_dirty = (data >> 54) & 1;
    // res.file_page = (data >> 61) & 1;
    // res.swapped = (data >> 62) & 1;
    // res.present = (data >> 63) & 1;
	return (uint64_t)( pfn * sysconf(_SC_PAGE_SIZE)) + ((uint64_t)va % sysconf(_SC_PAGE_SIZE));
}

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
	// #stage 1 vars
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
	// #stage 2 vars
	/*   Example of output of KGSL file
	 *
	 *  gpuaddr useraddr     size    id  flags       type            usage sglen
 	 *  b2e56000 b2e56000    65536    62 -r--pY     gpumem          command    16
	 *  b440a000 b440a000     4096    55 ----pY     gpumem           any(0)     1
	 *  b440c000 b440c000     4096    54 ----pY     gpumem           any(0)     1
	 */

	
	int pagemap_f = open("/proc/self/pagemap", O_RDONLY);
	char kgsl_path[100]; // no buffer overwlof here, please:)
	struct kgsl_entry kgsl_arr[50];
	struct kgsl_entry kgsl_cur;
	pid_t self = getpid();
	sprintf(kgsl_path, "/d/kgsl/proc/%d/mem", self);
	FILE* kgsl_f = fopen(kgsl_path, "r");
	size_t cur_i = 0;


	// STAGE #1

	memset(textures_data, 0x41, 4096);
	memset(readval, 0xaa, 32 * 32 * sizeof(unsigned int));
	GLuint VBO, tex, tex2[100];
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
	uint32_t ttmp = 0;
	// TEXTURE
	for(int i = 0; i < 100; ++i){
		glGenTextures(1, &tex2[i]);
		glBindTexture(GL_TEXTURE_2D, tex2[i]);
		ttmp = ttmp + rndX + rndY;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexSubImage2D(GL_TEXTURE_2D, 0, rndX, rndY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &ttmp);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	// DRAWING
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgram);
	tex_uniform_location = glGetUniformLocation(shaderProgram, "a_rnd");
	glUniform2f(tex_uniform_location, rndX / 32.f,  rndY / 32.f);
	tex_uniform_location = glGetUniformLocation(shaderProgram, "a_rnd2");
	glUniform2f(tex_uniform_location, rndX,  rndY);
	glBindTexture(GL_TEXTURE_2D, tex2[0]);
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

	
	// STAGE #2

	if(!pagemap_f || !kgsl_f){
		printf("Pagemap or KGSL interface is disabled( may be they both), please isntall all kernel modules or reinstall the kernel. See you later...");
		exit(1);
	}
	fseek(kgsl_f, 73, SEEK_SET);
	while(fscanf(kgsl_f, "%p %p     %lu    %lu  %s      %s            %s %lu\n", &kgsl_cur.kgsl_gpu_va,
									 	&kgsl_cur.kgsl_va, &kgsl_cur.kgsl_size, &kgsl_cur.kgsl_id, kgsl_cur.kgsl_flags,
										kgsl_cur.kgsl_type, kgsl_cur.kgsl_usage, &kgsl_cur.kgsl_sglen) != EOF){
		if(strcmp(kgsl_cur.kgsl_usage, "texture"))
			continue;
		// we have texture
		kgsl_cur.kgsl_pa = (void*)read_entry(pagemap_f, (void*)kgsl_cur.kgsl_va);
		kgsl_arr[cur_i++] = kgsl_cur;
		printf("%p %p     %lu    %3lu ", kgsl_cur.kgsl_gpu_va, kgsl_cur.kgsl_va, kgsl_cur.kgsl_size, kgsl_cur.kgsl_id);
		printf("%s      ", kgsl_cur.kgsl_flags);
		printf("%s            ", kgsl_cur.kgsl_type);
		printf("%s ", kgsl_cur.kgsl_usage);
		printf("%lu -- [physaddr:%p; virtualaddr:%p]\n", kgsl_cur.kgsl_sglen, kgsl_cur.kgsl_pa, kgsl_cur.kgsl_va);
		fflush(stdout);
	}
	
	


	return 0;

}
