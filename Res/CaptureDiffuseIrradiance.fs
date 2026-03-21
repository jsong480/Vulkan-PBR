#version 420
layout(location=0)out vec4 OutColor0;
layout(location=0)in vec3 V_Texcoord;
layout(binding=4)uniform samplerCube U_SkyBox;
const float PI=3.141592;
void main(){
    vec3 N=normalize(V_Texcoord);
    vec3 Y=vec3(0.0,1.0,0.0);
    vec3 X=normalize(cross(Y,N));
    Y=normalize(cross(N,X));
    
    vec3 precomputedLight=vec3(0.0);
    float sampleCount=0.0;
    float phiStep=2.0*PI/1000.0;
    float thetaStep=0.5*PI/200.0;
    for(float phi=0.0;phi<2.0*PI;phi+=phiStep){
        for(float theta=0.0;theta<0.5*PI;theta+=thetaStep){
            vec3 localL=vec3(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta));
            vec3 L=localL.x*X+localL.y*Y+localL.z*N;
            precomputedLight+=texture(U_SkyBox,L).rgb*cos(theta)*sin(theta);
            sampleCount++;
        }
    }
    precomputedLight=PI*precomputedLight*(1.0/sampleCount);
    OutColor0=vec4(precomputedLight,1.0);
}
