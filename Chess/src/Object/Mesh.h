#ifndef MESH_OBJECT_H
#define MESH_OBJECT_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../shader.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec2 Texture;
    glm::vec3 Normal;
};

class Object {
public:
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> textures;
    std::vector<glm::vec3> normals;
    std::vector<Vertex> vertices;
    int numVertices = 0;
    GLuint VBO = 0, VAO = 0;
    glm::mat4 model = glm::mat4(1.0f);

    explicit Object(const char* path) {
        std::ifstream infile(path);
        if (!infile) {
            std::cerr << "Failed to open OBJ: " << path << std::endl;
            return;
        }
        std::string line;
        while (std::getline(infile, line)) {
            std::istringstream iss(line);
            std::string tag; iss >> tag;
            if (tag == "v") {
                float x,y,z; iss >> x >> y >> z; positions.push_back({x,y,z});
            } else if (tag == "vn") {
                float x,y,z; iss >> x >> y >> z; normals.push_back({x,y,z});
            } else if (tag == "vt") {
                float u,v; iss >> u >> v; textures.push_back({u,v});
            } else if (tag == "f") {
                // Collect all vertex tokens on this face
                std::vector<std::string> tokens; std::string tok;
                while (iss >> tok) tokens.push_back(tok);

                auto pushVertexFromToken = [&](const std::string& token){
                    // token formats: p, p/t, p//n, p/t/n (1-based indices)
                    int pi = -1, ti = -1, ni = -1;
                    // split by '/'
                    std::vector<std::string> parts; parts.reserve(3);
                    size_t start = 0; size_t pos;
                    while ((pos = token.find('/', start)) != std::string::npos) {
                        parts.push_back(token.substr(start, pos - start));
                        start = pos + 1;
                    }
                    parts.push_back(token.substr(start));
                    try {
                        if (!parts.empty() && !parts[0].empty()) pi = std::stoi(parts[0]) - 1;
                        if (parts.size() >= 2 && !parts[1].empty()) ti = std::stoi(parts[1]) - 1;
                        if (parts.size() >= 3 && !parts[2].empty()) ni = std::stoi(parts[2]) - 1;
                    } catch (const std::exception&) {
                        // Ignore malformed indices; they will remain -1
                    }

                    Vertex vtx{}; // zero-initialize
                    if (pi >= 0 && pi < (int)positions.size()) vtx.Position = positions[pi];
                    if (ni >= 0 && ni < (int)normals.size())   vtx.Normal   = normals[ni];
                    if (ti >= 0 && ti < (int)textures.size())  vtx.Texture  = textures[ti];
                    vertices.push_back(vtx);
                };

                if (tokens.size() >= 3) {
                    // Build a temporary list of vertices for this polygon, then emit as triangle fan
                    std::vector<Vertex> polyVerts; polyVerts.reserve(tokens.size());
                    for (const auto& t : tokens) { pushVertexFromToken(t); polyVerts.push_back(vertices.back()); vertices.pop_back(); }
                    // Now polyVerts has the parsed vertices; emit triangles (0,1,2), (0,2,3), ...
                    auto emitTri = [&](const Vertex& a, const Vertex& b, const Vertex& c){ vertices.push_back(a); vertices.push_back(b); vertices.push_back(c); };
                    emitTri(polyVerts[0], polyVerts[1], polyVerts[2]);
                    for (size_t i = 3; i < polyVerts.size(); ++i) emitTri(polyVerts[0], polyVerts[i-1], polyVerts[i]);
                }
            }
        }
        infile.close();
        numVertices = (int)vertices.size();
        std::cout << "Loaded OBJ with " << numVertices << " vertices" << std::endl;
    }

    void makeObject(Shader& shader, bool useTex = true) {
        if (numVertices <= 0) return;
        std::vector<float> data(8 * numVertices);
        for (int i=0; i<numVertices; ++i) {
            const Vertex& v = vertices[i];
            data[i*8+0]=v.Position.x; data[i*8+1]=v.Position.y; data[i*8+2]=v.Position.z;
            data[i*8+3]=v.Texture.x;  data[i*8+4]=v.Texture.y;
            data[i*8+5]=v.Normal.x;   data[i*8+6]=v.Normal.y;  data[i*8+7]=v.Normal.z;
        }
        glGenVertexArrays(1,&VAO); glGenBuffers(1,&VBO);
        glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER,VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*data.size(), data.data(), GL_STATIC_DRAW);
        GLint att_pos = glGetAttribLocation(shader.ID, "position");
        if (att_pos >= 0) { glEnableVertexAttribArray(att_pos); glVertexAttribPointer(att_pos,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0); }
        if (useTex) {
            GLint att_tex = glGetAttribLocation(shader.ID, "tex_coord");
            if (att_tex >= 0) { glEnableVertexAttribArray(att_tex); glVertexAttribPointer(att_tex,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float))); }
        }
        GLint att_nrm = glGetAttribLocation(shader.ID, "normal");
        if (att_nrm >= 0) { glEnableVertexAttribArray(att_nrm); glVertexAttribPointer(att_nrm,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(5*sizeof(float))); }
        glBindBuffer(GL_ARRAY_BUFFER,0); glBindVertexArray(0);
    }

    void draw() { if (VAO) { glBindVertexArray(VAO); glDrawArrays(GL_TRIANGLES, 0, numVertices); } }
};

#endif
