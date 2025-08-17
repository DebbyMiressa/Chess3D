#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <iostream>

class Shader {
public:
    GLuint ID;
    Shader(std::string vs, std::string fs) {
        GLuint v = compileShader(vs, GL_VERTEX_SHADER);
        GLuint f = compileShader(fs, GL_FRAGMENT_SHADER);
        ID = compileProgram(v, f);
    }
    void use(){ glUseProgram(ID); }
    void setMatrix4(const char* name, const glm::mat4& m){ const float* p = &m[0][0]; glUniformMatrix4fv(glGetUniformLocation(ID,name),1,GL_FALSE,p);}    
    void setVector3f(const char* name, const glm::vec3& v){ glUniform3f(glGetUniformLocation(ID,name),v.x,v.y,v.z);}    
    void setFloat(const char* name, float v){ glUniform1f(glGetUniformLocation(ID,name),v);}    
    void setInt(const char* name, int v){ glUniform1i(glGetUniformLocation(ID,name),v);}    
    void setBool(const char* name, bool v){ glUniform1i(glGetUniformLocation(ID,name),v);}    

private:
    static GLuint compileShader(const std::string& src, GLenum type){
        GLuint s = glCreateShader(type); const char* c = src.c_str(); glShaderSource(s,1,&c,nullptr); glCompileShader(s);
        GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok); if(!ok){ char log[1024]; glGetShaderInfoLog(s,1024,nullptr,log); std::cerr<<"Shader compile error: "<<log<<"\n"; }
        return s;
    }
    static GLuint compileProgram(GLuint vs, GLuint fs){
        GLuint p = glCreateProgram(); glAttachShader(p,vs); glAttachShader(p,fs); glLinkProgram(p);
        GLint ok=0; glGetProgramiv(p,GL_LINK_STATUS,&ok); if(!ok){ char log[1024]; glGetProgramInfoLog(p,1024,nullptr,log); std::cerr<<"Program link error: "<<log<<"\n"; }
        glDeleteShader(vs); glDeleteShader(fs); return p;
    }
};

#endif


