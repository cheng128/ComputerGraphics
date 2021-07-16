#version 330 core

// note: need to pass two thing "position" and "color"
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

// note: output variable to shader.fs
out vec3 vertex_color;
uniform mat4 mvp;

void main()
{
	// [TODO] apply mvp matrix
	gl_Position = mvp * vec4(aPos.x, aPos.y, aPos.z, 1.0);
	vertex_color = aColor;
}

