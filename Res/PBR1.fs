#version 420
// PBR1: Full textured metallic-roughness workflow
// PBR1：完整贴图版金属度-粗糙度工作流
layout(location=0)in vec4 V_Texcoord;
layout(location=1)in vec3 V_Normal;
layout(location=2)in vec3 V_WorldPosition;
layout(location=3)in mat3 V_TBN;
layout(location=0)out vec4 OutColor0;
layout(binding=3)uniform XXX{
    vec4 CameraWorldPosition;
    mat4 LightVP;
    vec4 _pad5;vec4 _pad6;vec4 _pad7;vec4 _pad8;
    vec4 PointLightParams;
    vec4 PointLights[16];
    vec4 Reserved[998];
};
layout(binding=4)uniform samplerCube U_PrefilteredColor;
layout(binding=5)uniform samplerCube U_DiffuseIrradiance;
layout(binding=6)uniform sampler2D U_BRDFLUT;
layout(binding=7)uniform sampler2D U_Albedo;
layout(binding=8)uniform sampler2D U_Normal;
layout(binding=9)uniform sampler2D U_Emissive;
layout(binding=10)uniform sampler2D U_AO;
layout(binding=11)uniform sampler2D U_Metallic;
layout(binding=12)uniform sampler2D U_Roughness;
layout(binding=13)uniform sampler2D U_ShadowMap;
const float PI=3.141592;
// sRGB -> linear color space
// sRGB -> 线性空间
vec3 GammaDecode(vec3 inSRGB){
    return pow(inSRGB,vec3(2.2));
}
// Fresnel (Schlick approximation)
// 菲涅耳项（Schlick近似）
vec3 F(vec3 inF0,vec3 inH,vec3 inV){
    float HDotV=max(0.0,dot(inH,inV));
    return inF0+(vec3(1.0)-inF0)*pow(1-HDotV,5.0);
}
// Fresnel variant used in IBL ambient specular
// IBL间接镜面使用的菲涅耳变体
vec3 FRoughness(vec3 inF0,float inNDotV,float inRoughness){
    return inF0+(max(vec3(1.0-inRoughness),inF0)-inF0)*pow(1.0-inNDotV,5.0);
}
// D term: GGX normal distribution function
// D项：GGX法线分布函数
float NDF(vec3 inN,vec3 inH,float inRoughness){
    float r4=pow(inRoughness,4.0);
    float a=max(dot(inN,inH),0.0);
    float a2=a*a;
    float b=r4-1.0;
    float c=a2*b+1.0;
    float c2=pow(c,2.0);
    return r4/(PI*c2);
}
// G term: Schlick-GGX geometry function
// G项：Schlick-GGX几何遮蔽函数
float Geometry(float inNDotX,float inRoughness){
    float k=pow(inRoughness+1.0,2.0)/8.0;
    return inNDotX/(inNDotX*(1.0-k)+k);
}
// Sample tangent-space normal map and transform to world space
// 采样切线空间法线并转换到世界空间
vec3 GetNormal(vec2 inUV){
    vec3 normalTS=texture(U_Normal,inUV).xyz;
    normalTS=normalize(normalTS*2.0-vec3(1.0));
    return normalize(V_TBN*normalTS);
}
float CalcShadow(vec3 inWorldPos,vec3 inN,vec3 inL){
    vec4 lightSpacePos=LightVP*vec4(inWorldPos,1.0);
    vec3 projCoords=lightSpacePos.xyz/lightSpacePos.w;
    projCoords.x=projCoords.x*0.5+0.5;
    projCoords.y=projCoords.y*(-0.5)+0.5;
    if(projCoords.z>1.0||projCoords.x<0.0||projCoords.x>1.0||projCoords.y<0.0||projCoords.y>1.0)
        return 0.0;
    float bias=max(0.005*(1.0-dot(inN,inL)),0.001);
    float shadow=0.0;
    vec2 texelSize=1.0/textureSize(U_ShadowMap,0);
    for(int x=-1;x<=1;x++){
        for(int y=-1;y<=1;y++){
            float depth=texture(U_ShadowMap,projCoords.xy+vec2(x,y)*texelSize).r;
            shadow+=projCoords.z-bias>depth?1.0:0.0;
        }
    }
    return shadow/9.0;
}
void main(){
    // L: light direction (demo uses fixed direction)
    // L：光线方向（演示使用固定方向）
    vec3 L=vec3(1.0,1.0,1.0);
    // N/V/H/R are core shading directions
    // N/V/H/R 是着色核心方向向量
    vec3 N=GetNormal(V_Texcoord.xy);
    vec3 V=normalize(CameraWorldPosition.xyz-V_WorldPosition);
    vec3 H=normalize(L+V);
    vec3 R=normalize(2.0*dot(V,N)*N-V);
    float NDotL=max(dot(N,L),0.0);
    float NDotV=max(dot(N,V),0.0);
    vec3 FinalColor=vec3(0.0);
    vec3 F0=vec3(0.04);
    // Material inputs from textures
    // 材质输入来自贴图
    vec3 albedo=GammaDecode(texture(U_Albedo,V_Texcoord.xy).rgb);
    float roughness=max(texture(U_Roughness,V_Texcoord.xy).r,0.01);
    float metallic=texture(U_Metallic,V_Texcoord.xy).r;
    // Metallic workflow: metals use albedo as specular F0
    // 金属工作流：金属表面会使用albedo作为F0
    F0=mix(F0,albedo,metallic);
    float shadow=CalcShadow(V_WorldPosition,N,normalize(L));
    // Direct lighting: Cook-Torrance BRDF (modulated by shadow)
    // 直接光：Cook-Torrance BRDF（受阴影调制）
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
        FinalColor+=(1.0-shadow)*(diffuseColor+specularColor)*vec3(1.0)*2.0*NDotL;
    }
    int numPL=int(PointLightParams.x);
    for(int i=0;i<8&&i<numPL;i++){
        vec3 plPos=PointLights[i*2].xyz;
        float plRadius=PointLights[i*2].w;
        vec3 plColor=PointLights[i*2+1].xyz;
        float plIntensity=PointLights[i*2+1].w;
        vec3 Lp=plPos-V_WorldPosition;
        float dist=length(Lp);
        Lp=normalize(Lp);
        float atten=clamp(1.0-dist/plRadius,0.0,1.0);
        atten*=atten;
        vec3 Hp=normalize(Lp+V);
        float NDotLp=max(dot(N,Lp),0.0);
        vec3 Ksp=F(F0,Hp,V);
        vec3 Kdp=(vec3(1.0)-Ksp)*(1.0-metallic);
        float Dp=NDF(N,Hp,roughness);
        float Glp=Geometry(NDotLp,roughness);
        float Gvp=Geometry(NDotV,roughness);
        vec3 specP=(Dp*Ksp*Glp*Gvp)/(4.0*NDotV*NDotLp+0.0001);
        vec3 diffP=Kdp*albedo/PI;
        FinalColor+=(diffP+specP)*plColor*plIntensity*atten*NDotLp;
    }
    // Indirect lighting (IBL): diffuse irradiance + specular prefilter + BRDF LUT
    // 间接光（IBL）：漫反射辐照度 + 镜面预滤波 + BRDF查找表
    {
        vec3 Ks=FRoughness(F0,NDotV,roughness);
        vec3 Kd=vec3(1.0)-Ks;
        Kd*=1.0-metallic;
        vec3 diffuseLight=texture(U_DiffuseIrradiance,N).rgb;
        vec3 ambientDiffuse=Kd*diffuseLight*albedo;
        vec2 brdf=texture(U_BRDFLUT,vec2(NDotV,roughness)).rg;
        vec3 prefilteredColor=textureLod(U_PrefilteredColor,R,roughness*4.0).rgb;
        vec3 ambientSpecular=prefilteredColor*(F0*brdf.x+brdf.y);
        float ao=texture(U_AO,V_Texcoord.xy).r;
        float ambientShadow=1.0-shadow*0.3;
        FinalColor+=(ambientDiffuse+ambientSpecular)*ao*ambientShadow;
    }
    // Emissive is added after lighting
    // 自发光在光照计算后直接叠加
    vec3 emissive=texture(U_Emissive,V_Texcoord.xy).rgb;
    OutColor0=vec4(FinalColor+emissive,1.0);
}
