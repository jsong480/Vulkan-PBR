#version 420
layout(location=0)out vec4 OutColor0;
layout(location=0)in vec3 V_Texcoord;
layout(binding=4)uniform sampler2D U_HDRITexture;
const float PI=3.141592;
vec2 Vec3Texcoord2UV(vec3 inTexcoord){
    float arcsiny=asin(inTexcoord.y);
    arcsiny/=PI;
    arcsiny+=0.5;
    float arctanzx=atan(inTexcoord.z,inTexcoord.x);
    arctanzx/=PI;
    arctanzx*=0.5;
    arctanzx+=0.5;
    return vec2(arctanzx,arcsiny);
}
void main(){
    vec3 texcoord=normalize(V_Texcoord);
    vec2 uv=Vec3Texcoord2UV(texcoord);
    vec3 hdriColor=texture(U_HDRITexture,uv).rgb;
    OutColor0=vec4(hdriColor,1.0);
}
