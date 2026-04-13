#version 420
layout(binding=3)uniform SSAOParams{
    vec4 U_CameraWorldPos;
    vec4 U_SSAOParams;
    vec4 U_Proj0;vec4 U_Proj1;vec4 U_Proj2;vec4 U_Proj3;
    vec4 U_InvProj0;vec4 U_InvProj1;vec4 U_InvProj2;vec4 U_InvProj3;
    vec4 U_Kernel[32];
};
layout(binding=4)uniform sampler2D U_DepthBuffer;
layout(binding=5)uniform sampler2D U_NoiseTexture;
layout(location=0)in vec4 V_Texcoord;
layout(location=0)out vec4 OutColor0;

const int KERNEL_SIZE=32;
const float RADIUS=0.5;
const float BIAS=0.025;

float LinearizeDepth(float d){
    float n=U_SSAOParams.x;
    float f=U_SSAOParams.y;
    return n*f/(f-d*(f-n));
}
vec3 ReconstructViewPos(vec2 uv,float depth){
    mat4 invP=mat4(U_InvProj0,U_InvProj1,U_InvProj2,U_InvProj3);
    vec4 clip=vec4(uv.x*2.0-1.0,1.0-uv.y*2.0,depth,1.0);
    vec4 view=invP*clip;
    return view.xyz/view.w;
}
void main(){
    vec2 uv=V_Texcoord.xy;
    float depth=texture(U_DepthBuffer,uv).r;
    if(depth>=1.0){OutColor0=vec4(1.0);return;}
    vec3 fragPos=ReconstructViewPos(uv,depth);
    vec2 texelSize=1.0/vec2(U_SSAOParams.z,U_SSAOParams.w);
    vec3 posR=ReconstructViewPos(uv+vec2(texelSize.x,0),texture(U_DepthBuffer,uv+vec2(texelSize.x,0)).r);
    vec3 posU=ReconstructViewPos(uv+vec2(0,-texelSize.y),texture(U_DepthBuffer,uv+vec2(0,-texelSize.y)).r);
    vec3 normal=normalize(cross(posR-fragPos,posU-fragPos));
    vec2 noiseScale=vec2(U_SSAOParams.z,U_SSAOParams.w)/4.0;
    vec3 randomVec=texture(U_NoiseTexture,uv*noiseScale).xyz;
    vec3 tangent=normalize(randomVec-normal*dot(randomVec,normal));
    vec3 bitangent=cross(normal,tangent);
    mat3 TBN=mat3(tangent,bitangent,normal);
    mat4 proj=mat4(U_Proj0,U_Proj1,U_Proj2,U_Proj3);
    float fragLinearDepth=LinearizeDepth(depth);
    float occlusion=0.0;
    for(int i=0;i<KERNEL_SIZE;i++){
        vec3 samplePos=fragPos+TBN*U_Kernel[i].xyz*RADIUS;
        vec4 offsetClip=proj*vec4(samplePos,1.0);
        offsetClip.xyz/=offsetClip.w;
        vec2 sampleUV=vec2(offsetClip.x*0.5+0.5,0.5-offsetClip.y*0.5);
        float sampleBufDepth=texture(U_DepthBuffer,sampleUV).r;
        float sampleLinearDepth=LinearizeDepth(sampleBufDepth);
        float sampleExpectedDepth=-samplePos.z;
        float rangeCheck=smoothstep(0.0,1.0,RADIUS/(abs(fragLinearDepth-sampleLinearDepth)+0.0001));
        occlusion+=(sampleLinearDepth<=sampleExpectedDepth-BIAS?1.0:0.0)*rangeCheck;
    }
    occlusion=1.0-occlusion/float(KERNEL_SIZE);
    OutColor0=vec4(vec3(occlusion),1.0);
}
