#version 420
layout(location=0)in vec4 position;

layout(binding=0)uniform X{
    mat4 M;
    mat4 V;
    mat4 P;
    mat4 ITM;
    mat4 Reserved[1020];
};
layout(location=0)out vec3 V_Texcoord;
void main(){
    V_Texcoord=position.xyz;
    gl_Position=P*V*M*position;
}