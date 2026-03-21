#version 420
layout(location=0)in vec4 position;
layout(location=1)in vec4 texcoord;
layout(location=2)in vec4 normal;
layout(binding=0)uniform XXXX{
    mat4 M;
    mat4 V;
    mat4 P;
    mat4 ITM;
    mat4 Reserved[1020];
};
layout(location=0)out vec4 V_Texcoord;
layout(location=1)out vec3 V_Normal;
layout(location=2)out vec3 V_WorldPosition;
void main(){
    V_Normal=normalize(ITM*normal).xyz;
    V_Texcoord=texcoord;
    V_WorldPosition=(M*position).xyz;
    gl_Position=P*V*vec4(V_WorldPosition,1.0);
}
