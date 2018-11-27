#include <iostream>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
// #include <EGL/gl2ext.h>

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

//const char *vertexShaderSource = "#version 300 es\n"
//    "layout (location = 0) in vec3 aPos;\n"
//	"uniform vec2 a_rnd;\n"
//    "void main()\n"
//    "{\n"
//    "   gl_Position = vec4(a_rnd.x, a_rnd.y, 0.f, 1.0);\n"
//    "}\0";


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



    typedef struct 
    {
            GLuint       *counterList;
            int         numCounters;
            int         maxActiveCounters;
    } CounterInfo;

    void
    getGroupAndCounterList(GLuint **groupsList, int *numGroups, CounterInfo **counterInfo){
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
        int          numGroups;
        GLuint       *groups;
        CounterInfo  *counters;
        int          i = 0;
        if (!countersInitialized){
            getGroupAndCounterList(&groups, &numGroups, &counters);
            countersInitialized = 1;
        }
        for ( i = 0; i < numGroups; i++ ){
           char curGroupName[256];
           glGetPerfMonitorGroupStringAMD(groups[i], 256, NULL, curGroupName);
           if (strcmp(groupName, curGroupName) == 0){
               *groupID = groups[i];
               break;
           }
        }

        if ( i == numGroups )
            return -1;           // error - could not find the group name

        for ( int j = 0; j < counters[i].numCounters; j++ ){
            char curCounterName[256];
            glGetPerfMonitorCounterStringAMD(groups[i], counters[i].counterList[j], 256, NULL, curCounterName);
            if (strcmp(counterName, curCounterName) == 0){
                *counterID = counters[i].counterList[j];
                return 0;
            }
        }
        return -1;           // error - could not find the counter name
    }

    void drawFrameWithCounters(void){
        GLuint group[2];
        GLuint counter[2];
        GLuint monitor;
        GLuint *counterData;

        // Get group/counter IDs by name.  Note that normally the
        // counter and group names need to be queried for because
        // each implementation of this extension on different hardware
        // could define different names and groups.  This is just provided
        // to demonstrate the API.
        getCounterByName("a3xx_sp_perfcounter_select", "SP_ICL1_MISSES", &group[0],&counter[0]);
        getCounterByName("API", "Draw Calls", &group[1], &counter[1]);
        // create perf monitor ID
        glGenPerfMonitorsAMD(1, &monitor);
        // enable the counters
        glSelectPerfMonitorCountersAMD(monitor, GL_TRUE, group[0], 1,&counter[0]);
        // glSelectPerfMonitorCountersAMD(monitor, GL_TRUE, group[1], 1, &counter[1]);
        glBeginPerfMonitorAMD(monitor);
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
        glEndPerfMonitorAMD(monitor);
        // read the counters
        GLint resultSize;
        glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_SIZE_AMD, sizeof(GLint), (GLuint*)&resultSize, NULL);
        counterData = (GLuint*) malloc(resultSize);
        GLsizei bytesWritten;
        glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_AMD,  resultSize, counterData, &bytesWritten);
        // display or log counter info
        GLsizei wordCount = 0;

        while ( (4 * wordCount) < bytesWritten ){
            GLuint groupId = counterData[wordCount];
            GLuint counterId = counterData[wordCount + 1];
            // Determine the counter type
            GLuint counterType;
            glGetPerfMonitorCounterInfoAMD(groupId, counterId, GL_COUNTER_TYPE_AMD, &counterType);
            if ( counterType == GL_UNSIGNED_INT64_AMD ){
                uint64_t counterResult = *(uint64_t*)(&counterData[wordCount + 2]);
                // Print counter result
                wordCount += 4;
            }
            else if ( counterType == GL_FLOAT ){
                float counterResult = *(float*)(&counterData[wordCount + 2]);
                // Print counter result
                wordCount += 3;
            }
            // else if ( ... ) check for other counter types 
            //   (GL_UNSIGNED_INT and GL_PERCENTAGE_AMD)
        }
		printf("RESULT %lu\n", wordCount);
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
    }


int main(){
	// #stage 1 vars
	egl_setup();
	drawFrameWithCounters();
	return 0;

}
