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

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//#include <windows.h>

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

auto gFragmentShader =""
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"\n"
"uniform vec2 u_resolution;\n"
"uniform vec2 u_mouse;\n"
"uniform float u_time;\n"
"\n"
"float random (in vec2 _st) {\n"
"    return fract(sin(dot(_st.xy,\n"
"                         vec2(12.9898,78.233)))*\n"
"        43758.5453123);\n"
"}\n"
"\n"
"// Based on Morgan McGuire @morgan3d\n"
"// https://www.shadertoy.com/view/4dS3Wd\n"
"float noise (in vec2 _st) {\n"
"    vec2 i = floor(_st);\n"
"    vec2 f = fract(_st);\n"
"\n"
"    // Four corners in 2D of a tile\n"
"    float a = random(i);\n"
"    float b = random(i + vec2(1.0, 0.0));\n"
"    float c = random(i + vec2(0.0, 1.0));\n"
"    float d = random(i + vec2(1.0, 1.0));\n"
"\n"
"    vec2 u = f * f * (3.0 - 2.0 * f);\n"
"\n"
"    return mix(a, b, u.x) +\n"
"            (c - a)* u.y * (1.0 - u.x) +\n"
"            (d - b) * u.x * u.y;\n"
"}\n"
"\n"
"#define NUM_OCTAVES 8\n"
"\n"
"float fbm ( in vec2 _st) {\n"
"    float v = 0.0;\n"
"    float a = 0.5;\n"
"    vec2 shift = vec2(100.0);\n"
"    // Rotate to reduce axial bias\n"
"    mat2 rot = mat2(cos(0.5), sin(0.5),\n"
"                    -sin(0.5), cos(0.50));\n"
"    for (int i = 0; i < NUM_OCTAVES; ++i) {\n"
"        v += a * noise(_st);\n"
"        _st = rot * _st * 2.0 + shift;\n"
"        a *= 0.5;\n"
"    }\n"
"    return v;\n"
"}\n"
"\n"
"void main() {\n"
"    vec2 st = gl_FragCoord.xy/u_resolution.y*4.;\n"
"    // st += st * abs(sin(u_time*0.1)*3.0);\n"
"    vec3 color = vec3(0.0);\n"
"\n"
"    vec2 q = vec2(0.);\n"
"    q.x = fbm( st + 0.00*u_time);\n"
"    q.y = fbm( st + vec2(1.0));\n"
"\n"
"    vec2 r = vec2(0.);\n"
"    r.x = fbm( st + 1.0*q + vec2(1.7,9.2)+ 0.15*u_time*10. );\n"
"    r.y = fbm( st + 1.0*q + vec2(8.3,2.8)+ 0.126*u_time*10.);\n"
"\n"
"    float f = fbm(st+r);\n"
"\n"
"    color = mix(vec3(st.x,cos(u_time/10.),sin(u_time/10.)),\n"
"                vec3(st.y,sin(u_time/10.),sin(u_time/10.)*cos(u_time/10.)),\n"
"                clamp((f*f)*4.0,0.0,1.0));\n"
"\n"
"    color = mix(color,\n"
"                vec3(0,0,0.164706),\n"
"                clamp(length(q),.0,1.0));\n"
"\n"
"    color = mix(color,\n"
"                vec3(0.666667,1,1),\n"
"                clamp(length(r.x),0.0,1.0));\n"
"\n"
"    gl_FragColor = vec4((f*f*f+.6*f*f+.5*f)*color,1.);\n"
"}";

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

float time = 0.f;
//int Start = GetTickCount();
void renderFrame() {
//    static float grey;
//    grey += 0.01f;
//    if (grey > 1.0f) {
//        grey = 0.0f;
//    }

    time += 0.0166666666;


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
    glUniform1f(guTimehandle,(GLfloat) time);

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
