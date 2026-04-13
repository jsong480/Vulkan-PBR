#version 420
layout(binding=3)uniform BlurParams{
    vec4 U_CameraWorldPos;
    vec4 U_BlurDirection;
};
layout(binding=4)uniform sampler2D U_InputTexture;
layout(location=0)in vec4 V_Texcoord;
layout(location=0)out vec4 OutColor0;
void main(){
    float weight[5]=float[](0.227027,0.1945946,0.1216216,0.054054,0.016216);
    vec2 texOffset=U_BlurDirection.xy;
    vec3 result=texture(U_InputTexture,V_Texcoord.xy).rgb*weight[0];
    for(int i=1;i<5;i++){
        result+=texture(U_InputTexture,V_Texcoord.xy+texOffset*float(i)).rgb*weight[i];
        result+=texture(U_InputTexture,V_Texcoord.xy-texOffset*float(i)).rgb*weight[i];
    }
    result=max(result,vec3(0.0));
    result=min(result,vec3(60000.0));
    OutColor0=vec4(result,1.0);
}
