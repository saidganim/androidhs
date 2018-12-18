const canvas = document.getElementById('triangleCanvas');
const gl = canvas.getContext('webgl');
gl.viewport(0,0,canvas.width,canvas.height);

const vertShader = gl.createShader(gl.VERTEX_SHADER);
const vertexShaderSource = "#version 300 es\n"
"#pragma optimize(off)\n"
"uniform sampler2D row1;\n"
"uniform sampler2D row2;\n"
"uniform int border;\n"
"//layout (location = 0) in vec3 threadD;\n"
"uniform sampler2D evict1[9];\n"  
"uniform sampler2D evict2;\n" 
"uniform sampler2D evict3;\n"
"uniform sampler2D evict4;\n"
"uniform sampler2D evict5;\n"
"uniform sampler2D evict6;\n"
"uniform sampler2D evict7;\n"
"uniform sampler2D evict8;\n"
"uniform sampler2D evict9;\n"
"uniform sampler2D evict10;\n"
"uniform sampler2D evict11;\n"
"uniform sampler2D evict12;\n"
"uniform int j;\n"
"uniform int k;\n"
"vec4 val;\n"
"void main(){\n"
"	//int id = int(threadD.x) % 4;\n"
"	for(uint i = 0u; i < 1200000u; i++){\n"
"		\n"
"		 val += texelFetch(row1, ivec2(j, k), 0); \n "
"		 val += texelFetch(row2, ivec2(j, k), 0) ;\n "
"		 val += texelFetch(evict1[0], ivec2(j, k), 0) ;\n "
"		 val += texelFetch(evict1[6], ivec2(j, k), 0) ;\n "
"		 val += texelFetch(evict1[1], ivec2(j, k), 0) ;\n "
"		 val += texelFetch(evict1[7], ivec2(j, k), 0) ;\n "
"		 val += texelFetch(evict1[2], ivec2(j, k), 0) ;\n "
"		 val += texelFetch(evict1[8], ivec2(j, k), 0) ;\n "
"		 val += texelFetch(evict1[5], ivec2(j, k), 0) ;\n "

"		 val += texelFetch(row1, ivec2(j,k+2), 0) ;\n"
"		 val += texelFetch(row2, ivec2(j, k+2), 0) ;\n "
"		 val += texelFetch(evict1[0], ivec2(j, k+2), 0) ;\n "
"		 val += texelFetch(evict1[6], ivec2(j, k+2), 0) ;\n "
"		 val += texelFetch(evict1[1], ivec2(j, k+2), 0) ;\n "
"		 val += texelFetch(evict1[7], ivec2(j, k+2), 0) ;\n "
"		 val += texelFetch(evict1[2], ivec2(j, k+2), 0) ;\n "
"		 val += texelFetch(evict1[8], ivec2(j, k+2), 0) ;\n "
"		 val += texelFetch(evict1[5], ivec2(j, k+2), 0) ;\n "
"	}\n"
"	gl_Position = val;\n"
"}\n\0";


const fragmentShaderSource = "#version 300 es\n"
//"in float val;\n"
"out vec4 FragColor;\n"
"void main(){\n"
"	FragColor = vec4(0.f, 0.f, 0.f, 1.f);\n"
"}\n";
gl.shaderSource(vertShader, vertexShaderSource);
gl.compileShader(vertShader);
const fragShader = gl.createShader(gl.FRAGMENT_SHADER);
gl.shaderSource(fragShader, fragmentShaderSource);
gl.compileShader(fragShader);
const prog = gl.createProgram();
gl.attachShader(prog, vertShader);
gl.attachShader(prog, fragShader);
gl.linkProgram(prog);
gl.useProgram(prog);

gl.clearColor(1, 0, 1, 1);
gl.clear(gl.COLOR_BUFFER_BIT);

const vertexBuf = gl.createBuffer();
gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuf);
gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([ -0.5,0.5,0.0,  -0.5,-0.5,0.0,  0.5,-0.5,0.0 ]), gl.STATIC_DRAW);

const coord = gl.getAttribLocation(prog, "c");
gl.vertexAttribPointer(coord, 3, gl.FLOAT, false, 0, 0);
gl.enableVertexAttribArray(coord);

gl.drawArrays(gl.TRIANGLES, 0, 3);
