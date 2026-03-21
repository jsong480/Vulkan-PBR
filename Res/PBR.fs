#version 420
layout(location=0)in vec4 V_Texcoord;
layout(location=1)in vec3 V_Normal;
layout(location=2)in vec3 V_WorldPosition;
layout(location=0)out vec4 OutColor0;
layout(binding=3)uniform XXX{
    vec4 CameraWorldPosition;
    vec4 Reserved[1023];
};
layout(binding=4)uniform samplerCube U_SkyBox;
layout(binding=5)uniform samplerCube U_PrefilteredColor;
layout(binding=6)uniform samplerCube U_DiffuseIrradiance;
layout(binding=7)uniform sampler2D U_BRDFLUT;
const float PI=3.141592;
vec3 F(vec3 inF0,vec3 inH,vec3 inV){
    float HDotV=max(0.0,dot(inH,inV));
    return inF0+(vec3(1.0)-inF0)*pow(1-HDotV,5.0);
}
vec3 FRoughness(vec3 inF0,float inNDotV,float inRoughness){
    return inF0+(max(vec3(1.0-inRoughness),inF0)-inF0)*pow(1.0-inNDotV,5.0);
}
float NDF(vec3 inN,vec3 inH,float inRoughness){
    float r4=pow(inRoughness,4.0);
    float a=max(dot(inN,inH),0.0);
    float a2=a*a;
    float b=r4-1.0;
    float c=a2*b+1.0;
    float c2=pow(c,2.0);
    return r4/(PI*c2);
}
float Geometry(float inNDotX,float inRoughness){
    float k=pow(inRoughness+1.0,2.0)/8.0;
    return inNDotX/(inNDotX*(1.0-k)+k);
}
void main(){
    vec3 L=vec3(1.0,1.0,1.0);
    vec3 N=normalize(V_Normal);
    vec3 V=normalize(CameraWorldPosition.xyz-V_WorldPosition);
    vec3 H=normalize(L+V);
    vec3 R=normalize(2.0*dot(V,N)*N-V);
    float NDotL=max(dot(N,L),0.0);
    float NDotV=max(dot(N,V),0.0);
    vec3 FinalColor=vec3(0.0);
    vec3 F0=vec3(0.04);
    vec3 albedo=vec3(1.0,0.15,0.1);
    float roughness=max(0.08,abs(sin(V_Texcoord.x*6.283185*0.25)));
    float metallic=0.6;
    F0=mix(F0,albedo,metallic);
    {
        vec3 Ks=F(F0,H,V);
        vec3 Kd=vec3(1.0)-Ks;
        Kd*=1.0-metallic;
        float D=NDF(N,H,roughness);
        float Gl=Geometry(NDotL,roughness);
        float Gv=Geometry(NDotV,roughness);
        float G=Gl*Gv;
        vec3 specularColor=(D*Ks*G)/(4.0*NDotV*NDotL+0.0001);
        vec3 diffuseColor=Kd*albedo/PI;
        FinalColor+=(diffuseColor+specularColor)*vec3(1.0)*2.0*NDotL;
    }
    {
        vec3 Ks=FRoughness(F0,NDotV,roughness);
        vec3 Kd=vec3(1.0)-Ks;
        Kd*=1.0-metallic;
        vec3 diffuseLight=texture(U_DiffuseIrradiance,N).rgb;
        vec3 ambientDiffuse=Kd*diffuseLight*albedo;
        vec2 brdf=texture(U_BRDFLUT,vec2(NDotV,roughness)).rg;
        vec3 prefilteredColor=textureLod(U_PrefilteredColor,R,roughness*4.0).rgb;
        vec3 ambientSpecular=prefilteredColor*(F0*brdf.x+brdf.y);
        FinalColor+=ambientDiffuse+ambientSpecular;
    }
    OutColor0=vec4(FinalColor,1.0);
}
