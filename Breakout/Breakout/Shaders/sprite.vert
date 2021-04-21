#version 450
layout (location=0) in vec2 position; // <vec2 position, vec2 texCoords>
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aTexCoords;
layout (location=0) out vec2 TexCoords;
layout (location=1) out vec3 Color;

layout (binding=0) uniform UBOP{
	
	mat4 projection;	
}uboP;

struct InstanceData{
	mat4 model;
	vec3 color;
};

layout (binding=1) readonly buffer InstanceBuffer{
	InstanceData instances[];
}instanceBuffer;

void main(){
	TexCoords = aTexCoords;
	mat4 model = instanceBuffer.instances[gl_InstanceIndex].model;
	vec3 color = instanceBuffer.instances[gl_InstanceIndex].color;
	gl_Position = uboP.projection * model * vec4(position,0.0,1.0);
	Color=color;
}