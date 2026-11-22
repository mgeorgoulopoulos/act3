#define NOMINMAX
#define MA_NO_ENGINE
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENCODING
#define MINIAUDIO_IMPLEMENTATION
#include "../third_party/miniaudio/miniaudio.h"
#include "music.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstring>

namespace {
constexpr unsigned rate=44100, window=2048;
ma_device device;
float* pcm;
ma_uint64 length;
std::atomic<std::uint64_t> submitted{};
std::uint64_t latency{}, lastAnalysis{}, lastPulse{};
bool initialized{}, playing{};
music::Features analysis;
std::array<float,window/2> previous{};
float averageFlux{};
float averageOrchestralFlux{};
std::uint64_t lastOrchestralPulse{};

void output(ma_device*,void* destination,const void*,ma_uint32 count)
{
    const auto first=submitted.load(std::memory_order_relaxed);
    const auto available=first<length ? std::min<ma_uint64>(count,length-first) : 0;
    if (available) std::memcpy(destination,pcm+first*2,available*2*sizeof(float));
    if (available<count) std::memset(static_cast<float*>(destination)+available*2,0,(count-available)*2*sizeof(float));
    // Advance through the short silent tail too, so the final visual fade can finish.
    submitted.store(first+count,std::memory_order_release);
}
std::uint64_t audibleFrame()
{
    const auto end=submitted.load(std::memory_order_acquire);
    return end>latency ? end-latency : 0;
}
}

bool music::load(const char* path)
{
    release();
    auto config=ma_decoder_config_init(ma_format_f32,2,rate);
    void* decoded=nullptr;
    if (ma_decode_file(path,&config,&length,&decoded)!=MA_SUCCESS) {
        std::fprintf(stderr,"ACT3: Could not decode soundtrack: %s\n",path); return false;
    }
    pcm=static_cast<float*>(decoded);
    auto playback=ma_device_config_init(ma_device_type_playback);
    playback.playback.format=ma_format_f32; playback.playback.channels=2;
    playback.sampleRate=rate; playback.periodSizeInMilliseconds=10; playback.periods=2;
    playback.dataCallback=output;
    if (ma_device_init(nullptr,&playback,&device)!=MA_SUCCESS) {
        std::fputs("ACT3: Could not initialize audio output\n",stderr); release(); return false;
    }
    initialized=true;
    latency=std::uint64_t(device.playback.internalPeriodSizeInFrames)*device.playback.internalPeriods*rate/
        std::max(1u,device.playback.internalSampleRate);
    std::fprintf(stderr,"ACT3: miniaudio, %u Hz stereo, soundtrack %.3f seconds\n",rate,double(length)/rate);
    return true;
}
bool music::start()
{
    if (playing) return true;
    if (!initialized) return false;
    if (ma_device_start(&device)!=MA_SUCCESS) {
        std::fputs("ACT3: Could not start soundtrack playback\n",stderr); return false;
    }
    playing=true; return true;
}
std::uint64_t music::milliseconds() { return audibleFrame()*1000/rate; }

const music::Features& music::features()
{
    const auto end=audibleFrame();
    if (!pcm || end<window || end<lastAnalysis+512) return analysis;
    const float dt=float(end-lastAnalysis)/rate;
    std::array<std::complex<float>,window> fft{};
    double energy=0; float peak=0;
    for (unsigned i=0;i<window;++i) {
        const auto frame=end-window+i;
        const float sample=frame<length ? (pcm[frame*2]+pcm[frame*2+1])*0.5f : 0;
        energy+=sample*sample; peak=std::max(peak,std::abs(sample));
        fft[i]=sample*(0.5f-0.5f*std::cos(6.283185307f*i/(window-1)));
    }
    for (unsigned i=1,j=0;i<window;++i) {
        unsigned bit=window>>1;
        for (;j&bit;bit>>=1) j^=bit;
        j^=bit; if (i<j) std::swap(fft[i],fft[j]);
    }
    for (unsigned size=2;size<=window;size<<=1) {
        const auto step=std::polar(1.f,-6.283185307f/size);
        for (unsigned begin=0;begin<window;begin+=size) {
            std::complex<float> phase{1,0};
            for (unsigned j=0;j<size/2;++j) {
                const auto a=fft[begin+j], b=fft[begin+j+size/2]*phase;
                fft[begin+j]=a+b; fft[begin+j+size/2]=a-b; phase*=step;
            }
        }
    }
    float flux=0,bass=0,mids=0,treble=0,orchestralFlux=0;
    std::array<float,window/2> magnitudes{};
    for (unsigned bin=1;bin<window/2;++bin) {
        const float value=std::abs(fft[bin])*4/window;
        magnitudes[bin]=value;
        const float hz=float(bin)*rate/window;
        const float attack=std::max(0.f,value-previous[bin]);
        flux+=attack;
        if (hz>=350 && hz<=6000) orchestralFlux+=attack;
        previous[bin]=value;
        if (hz<250) bass+=value*value;
        else if (hz<4000) mids+=value*value;
        else treble+=value*value;
    }
    for (unsigned band=0;band<analysis.spectrum.size();++band) {
        const unsigned lo=std::max(1u,unsigned(40*std::pow(400.f,float(band)/64)*window/rate));
        const unsigned hi=std::min(window/2-1,std::max(lo+1,unsigned(40*std::pow(400.f,float(band+1)/64)*window/rate)));
        float value=0; for (unsigned bin=lo;bin<hi;++bin) value=std::max(value,magnitudes[bin]);
        analysis.spectrum[band]=value;
    }
    analysis.rms=static_cast<float>(std::sqrt(energy/window)); analysis.peak=peak;
    analysis.bass=std::sqrt(bass); analysis.mids=std::sqrt(mids); analysis.treble=std::sqrt(treble);
    analysis.onset=std::max(0.f,(flux-averageFlux)/(averageFlux+0.015f));
    analysis.pulse*=std::exp(-dt*7);
    if (lastAnalysis && flux>averageFlux*1.65f+0.025f && end-lastPulse>rate/5) {
        analysis.pulse=1; lastPulse=end;
    }
    averageFlux+=(flux-averageFlux)*(1-std::exp(-dt*2));
    analysis.orchestralPulse*=std::exp(-dt*5.5f);
    if (lastAnalysis && analysis.rms>0.015f && orchestralFlux>averageOrchestralFlux*1.55f+0.012f &&
        end-lastOrchestralPulse>rate/6) {
        analysis.orchestralPulse=1; lastOrchestralPulse=end;
    }
    averageOrchestralFlux+=(orchestralFlux-averageOrchestralFlux)*(1-std::exp(-dt*2));
    lastAnalysis=end;
    return analysis;
}
void music::release()
{
    if (initialized) ma_device_uninit(&device);
    initialized=playing=false;
    if (pcm) ma_free(pcm,nullptr);
    pcm=nullptr; length=0; submitted.store(0); latency=lastAnalysis=lastPulse=0;
    analysis={}; previous={}; averageFlux=0;
    averageOrchestralFlux=0; lastOrchestralPulse=0;
}
