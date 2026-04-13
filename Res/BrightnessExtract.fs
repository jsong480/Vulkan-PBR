#version 420
layout(binding=3)uniform XXX{
    vec4 CameraWorldPosition;
    vec4 BloomParams;
    vec4 Reserved[1022];
};
layout(binding=4)uniform sampler2D U_HDRBuffer;
layout(location=0)in vec4 V_Texcoord;
layout(location=0)out vec4 OutColor0;
void main(){
    vec2 uv=V_Texcoord.xy;
    vec2 ts=BloomParams.xy;
    vec3 acc=vec3(0.0);
    for(int dy=-2;dy<=2;++dy){
        for(int dx=-2;dx<=2;++dx){
            acc+=texture(U_HDRBuffer,uv+vec2(float(dx),float(dy))*ts).rgb;
        }
    }
    vec3 hdrColor=acc*(1.0/25.0);
    hdrColor=max(hdrColor,vec3(0.0));
    hdrColor=min(hdrColor,vec3(60000.0));
    float lum=dot(hdrColor,vec3(0.2126,0.7152,0.0722));
    float threshold=0.58;
    float knee=0.45;
    float soft=lum-threshold+knee;
    soft=clamp(soft,0.0,2.0*knee);
    soft=soft*soft/(4.0*knee+1e-4);
    float contrib=max(lum-threshold,0.0)+soft;
    float denom=max(lum,1e-4);
    vec3 bloom=hdrColor*(contrib/denom);
    bloom=min(bloom,vec3(60000.0));
    float bl=dot(bloom,vec3(0.2126,0.7152,0.0722));
    float maxBloomLum=10.0;
    if(bl>maxBloomLum)bloom*=maxBloomLum/(bl+1e-4);
    OutColor0=vec4(bloom,1.0);
}
