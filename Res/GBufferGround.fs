#version 420
layout(location=0)in vec4 V_Texcoord;
layout(location=1)in vec3 V_Normal;
layout(location=2)in vec3 V_WorldPosition;
layout(location=0)out vec4 GBuffer0;
layout(location=1)out vec4 GBuffer1;
layout(location=2)out vec4 GBuffer2;
layout(location=3)out vec4 GBuffer3;
void main(){
    GBuffer0=vec4(V_WorldPosition,0.0);
    GBuffer1=vec4(normalize(V_Normal),0.8);
    GBuffer2=vec4(0.5,0.5,0.5,1.0);
    GBuffer3=vec4(0.0,0.0,0.0,0.0);
}
