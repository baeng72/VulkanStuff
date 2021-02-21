#version 450
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;


layout (location = 0) out vec2 TexCoords;

layout (binding=0) uniform UBO{	
	mat4 view;	
	mat4 projection;	
}ubo;


struct InstanceData{
	mat4 model;

};

layout (binding = 1) readonly buffer InstanceBuffer{
	InstanceData instances[];
}instanceBuffer;
	

void main()
{
	mat4 model = instanceBuffer.instances[gl_InstanceIndex].model; 	
	
	gl_Position = ubo.projection * ubo.view * model* vec4(aPos, 1.0);				
	
	TexCoords = aTexCoords;
}