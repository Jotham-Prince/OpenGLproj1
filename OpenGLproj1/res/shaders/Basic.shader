#shader vertex
#version 330 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 color; // Color attribute

uniform mat4 u_Model; // Model matrix uniform

out vec3 v_Color; // Output color for the fragment shader

void main()
{
	gl_Position = u_Model * position;
	v_Color = color;
};

#shader fragment
#version 330 core

in vec3 v_Color; // Input color from vertex shader

out vec4 fragColor;

layout(location = 0) out vec4 color;

void main()
{
	fragColor = vec4(v_Color, 1.0); // Use the color directly
};