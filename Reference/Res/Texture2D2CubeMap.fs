#version 420
layout(location=0)out vec4 OutColor0;
layout(location=0)in vec3 V_Texcoord;
layout(binding=4)uniform sampler2D U_HDRITexture;
const float PI=3.141592;
vec2 Vec3Texcoord2UV(vec3 inTexcoord){
    float arcsiny=asin(inTexcoord.y);//
    arcsiny/=PI;
    arcsiny+=0.5;// -> v
    float fracZX=inTexcoord.z/inTexcoord.x;
    float arctanzx=atan(inTexcoord.z,inTexcoord.x);//-PI~PI
    arctanzx/=PI;//-1~1
    arctanzx*=0.5;//-0.5~0.5
    arctanzx+=0.5;//0.0~1.0
    return vec2(arctanzx,arcsiny);
}
void main(){
    vec3 texcoord=normalize(V_Texcoord);
    vec2 uv=Vec3Texcoord2UV(texcoord);
    vec3 hdriColor=texture(U_HDRITexture,uv).rgb;//HDR Color
    OutColor0=vec4(hdriColor,1.0);
}