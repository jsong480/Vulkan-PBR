#version 420
layout(binding=4)uniform sampler2D U_HDRBuffer;
layout(location=0)in vec4 V_Texcoord;
layout(location=0)out vec4 OutColor0;
void main(){
    vec3 hdrColor=texture(U_HDRBuffer,V_Texcoord.xy).rgb;
    vec3 mappedColor=hdrColor/(hdrColor+vec3(1.0));
    OutColor0=vec4(pow(mappedColor,vec3(1.0/2.2)),1.0);
}