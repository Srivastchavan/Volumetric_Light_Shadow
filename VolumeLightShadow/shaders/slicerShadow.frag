#version 330 core

layout(location = 0) out vec4 vFragColor;	

smooth in vec3 vUV;			
smooth in vec4 vLightUVW;	

//uniforms
uniform sampler3D volume;		
uniform sampler2D shadowTex;	
uniform vec4 color;					

void main()
{  
	
    vec3 lightIntensity =  textureProj(shadowTex, vLightUVW.xyw).xyz;
	
	float density = texture(volume, vUV).r;

	if(density > 0.1) {
		
		float alpha = clamp(density, 0.0, 1.0);
		
		alpha *= color.a;

		vFragColor = vec4(color.xyz*lightIntensity*alpha, alpha);
	} 
}