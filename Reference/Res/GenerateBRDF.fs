#version 420
layout(location=0)in vec4 V_Texcoord;
layout(location=0)out vec4 OutColor0;
const float PI=3.141592;
float RadicalInverse_VDC(uint inIndex) {
    inIndex = (inIndex << 16u) | (inIndex >> 16u);
    inIndex = ((inIndex & 0x55555555u) << 1u) | ((inIndex & 0xAAAAAAAAu) >> 1u);
    inIndex = ((inIndex & 0x33333333u) << 2u) | ((inIndex & 0xCCCCCCCCu) >> 2u);
    inIndex = ((inIndex & 0x0F0F0F0Fu) << 4u) | ((inIndex & 0xF0F0F0F0u) >> 4u);
    inIndex = ((inIndex & 0x00FF00FFu) << 8u) | ((inIndex & 0xFF00FF00u) >> 8u);
    return float(inIndex) * 2.3283064365386963e-10; // / 0x100000000
}
vec2 HammersleyPoint(uint inIndex,uint inTotalSampleCount){
    return vec2(float(inIndex)/float(inTotalSampleCount), RadicalInverse_VDC(inIndex));
}
vec3 ImportanceSampleGGX(vec2 inXi,vec3 inN,float inRoughness){
    float r4=pow(inRoughness,4);
    //Xi->Spherical Coordnate
    float phi=2.0*PI*inXi.x;
    float cosTheta=sqrt((1.0-inXi.y)/(1.0+(r4-1.0)*inXi.y));//D
    float sinTheta=sqrt(1.0-cosTheta*cosTheta);
    //Spherical -> Cartision
    vec3 H;
    H.x=sinTheta*cos(phi);
    H.y=sinTheta*sin(phi);
    H.z=cosTheta;
    //align -> N
    vec3 temp=abs(inN.z)<0.999?vec3(0.0,0.0,1.0):vec3(1.0,0.0,0.0);
    vec3 X=normalize(cross(temp,inN));
    vec3 Y=normalize(cross(inN,X));

    vec3 HFinal=X*H.x+Y*H.y+inN*H.z;
    return normalize(HFinal);
}
float Geometry(float inNDotX,float inRoughness){
    float k=pow(inRoughness,2.0)/2.0;
    return inNDotX/(inNDotX*(1.0-k)+k);
}
vec2 GenerateBRDF(float inNDotV,float inRoughness){
    vec3 N=vec3(0.0,0.0,1.0);
    vec3 V=vec3(sqrt(1.0-inNDotV*inNDotV),0.0,inNDotV);
    const uint sampleCount=1000u;
    float A=0.0;
    float B=0.0;
    for(uint i=0;i<sampleCount;i++){
        vec2 Xi=HammersleyPoint(i,sampleCount);
        vec3 H=ImportanceSampleGGX(Xi,N,inRoughness);
        vec3 L=normalize(2.0*dot(V,H)*H-V);//G

        float NDotL=max(dot(N,L),0.0);
        float NDotH=max(dot(N,H),0.0);
        float HDotV=max(dot(H,V),0.0);
        if(NDotL>0.0){
            float Gv=Geometry(inNDotV,inRoughness);
            float Gl=Geometry(NDotL,inRoughness);
            float G=Gv*Gl;
            float Vis=(G*NDotL)/(NDotH*inNDotV);
            float f=pow(1.0-HDotV,5.0);
            float scale=(1.0-f)*Vis;
            float bias=f*Vis;
            A+=scale;
            B+=bias;
        }
    }
    A=A/float(sampleCount);
    B=B/float(sampleCount);
    return vec2(A,B);
}
void main(){
    vec2 brdf=GenerateBRDF(V_Texcoord.x,V_Texcoord.y);
    OutColor0=vec4(brdf,0.0,0.0);//RG
}