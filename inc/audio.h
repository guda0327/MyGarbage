#pragma once
#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <memory>
#include "./proxy.h"
#include "./clock.h"
#include "./logger.h"
#include "./tools.h"
extern "C"{
    #include <libavcodec/packet.h>
    #include <libavutil/avutil.h>
    #include <libavutil/log.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libavutil/opt.h>
    #include <SDL2/SDL.h>
}


struct FrameBuffer{
    FrameBuffer();
    ~FrameBuffer(){
        av_free(buffer);
    }
    uint8_t* buffer;
    int size;
    int idx;
};

class MyAudio{
    public:
    MyAudio(ResourceProxy& resrc);
    int run();
    private:
    void setSwrCtx(AVCodecContext* ptr);
    //这个函数逻辑暂时是写死了，有需要可以加参数改
    void setSpec(SDL_AudioSpec& spec);
    //解码，并写入缓存
    void decodeFrame(std::unique_ptr<PacketST, void(*)(PacketST*)>&& packet);
    //对frame做处理，主要是重采样
    int editFrame();

    SyncClock audioCLK;
    SDL_AudioSpec desiredSpec;
    SDL_AudioSpec obtainedSpec;
    //SDL内部音频数据低于这个数会调用回调函数
    //由SDL_OpenAudioDevice函数设定，从obtainedSpec中获得
    int callBackRequiredSize;
    double bytesPerSec;
    FrameBuffer resampledBuffer;
    AVCodecContext* audioCodecCtx;
    std::shared_ptr<FrameST> decodedFrameST;
    std::unique_ptr<SwrContext, void(*)(SwrContext*)> swrCtx;
    std::unique_ptr<SDL_AudioDeviceID, void(*)(SDL_AudioDeviceID*)> audioDevice;
    std::unique_ptr<AVChannelLayout, void(*)(AVChannelLayout* ptr)> channelLayout;
    ResourceProxy& proxy;

    friend void SDLAudioCallback(void *userdata, Uint8 * stream, int len);
};

