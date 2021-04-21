#version 450
layout (location=0) in vec2 TexCoords;
layout (location=1) in vec3 Color;
layout (location=0) out vec4 color;



layout (binding=2) uniform sampler2D image;

void main()
{
	color = vec4(Color,1.0) * texture(image,TexCoords);
}

