#version 420
layout(location=0)in vec4 V_Texcoord;
layout(location=1)in vec3 V_Normal;
layout(location=2)in vec3 V_WorldPosition;
layout(location=3)in mat3 V_TBN;
layout(location=0)out vec4 GBuffer0;
layout(location=1)out vec4 GBuffer1;
layout(location=2)out vec4 GBuffer2;
layout(location=3)out vec4 GBuffer3;
layout(binding=7)uniform sampler2D U_Albedo;
layout(binding=8)uniform sampler2D U_Normal;
layout(binding=9)uniform sampler2D U_Emissive;
layout(binding=10)uniform sampler2D U_AO;
layout(binding=11)uniform sampler2D U_Metallic;
layout(binding=12)uniform sampler2D U_Roughness;
vec3 GetNormal(vec2 inUV){
    vec3 normalTS=texture(U_Normal,inUV).xyz;
    normalTS=normalize(normalTS*2.0-vec3(1.0));
    return normalize(V_TBN*normalTS);
}
void main(){
    vec3 albedo=pow(texture(U_Albedo,V_Texcoord.xy).rgb,vec3(2.2));
    vec3 N=GetNormal(V_Texcoord.xy);
    float metallic=texture(U_Metallic,V_Texcoord.xy).r;
    float roughness=max(texture(U_Roughness,V_Texcoord.xy).r,0.01);
    float ao=texture(U_AO,V_Texcoord.xy).r;
    vec3 emissive=texture(U_Emissive,V_Texcoord.xy).rgb;
    GBuffer0=vec4(V_WorldPosition,metallic);
    GBuffer1=vec4(N,roughness);
    GBuffer2=vec4(albedo,ao);
    GBuffer3=vec4(emissive,0.0);
}
