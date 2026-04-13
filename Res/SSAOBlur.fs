#version 420
layout(binding=4)uniform sampler2D U_SSAOTexture;
layout(location=0)in vec4 V_Texcoord;
layout(location=0)out vec4 OutColor0;
void main(){
    vec2 texelSize=1.0/textureSize(U_SSAOTexture,0);
    float result=0.0;
    for(int x=-2;x<2;x++){
        for(int y=-2;y<2;y++){
            result+=texture(U_SSAOTexture,V_Texcoord.xy+vec2(float(x),float(y))*texelSize).r;
        }
    }
    OutColor0=vec4(vec3(result/16.0),1.0);
}
