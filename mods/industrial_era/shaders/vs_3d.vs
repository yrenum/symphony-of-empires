#version 330 compatibility
precision lowp float;

layout (location = 0) in vec3 m_pos;
layout (location = 1) in vec2 m_texcoord;

provided mat4 view;
provided mat4 projection;
provided mat4 model;

out vec2 v_texcoord;

void main() {
    v_texcoord = m_texcoord;
    gl_Position = projection * view * model * vec4(m_pos, 1.0);
}