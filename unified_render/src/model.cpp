// Unified Render - General purpouse game engine
// Copyright (C) 2021, Unified Render contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ----------------------------------------------------------------------------
// Name:
//      model.cpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#include <algorithm>
#include <fstream>
#include <vector>
#include <iterator>

#include "unified_render/model.hpp"
#include "unified_render/shader.hpp"
#include "unified_render/print.hpp"
#include "unified_render/path.hpp"
#include "unified_render/material.hpp"
#include "unified_render/texture.hpp"
#include "unified_render/shader.hpp"
#include "unified_render/state.hpp"

//
// VAO
//
UnifiedRender::OpenGL::VAO::VAO(void) {
    glGenVertexArrays(1, &id);
}

UnifiedRender::OpenGL::VAO::~VAO(void) {
    glDeleteVertexArrays(1, &id);
}

void UnifiedRender::OpenGL::VAO::bind(void) const {
    glBindVertexArray(id);
}

GLuint UnifiedRender::OpenGL::VAO::get_id(void) const {
    return id;
}

//
// VBO
//
UnifiedRender::OpenGL::VBO::VBO(void) {
    glGenBuffers(1, &id);
}

UnifiedRender::OpenGL::VBO::~VBO(void) {
    glDeleteBuffers(1, &id);
}

void UnifiedRender::OpenGL::VBO::bind(GLenum target) const {
    glBindBuffer(target, id);
}

GLuint UnifiedRender::OpenGL::VBO::get_id(void) const {
    return id;
}

//
// Simple model
//
UnifiedRender::SimpleModel::SimpleModel(GLint _mode)
    : UnifiedRender::OpenGL::PackedModel<glm::vec3, glm::vec2>(_mode)
{

}

UnifiedRender::SimpleModel::~SimpleModel(void) {

}

void UnifiedRender::SimpleModel::draw(const UnifiedRender::OpenGL::Program& shader) const {
    // Change color if material wants it
    if(material != nullptr) {
        if(material->diffuse_map != nullptr) {
            shader.set_texture(0, "diffuse_map", *material->diffuse_map);
        }
        shader.set_uniform("color", material->ambient_color.r, material->ambient_color.g, material->ambient_color.b, 1.f);
    } else {
        shader.set_uniform("color", 1.f, 1.f, 1.f, 1.f);
    }

    vao.bind();
    glDrawArrays(mode, 0, buffer.size());
}

void UnifiedRender::SimpleModel::upload(void) {
    if(buffer.empty()) {
        return;
    }

    vao.bind();
    vbo.bind(GL_ARRAY_BUFFER);
    glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(buffer[0]), &buffer[0], GL_STATIC_DRAW);

    // Vertices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(buffer[0]), (void*)0);
    glEnableVertexAttribArray(0);
    // Texcoords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(buffer[0]), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

//
// Model
//
UnifiedRender::Model::Model(void) {

}

UnifiedRender::Model::~Model(void) {

}

void UnifiedRender::Model::draw(const UnifiedRender::OpenGL::Program& shader) const {
    std::vector<const UnifiedRender::SimpleModel *>::const_iterator model;
    for(model = simple_models.begin(); model != simple_models.end(); model++) {
        (*model)->draw(shader);
    }
}

const UnifiedRender::SimpleModel& UnifiedRender::ModelManager::load_simple(const std::string& path) {
    return *((const SimpleModel *)nullptr);
}

struct WavefrontFace {
public:
    WavefrontFace() {};
    ~WavefrontFace() {};

    // Indexes to the actual points
    std::vector<int> vertices, texcoords, normals;
};

struct WavefrontObj {
public:
    WavefrontObj(const std::string& _name) : name(_name) {};
    ~WavefrontObj() {};

    std::string name;

    std::vector<WavefrontFace> faces;
    const UnifiedRender::Material* material = nullptr;
};

const UnifiedRender::Model& UnifiedRender::ModelManager::load_wavefront(const std::string& path) {
    std::ifstream file(path);
    std::string line;

    // Recollect all data from the .OBJ file
    std::vector<glm::vec3> vertices, normals;
    std::vector<glm::vec2> texcoords;

    std::vector<WavefrontObj> objects;
    objects.push_back(WavefrontObj("default"));
    while(std::getline(file, line)) {
        // Skip whitespace
        size_t len = line.find_first_not_of(" \t");
        if(len != std::string::npos) {
            line = line.substr(len, line.length() - len);
        }

        // Comment
        if(line[0] == '#' || line.empty()) {
            continue;
        }
        
        std::istringstream sline(line);
        std::string cmd;
        sline >> cmd;
        if(cmd == "mtllib") {
            std::string name;
            sline >> name;
            State::get_instance().material_man->load_wavefront(Path::get("3d/" + name));
        } else if(cmd == "usemtl") {
            std::string name;
            sline >> name;
            objects.front().material = &UnifiedRender::State::get_instance().material_man->load(name);
        } else if(cmd == "o") {
            std::string name;
            sline >> name;
            objects.push_back(WavefrontObj(name));
        } else if(cmd == "v") {
            glm::vec3 vert;
            sline >> vert.x >> vert.y >> vert.z;
            vertices.push_back(vert);
        } else if(cmd == "vt") {
            glm::vec2 tex;
            sline >> tex.x >> tex.y;
            texcoords.push_back(tex);
        } else if(cmd == "vn") {
            glm::vec3 norm;
            sline >> norm.x >> norm.y >> norm.z;
            normals.push_back(norm);
        } else if(cmd == "f") {
            WavefrontFace face = WavefrontFace();
            while(sline.peek() != -1) {
                // Assemble faces - allowing for any number of vertices
                // (and respecting the optional-ity of vt and vn fields)
                int value;
                char ch;

                // Vertices
                sline >> value;
                face.vertices.push_back(value);

                ch = sline.peek();
                if(ch == '/') {
                    sline >> ch;

                    // Texcoords
                    sline >> value;
                    face.texcoords.push_back(value);

                    ch = sline.peek();
                    if(ch == '/') {
                        sline >> ch;
                        
                        // Normals
                        sline >> value;
                        face.normals.push_back(value);

                        ch = sline.peek();
                        if(ch == '/') {
                            print_error("Invalid face declaration");
                            break;
                        }
                    }
                }
            }

            if(face.vertices.size() < 3) {
                print_error("Cannot create polygon - malformed face?");
            } else {
                objects.front().faces.push_back(face);
            }
        } else if(cmd == "s") {
            std::string light_mode;
            sline >> light_mode;

            if(light_mode != "off") {
                print_error("Unsupported light mode %s", light_mode.c_str());
            }
        } else {
            print_error("Unsupported command %s", cmd.c_str());
        }
    }

    // Convert objects into (UnifiedRender) simple objects so we can now use them
    UnifiedRender::Model* final_model = new UnifiedRender::Model();
    for(const auto& obj: objects) {
        // Register each simple object to the model manager
        for(const auto& face: obj.faces) {
            UnifiedRender::SimpleModel* model = new UnifiedRender::SimpleModel(GL_TRIANGLE_FAN);

            // The faces dictate indices for the vertices and we will also subtract 1 because the indexing is 0 based
            if(face.vertices.size() == face.texcoords.size()) {
                for(unsigned int i = 0; i < face.vertices.size(); i++) {
                    model->buffer.push_back(UnifiedRender::OpenGL::PackedData(
                        glm::vec3(vertices[face.vertices[i] - 1]),
                        glm::vec2(texcoords[face.texcoords[i] - 1])
                    ));
                }
            } else {
                for(unsigned int i = 0; i < face.vertices.size(); i++) {
                    model->buffer.push_back(UnifiedRender::OpenGL::PackedData(
                        glm::vec3(vertices[face.vertices[i] - 1]),
                        glm::vec2(0.f, 0.f)
                    ));
                }
            }

            print_info("Created new SimpleModel with %zu vertices", model->buffer.size());
            model->material = obj.material;
            model->upload();

            simple_models.insert(std::make_pair(model, obj.name));
            final_model->simple_models.push_back(model);
        }
    }
    complex_models.insert(std::make_pair(final_model, path));
    return *final_model;
}

const UnifiedRender::Model& UnifiedRender::ModelManager::load_stl(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});

    UnifiedRender::Model* final_model = new UnifiedRender::Model();
    UnifiedRender::SimpleModel* model = new UnifiedRender::SimpleModel(GL_TRIANGLES);

    // TODO: This needs more work
    // 1. we need little endian reading
    uint32_t n_triangles;
    memcpy(&n_triangles, &buffer[80], sizeof(uint32_t));

    for(uint32_t i = 0; i < n_triangles; i++) {
        glm::vec3 vert;

        // TODO: We need to guarantee 2 things:
        // 1. that the floating point is 32-bits
        // 2. little endian
        // 3. ieee787 floating point
        memcpy(&vert.x, &buffer[84 + i * (sizeof(float) * 3)], sizeof(float));
        memcpy(&vert.y, &buffer[88 + i * (sizeof(float) * 3)], sizeof(float));
        memcpy(&vert.z, &buffer[92 + i * (sizeof(float) * 3)], sizeof(float));

        model->buffer.push_back(UnifiedRender::OpenGL::PackedData(
            glm::vec3(vert),
            glm::vec2(0.f, 0.f)
        ));
    }

    simple_models.insert(std::make_pair(model, path));
    final_model->simple_models.push_back(model);
    complex_models.insert(std::make_pair(final_model, path));
    return *final_model;
}

const UnifiedRender::Model& UnifiedRender::ModelManager::load(const std::string& path) {
    auto it = std::find_if(complex_models.begin(), complex_models.end(), [&path](const auto& element) {
        return (element.second == path);
    });

    if(it != complex_models.end()) {
        return *((*it).first);
    }
    
    // Wavefront OBJ loader
    try {
        // TODO: This is too horrible, we need a better solution
        if(path[path.length() - 3] == 's'
        && path[path.length() - 2] == 't'
        && path[path.length() - 1] == 'l') {
            return load_stl(path);
        } else {
            return load_wavefront(path);
        }
    } catch(std::ifstream::failure& e) {
        throw ("Model " + path + " not found");
    }
}