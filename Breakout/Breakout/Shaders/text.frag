#version 450
layout (location=0) in vec2 TextCoords;
layout (location=0) out vec4 FragColor;

layout (binding=0) uniform UBOP{
	mat4 projection;
	vec3 textColor;
};

layout (binding=1) uniform sampler2D text;

void main(){
	vec4 sampled = vec4(1.0,1.0,1.0,texture(text,TextCoords).r);
	FragColor = vec4(textColor,1.0)*sampled;
}