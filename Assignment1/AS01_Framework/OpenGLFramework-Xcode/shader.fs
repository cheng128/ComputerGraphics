#version 330 core

out vec4 FragColor;
// note: from shader.vs
in vec3 vertex_color;

void main() {
    // [TODO]
	FragColor = vec4(vertex_color, 1.0f);
}
