#version 420
layout(binding=4)uniform sampler2D U_HDRBuffer;
layout(binding=5)uniform sampler2D U_BloomTexture;
layout(binding=6)uniform sampler2D U_SSAOTexture;
layout(location=0)in vec4 V_Texcoord;
layout(location=0)out vec4 OutColor0;
void main(){
    vec3 hdrColor=texture(U_HDRBuffer,V_Texcoord.xy).rgb;
    vec3 bloomColor=texture(U_BloomTexture,V_Texcoord.xy).rgb;
    float ssao=texture(U_SSAOTexture,V_Texcoord.xy).r;
    hdrColor=max(hdrColor,vec3(0.0));
    bloomColor=max(bloomColor,vec3(0.0));
    hdrColor*=ssao;
    hdrColor+=bloomColor*0.32;
    hdrColor=min(hdrColor,vec3(1e6));
    vec3 mappedColor=hdrColor/(hdrColor+vec3(1.0));
    mappedColor=clamp(mappedColor,vec3(0.0),vec3(1.0));
    OutColor0=vec4(pow(mappedColor,vec3(1.0/2.2)),1.0);
}
