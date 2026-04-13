#version 420
layout(binding=3)uniform XXX{
    vec4 CameraWorldPosition;
    mat4 LightVP;
    vec4 InvVP0;vec4 InvVP1;vec4 InvVP2;vec4 InvVP3;
    vec4 PointLightParams;
    vec4 PointLights[16];
    vec4 Reserved[998];
};
layout(binding=4)uniform sampler2D U_GBufPosMetallic;
layout(binding=5)uniform sampler2D U_GBufNormRoughness;
layout(binding=6)uniform sampler2D U_GBufAlbedoAO;
layout(binding=7)uniform sampler2D U_GBufEmissive;
layout(binding=8)uniform sampler2D U_DepthBuffer;
layout(binding=9)uniform sampler2D U_ShadowMap;
layout(binding=10)uniform samplerCube U_PrefilteredColor;
layout(binding=11)uniform samplerCube U_DiffuseIrradiance;
layout(binding=12)uniform sampler2D U_BRDFLUT;
layout(binding=13)uniform samplerCube U_SkyBox;
layout(location=0)in vec4 V_Texcoord;
layout(location=0)out vec4 OutColor0;
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
    vec2 uv=V_Texcoord.xy;
    float depth=texture(U_DepthBuffer,uv).r;
    if(depth>=1.0){
        mat4 invVP=mat4(InvVP0,InvVP1,InvVP2,InvVP3);
        vec4 clipPos=vec4(uv.x*2.0-1.0,1.0-uv.y*2.0,1.0,1.0);
        vec4 worldPos=invVP*clipPos;
        vec3 viewDir=normalize(worldPos.xyz/worldPos.w-CameraWorldPosition.xyz);
        OutColor0=vec4(texture(U_SkyBox,viewDir).rgb,1.0);
        return;
    }
    vec4 gbuf0=texture(U_GBufPosMetallic,uv);
    vec4 gbuf1=texture(U_GBufNormRoughness,uv);
    vec4 gbuf2=texture(U_GBufAlbedoAO,uv);
    vec4 gbuf3=texture(U_GBufEmissive,uv);
    vec3 worldPos=gbuf0.xyz;
    float metallic=gbuf0.w;
    vec3 N=normalize(gbuf1.xyz);
    float roughness=max(gbuf1.w,0.01);
    vec3 albedo=gbuf2.rgb;
    float ao=gbuf2.a;
    vec3 emissive=gbuf3.rgb;
    vec3 L=vec3(1.0,1.0,1.0);
    vec3 V=normalize(CameraWorldPosition.xyz-worldPos);
    vec3 H=normalize(L+V);
    vec3 R=normalize(2.0*dot(V,N)*N-V);
    float NDotL=max(dot(N,L),0.0);
    float NDotV=max(dot(N,V),0.0);
    vec3 F0=mix(vec3(0.04),albedo,metallic);
    float shadow=CalcShadow(worldPos,N,normalize(L));
    vec3 FinalColor=vec3(0.0);
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
        vec3 Lp=plPos-worldPos;
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
    {
        vec3 Ks=FRoughness(F0,NDotV,roughness);
        vec3 Kd=vec3(1.0)-Ks;
        Kd*=1.0-metallic;
        vec3 diffuseLight=texture(U_DiffuseIrradiance,N).rgb;
        vec3 ambientDiffuse=Kd*diffuseLight*albedo;
        vec2 brdf=texture(U_BRDFLUT,vec2(NDotV,roughness)).rg;
        vec3 prefilteredColor=textureLod(U_PrefilteredColor,R,roughness*4.0).rgb;
        vec3 ambientSpecular=prefilteredColor*(F0*brdf.x+brdf.y);
        float ambientShadow=1.0-shadow*0.3;
        FinalColor+=(ambientDiffuse+ambientSpecular)*ao*ambientShadow;
    }
    OutColor0=vec4(FinalColor+emissive,1.0);
}
