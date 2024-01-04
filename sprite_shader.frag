#version 330 core

out vec4 FragColor;
uniform sampler2D texture1;

void main()
{
	FragColor = vec4(0.5, 0.3, 0.8, 1.0);
}