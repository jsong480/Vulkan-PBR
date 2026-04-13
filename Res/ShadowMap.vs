#version 420
layout(location=0)in vec4 position;
layout(location=1)in vec4 texcoord;
layout(location=2)in vec4 normal;
layout(binding=0)uniform XXXX{
    mat4 M;
    mat4 V;
    mat4 P;
    mat4 Reserved[1021];
};
void main(){
    gl_Position=P*V*M*position;
}
