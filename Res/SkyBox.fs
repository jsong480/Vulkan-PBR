#version 420
layout(location=0)out vec4 OutColor0;
layout(location=0)in vec3 V_Texcoord;
layout(binding=4)uniform samplerCube U_SkyBox;
void main(){
    vec3 texcoord=normalize(V_Texcoord);
    vec3 hdrColor=texture(U_SkyBox,texcoord).rgb;
    OutColor0=vec4(hdrColor,1.0);
}
