#version 420
layout(location=0)in vec4 position;
layout(location=1)in vec4 texcoord;
layout(location=0)out vec4 V_Texcoord;
void main(){
    V_Texcoord=texcoord;
    gl_Position=position;
}