#version 330 core
  
layout(location = 0) in vec3 vVertex; 
  

uniform mat4 MVP;	
uniform mat4 S;		

smooth out vec3 vUV;		
smooth out vec4 vLightUVW;	

void main()
{  
	vLightUVW = S*vec4(vVertex,1);

	gl_Position = MVP*vec4(vVertex,1);

	vUV = vVertex + vec3(0.5);
}