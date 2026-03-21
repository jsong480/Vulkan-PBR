#version 420
layout(location=0)in vec4 position;
layout(location=1)in vec4 texcoord;
layout(location=2)in vec4 normal;
layout(location=3)in vec4 tangent;
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
layout(location=3)out mat3 V_TBN;
void main(){
    V_Normal=normalize(ITM*normal).xyz;
    V_Texcoord=texcoord;
    V_WorldPosition=(M*position).xyz;
    vec3 t=normalize(vec3(M*vec4(tangent.xyz,0.0)));
    vec3 b=normalize(cross(V_Normal,t));
    V_TBN=mat3(t,b,V_Normal);
    gl_Position=P*V*vec4(V_WorldPosition,1.0);
}
