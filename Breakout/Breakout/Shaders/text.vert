#version 450
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aTexCoords;

layout (location=0) out vec2 TexCoords;

layout (binding=0) uniform UBOP{
	mat4 projection;
	vec3 color;
};

void main(){
	gl_Position = projection * vec4(aPos,1.0);
	TexCoords = aTexCoords;
}

