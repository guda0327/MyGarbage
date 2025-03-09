#pragma once
#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <memory>
#include <atomic>
#include <algorithm>
#include "./proxy.h"
#include "tools.h"
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
class MyDemuxer{
    public:
    MyDemuxer(ResourceProxy &resrc);
    void openInput(const std::string& fileName);
    int run();
    std::atomic<bool> goOn;
    private:
    double videoDuration;
    double audioDuration;
    bool STOP;
    int videoStreamIdx;
    int audioStreamIdx;
    int demux(int sec);
    double p2d(AVPacket* pkg);
    std::unique_ptr<AVFormatContext, void(*)(AVFormatContext*)> fileCtx;
    std::unique_ptr<AVPacket, void(*)(AVPacket*)> pkg;
    ResourceProxy &proxy;
};


