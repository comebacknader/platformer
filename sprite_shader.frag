#version 330 core

out vec4 FragColor;
uniform sampler2D texture1;

void main()
{
	FragColor = vec4(0.9, 0.2, 0.3, 1.0);
}