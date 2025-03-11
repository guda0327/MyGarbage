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
    int calculateSeekTS(int sec);
    std::atomic<bool> goOn;
    private:
    double videoDuration;
    double audioDuration;
    bool STOP;
    int seekMethod = 0;
    int videoStreamIdx = 0;
    int audioStreamIdx = 1;
    int demux(int sec);
    double p2d(AVPacket* pkg);
    void flushCodecCtxs();
    std::unique_ptr<AVFormatContext, void(*)(AVFormatContext*)> fileCtx;
    std::unique_ptr<PacketST, void(*)(PacketST*)> packet;
    ResourceProxy &proxy;
};


