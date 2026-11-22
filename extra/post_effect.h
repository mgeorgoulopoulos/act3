#pragma once
namespace extra {
float dramaAt(unsigned demoTime);
float postIntensity(unsigned demoTime);
void postProcess(int width,int height,unsigned demoTime);
void releasePostProcess();
}
