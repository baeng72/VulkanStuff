#version 450
layout (location=0) in vec3 position; //<vec2 position, vec2 texCoords>
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aTexCoords;
layout (location=0) out vec2 TexCoords;
layout (location=1) out vec4 ParticleColor;

layout (binding=0) uniform UBOP{	
	mat4 projection;	
};

struct InstanceData{
	vec2 offset;
	vec4 color;
};

layout (binding=1) readonly buffer InstanceBuffer{
	InstanceData instances[];
}instanceBuffer;

void main(){
	float scale=10.f;
	TexCoords = aTexCoords;
	vec2 offset = instanceBuffer.instances[gl_InstanceIndex].offset;
	vec4 color = instanceBuffer.instances[gl_InstanceIndex].color;
	gl_Position = projection * vec4((position.xy*scale)+offset,0.0,1.0);
	ParticleColor = color;
}