#version 420
layout(binding=3)uniform XXX{
    vec4 CameraWorldPosition;
    vec4 FXAAParams;
    vec4 Reserved[1022];
};
layout(binding=4)uniform sampler2D U_InputTexture;
layout(location=0)in vec4 V_Texcoord;
layout(location=0)out vec4 OutColor0;
float Luma(vec3 c){
    return dot(c,vec3(0.299,0.587,0.114));
}
void main(){
    vec2 uv=V_Texcoord.xy;
    vec2 texelSize=vec2(FXAAParams.y,FXAAParams.z);
    vec3 color=texture(U_InputTexture,uv).rgb;
    if(FXAAParams.x<0.5){
        OutColor0=vec4(color,1.0);
        return;
    }
    float lumaM=Luma(color);
    float lumaN=Luma(texture(U_InputTexture,uv+vec2(0,-texelSize.y)).rgb);
    float lumaS=Luma(texture(U_InputTexture,uv+vec2(0,texelSize.y)).rgb);
    float lumaE=Luma(texture(U_InputTexture,uv+vec2(texelSize.x,0)).rgb);
    float lumaW=Luma(texture(U_InputTexture,uv+vec2(-texelSize.x,0)).rgb);
    float lumaMin=min(lumaM,min(min(lumaN,lumaS),min(lumaE,lumaW)));
    float lumaMax=max(lumaM,max(max(lumaN,lumaS),max(lumaE,lumaW)));
    float lumaRange=lumaMax-lumaMin;
    if(lumaRange<max(0.0312,lumaMax*0.125)){
        OutColor0=vec4(color,1.0);
        return;
    }
    float lumaNW=Luma(texture(U_InputTexture,uv+vec2(-texelSize.x,-texelSize.y)).rgb);
    float lumaNE=Luma(texture(U_InputTexture,uv+vec2(texelSize.x,-texelSize.y)).rgb);
    float lumaSW=Luma(texture(U_InputTexture,uv+vec2(-texelSize.x,texelSize.y)).rgb);
    float lumaSE=Luma(texture(U_InputTexture,uv+vec2(texelSize.x,texelSize.y)).rgb);
    float edgeH=abs(-2.0*lumaW+lumaNW+lumaSW)+abs(-2.0*lumaM+lumaN+lumaS)*2.0+abs(-2.0*lumaE+lumaNE+lumaSE);
    float edgeV=abs(-2.0*lumaN+lumaNW+lumaNE)+abs(-2.0*lumaM+lumaW+lumaE)*2.0+abs(-2.0*lumaS+lumaSW+lumaSE);
    bool isHorz=edgeH>=edgeV;
    float luma1=isHorz?lumaN:lumaW;
    float luma2=isHorz?lumaS:lumaE;
    float grad1=abs(luma1-lumaM);
    float grad2=abs(luma2-lumaM);
    float stepLen=isHorz?texelSize.y:texelSize.x;
    float lumaLocalAvg;
    if(grad1>=grad2){
        stepLen=-stepLen;
        lumaLocalAvg=0.5*(luma1+lumaM);
    }else{
        lumaLocalAvg=0.5*(luma2+lumaM);
    }
    float gradScaled=max(grad1,grad2)*0.25;
    vec2 edgeUV=uv;
    if(isHorz) edgeUV.y+=stepLen*0.5;
    else edgeUV.x+=stepLen*0.5;
    vec2 step=isHorz?vec2(texelSize.x,0):vec2(0,texelSize.y);
    vec2 uv1=edgeUV-step;
    vec2 uv2=edgeUV+step;
    float lumaEnd1=Luma(texture(U_InputTexture,uv1).rgb)-lumaLocalAvg;
    float lumaEnd2=Luma(texture(U_InputTexture,uv2).rgb)-lumaLocalAvg;
    bool reached1=abs(lumaEnd1)>=gradScaled;
    bool reached2=abs(lumaEnd2)>=gradScaled;
    for(int i=0;i<12;i++){
        if(!reached1){uv1-=step;lumaEnd1=Luma(texture(U_InputTexture,uv1).rgb)-lumaLocalAvg;reached1=abs(lumaEnd1)>=gradScaled;}
        if(!reached2){uv2+=step;lumaEnd2=Luma(texture(U_InputTexture,uv2).rgb)-lumaLocalAvg;reached2=abs(lumaEnd2)>=gradScaled;}
        if(reached1&&reached2)break;
    }
    float dist1=isHorz?(uv.x-uv1.x):(uv.y-uv1.y);
    float dist2=isHorz?(uv2.x-uv.x):(uv2.y-uv.y);
    float distMin=min(dist1,dist2);
    float edgeLen=dist1+dist2;
    float pixelOffset=-distMin/edgeLen+0.5;
    bool isSmaller=lumaM<lumaLocalAvg;
    bool correctVar=((dist1<dist2?lumaEnd1:lumaEnd2)<0.0)!=isSmaller;
    float finalOff=correctVar?pixelOffset:0.0;
    float lumaAvg=(1.0/12.0)*(2.0*(lumaN+lumaS+lumaE+lumaW)+lumaNW+lumaNE+lumaSW+lumaSE);
    float subPixel=clamp(abs(lumaAvg-lumaM)/lumaRange,0.0,1.0);
    subPixel=smoothstep(0.0,1.0,subPixel);
    subPixel=subPixel*subPixel*0.75;
    finalOff=max(finalOff,subPixel);
    vec2 finalUV=uv;
    if(isHorz) finalUV.y+=finalOff*stepLen;
    else finalUV.x+=finalOff*stepLen;
    OutColor0=vec4(texture(U_InputTexture,finalUV).rgb,1.0);
}
