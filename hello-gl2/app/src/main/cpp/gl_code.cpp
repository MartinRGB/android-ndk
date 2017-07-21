/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// OpenGL ES 2.0 code

#include <jni.h>
#include <android/log.h>

#include <time.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

auto gVertexShader =
    "attribute vec4 a_position;\n"
//    "varying vec2 u_resolution;\n"
    "void main() {\n"
    "  gl_Position = vec4 ( a_position.x, a_position.y, 1.0, 1.0 );\n"
//    "  u_resolution.xy = a_position.xy;\n"
    "}\n";

auto gFragmentShader =
"#ifdef GL_ES\n"
        "precision highp float;\n"
        "#endif\n"
        "\n"
        "uniform vec2 u_resolution;\n"
        "uniform float u_time;\n"
        "//2\n"
        "#define iterations 17\n"
        "#define formuparam 0.53\n"
        "\n"
        "#define volsteps 10\n"
        "#define stepsize 0.1\n"
        "\n"
        "#define zoom   0.800\n"
        "#define tile   0.850\n"
        "#define speed  0.050\n"
        "\n"
        "#define brightness 0.0015\n"
        "#define darkmatter 0.300\n"
        "#define distfading 0.730\n"
        "#define saturation 0.850\n"
        "\n"
        "\n"
        "vec3 mainImage(in vec2 fragCoord )\n"
        "{\n"
        "    //get coords and direction\n"
        "    vec2 uv=fragCoord.xy/u_resolution.xy-.5;\n"
        "    uv.y*=u_resolution.y/u_resolution.x;\n"
        "    vec3 dir=vec3(uv*zoom,1.);\n"
        "    float time=u_time*speed+.25;\n"
        "\n"
        "    //mouse rotation\n"
        "    float a1=.5+(u_time)*10./u_resolution.x/100.;\n"
        "    float a2=.8+(u_time)*10./u_resolution.y/100.;\n"
        "    mat2 rot1=mat2(cos(a1),sin(a1),-sin(a1),cos(a1));\n"
        "    mat2 rot2=mat2(cos(a2),sin(a2),-sin(a2),cos(a2));\n"
        "    dir.xz*=rot1;\n"
        "    dir.xy*=rot2;\n"
        "    vec3 from=vec3(1.,0.5,0.5);\n"
        "    from+=vec3(time*2.,time,-2.);\n"
        "    from.xz*=rot1;\n"
        "    from.xy*=rot2;\n"
        "\n"
        "    //volumetric rendering\n"
        "    float s=0.1,fade=1.;\n"
        "    vec3 v=vec3(0.);\n"
        "    for (int r=0; r<volsteps; r++) {\n"
        "        vec3 p=from+s*dir*.5;\n"
        "        p = abs(vec3(tile)-mod(p,vec3(tile*2.))); // tiling fold\n"
        "        float pa,a=pa=0.;\n"
        "        for (int i=0; i<iterations; i++) {\n"
        "            p=abs(p)/dot(p,p)-formuparam; // the magic formula\n"
        "            a+=abs(length(p)-pa); // absolute sum of average change\n"
        "            pa=length(p);\n"
        "        }\n"
        "        float dm=max(0.,darkmatter-a*a*.001); //dark matter\n"
        "        a*=a*a; // add contrast\n"
        "        if (r>6) fade*=1.-dm; // dark matter, don't render near\n"
        "        //v+=vec3(dm,dm*.5,0.);\n"
        "        v+=fade;\n"
        "        v+=vec3(s/2.,s*s,s*s*s*s*4.9)*a*brightness*fade; // coloring based on distance\n"
        "        fade*=distfading; // distance fading\n"
        "        s+=stepsize;\n"
        "    }\n"
        "    v=mix(vec3(length(v)),v,saturation); //color adjust\n"
        "    //fragColor = vec4(v*.01,1.);\n"
        "\n"
        "    return v*0.01;\n"
        "}\n"
        "\n"
        "void main() {\n"
        "    //vec2 st = gl_FragCoord.xy/u_resolution;\n"
        "    //gl_FragColor = vec4(st.x,st.y,0.0,1.0);\n"
        "\n"
        "    gl_FragColor = vec4(mainImage(gl_FragCoord.xy),1.);\n"
        "\n"
        "}"
;

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram;
GLuint gvPositionHandle;
GLuint guResolutionhandle;
GLuint screenWidth;
GLuint screenHeight;
GLfloat guTimehandle;

bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
    gvPositionHandle = glGetAttribLocation(gProgram, "a_position");
    checkGlError("glGetAttribLocation");
    LOGI("glGetAttribLocation(\"a_position\") = %d\n",
            gvPositionHandle);

    screenWidth = w;
    screenHeight = h;

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
}

const GLfloat VERTEX_BUF[] = {
    1.0f, -1.0f,
    -1.0f, -1.0f,
    1.0f, 1.0f,
    -1.0f, 1.0f,
};

float mtime = 0.f;

void renderFrame() {
    mtime += 0.0166666666;

    glClearColor(0., 0., 0., 1.0f);
    checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 8, VERTEX_BUF);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");

    guResolutionhandle = glGetUniformLocation(gProgram,"u_resolution");
    glUniform2f(guResolutionhandle,(GLfloat)screenWidth,(GLfloat)screenHeight);
    checkGlError("glGetUniformLocation");

    guTimehandle = glGetUniformLocation(gProgram,"u_time");
    glUniform1f(guTimehandle,(GLfloat) mtime);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    checkGlError("glDrawArrays");



}



extern "C" {
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height);
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
    setupGraphics(width, height);
}



JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jobject obj)
{

    renderFrame();

}
