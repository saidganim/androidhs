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

typedef struct{
    GLuint       *counterList;
    int         numCounters;
    int         maxActiveCounters;
} CounterInfo;

const char *fragmentShaderSource = 
    "#version 300 es\n"
	"#define STRIDE 4 // access stride\n"
	"uniform sampler2D tex;\n"
    "uniform int MAX; \n"
	"out vec4 val;\n"
	"void main() {\n"
	"   ivec2 texCoord;\n"
	"   // external loop not required for (a)\n"
	"   for (int i=0; i<2; i++) {\n"
	"       for (int x=0; x < MAX; x += 4) {\n"

	"        texCoord.x = x % 2048;\n"
    "        texCoord.y = x;\n"
    
    // "           texCoord.x = (x % 1024);\n"
	// "           texCoord.y = (x / 1024);\n"
	"           val += texelFetch(tex, ivec2(texCoord),0);\n"
	"       }\n"
	"   }\n"
	"}\n\0";

const char *vertexShaderSource = "#version 300 es\n"
    "void main(){\n"
    "   gl_Position =  vec4(0.f, 0.f, 0.f, 1.f);\n"
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
    EGLint attribs[] = {
        EGL_NONE, 
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        
    };
    
    EGLint pBuffer_attribs[] = {
        EGL_WIDTH, 32,
        EGL_HEIGHT, 32,
        EGL_TEXTURE_TARGET, EGL_NO_TEXTURE,
        EGL_TEXTURE_FORMAT, EGL_NO_TEXTURE,
        EGL_NONE
    };
    EGLint ctx_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    EGLint num_configs;
    EGLConfig config;
    if (!eglChooseConfig(display, attribs, &config, 1, &num_configs) || (num_configs < 1)){
        printf("Could not find config for %s (perhaps this API is unsupported?)\n", "GLES3");
    }
    EGLint vid;
    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &vid)){
        printf("Could not get native visual ID from chosen config\n");
    }
    eglBindAPI(EGL_OPENGL_ES_API);
    pBuffer = eglCreatePbufferSurface(display, config, pBuffer_attribs);
    ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attribs);
    if (ctx == EGL_NO_CONTEXT) {
        printf("Context not created!\n");
    }

    if(!eglMakeCurrent(display, pBuffer, pBuffer, ctx)){ 
        printf("eglMakeCurrent() failed\n");

    }
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
		   printf("GROUP CMP %s\n", curGroupName);
           if (strcmp(groupName, curGroupName) == 0){
               *groupID = groups[i];
				printf("SUCCESS GROUP\n");
               break;
           }
        }

        if ( i == numGroups )
            exit(-1);           // error - could not find the group name

        for ( int j = 0; j < counters[i].numCounters; j++ ){
            char curCounterName[256];
            glGetPerfMonitorCounterStringAMD(groups[i], counters[i].counterList[j], 256, NULL, curCounterName);
			printf("COUNTER CMP %s\n", curCounterName);
            if (strcmp(counterName, curCounterName) == 0){
                *counterID = counters[i].counterList[j];
				printf("SUCCESS COUNTER\n");
                return 0;
            }
        }
        exit(-1);           // error - could not find the counter name
    }

void drawFrameWithCounters(void){
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

	// TEXTURE
	glGenTextures(1, &tex2);
	glBindTexture(GL_TEXTURE_2D, tex2);
	uint32_t ttmp = rndX + rndY;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2048, 2048, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexSubImage2D(GL_TEXTURE_2D, 0, rndX, rndY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &ttmp);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// DRAWING
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgram);
	glBindTexture(GL_TEXTURE_2D, tex2);

	GLuint group[2];
    GLuint counter[2];
    GLuint monitor;
    GLuint *counterData;

    // ========================================= L1 CACHE RE =========================================
    printf(" ========================================= L1 CACHE RE =========================================\n");
    getCounterByName("TP", "TPL1_TPPERF_TP0_L1_MISSES", &group[0],&counter[0]);
    getCounterByName("TP", "TPL1_TPPERF_TP0_L1_REQUESTS", &group[1],&counter[1]);
    glGenPerfMonitorsAMD(1, &monitor);
    for(size_t maxval = 4; maxval < 500; maxval += 4){
        printf("%lu,", maxval);
        tex_uniform_location = glGetUniformLocation(shaderProgram, "MAX");
	    glUniform1i(tex_uniform_location, maxval);
        glSelectPerfMonitorCountersAMD(monitor, GL_TRUE, group[0], 1 ,&counter[0]);   
        glSelectPerfMonitorCountersAMD(monitor, GL_TRUE, group[1], 1 ,&counter[1]);   
        glBeginPerfMonitorAMD(monitor);
        glDrawArrays(GL_POINTS, 0, 1);
        glEndPerfMonitorAMD(monitor);
        // read the counters
        GLint resultSize;
        usleep(1000);
        glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_SIZE_AMD, sizeof(GLint), (GLuint*)&resultSize, NULL);
        if(!resultSize){
            printf("RESULTSIZE == 0...\n");
            return;
        }
        counterData = (GLuint*) malloc(resultSize);
        GLsizei bytesWritten;
        glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_AMD,  resultSize, counterData, &bytesWritten);
        // printf("COUNTER DATA HAS SIZE OF %lu / %lu\n", bytesWritten, resultSize);
        // display or log counter info
        GLsizei wordCount = 0;

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
    }   
    

    // =========================================== UCHE RE =================================================
    printf("=========================================== UCHE RE =================================================\n");
    getCounterByName("VBIF", "AXI_READ_REQUESTS_TOTAL", &group[0],&counter[0]);
    // getCounterByName("UCHE", "UCHE_UCHEPERF_VBIF_READ_BEATS_TP", &group[0],&counter[0]);
    getCounterByName("TP", "TPL1_TPPERF_TP0_L1_REQUESTS", &group[1],&counter[1]);
    glGenPerfMonitorsAMD(1, &monitor);
    for(size_t maxval = 100; maxval < 32768 / 4; maxval += 100){
        printf("%lu,", maxval * 4);
        tex_uniform_location = glGetUniformLocation(shaderProgram, "MAX");
	    glUniform1i(tex_uniform_location, maxval);
        glSelectPerfMonitorCountersAMD(monitor, GL_TRUE, group[0], 1 ,&counter[0]);   
        glSelectPerfMonitorCountersAMD(monitor, GL_TRUE, group[1], 1 ,&counter[1]);   
        glBeginPerfMonitorAMD(monitor);
        glDrawArrays(GL_POINTS, 0, 1);
        glEndPerfMonitorAMD(monitor);
        // read the counters
        GLint resultSize;
        usleep(1000);
        glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_SIZE_AMD, sizeof(GLint), (GLuint*)&resultSize, NULL);
        if(!resultSize){
            printf("RESULTSIZE == 0...\n");
            return;
        }
        counterData = (GLuint*) malloc(resultSize);
        GLsizei bytesWritten;
        glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_AMD,  resultSize, counterData, &bytesWritten);
        // printf("COUNTER DATA HAS SIZE OF %lu / %lu\n", bytesWritten, resultSize);
        // display or log counter info
        GLsizei wordCount = 0;

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
                    counterResult -= 36;
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
    }   




    // printf("RESULT %lu\n", *counterData);
	// printf("READVALS: \n");
    // unsigned int* frame  = (unsigned int*)malloc(sizeof(unsigned int) * 32 * 32);
	// memset(frame, 0x00, 32 * 32 * sizeof(unsigned int));
	// for(int  i = 0; i < 32; ++i){
	// 	for( int j = 0; j < 32; ++j){
	// 		printf("%2d ", frame[i * 32 + j]);
				
	// 	}
	// 	printf("\n");
	// }
    }


int main(){
	// #stage 1 vars
	egl_setup();
	drawFrameWithCounters();
	return 0;

}
