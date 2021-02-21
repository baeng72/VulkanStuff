#version 450


layout (location = 0) in vec2 TexCoords;


layout (location=0) out vec4 FragColor;	//binding 0 is our color attachment

layout (binding=2) uniform sampler2D textureMap;
	
void main()
{
	
	FragColor = texture(textureMap,TexCoords);
}