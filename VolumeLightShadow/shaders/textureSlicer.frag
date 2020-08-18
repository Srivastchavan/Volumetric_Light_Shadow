#version 330 core

layout(location = 0) out vec4 vFragColor;

smooth in vec3 vUV;	
	
uniform sampler3D volume;	
uniform vec4 color;			 
							

void main()
{
	vFragColor = texture(volume, vUV).rrrr * color ;	 
}
