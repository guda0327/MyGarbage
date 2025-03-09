#pragma once
#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <algorithm>
#include "./proxy.h"
#include "./clock.h"
#include "tools.h"
#include "./logger.h"
extern "C"{
    #include <libavcodec/packet.h>
    #include <libavutil/avutil.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/log.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libavutil/opt.h>
    #include <SDL2/SDL.h>
}

extern AsyncLogger logger;

class MyVideo{
    public:
    MyVideo(ResourceProxy& resrc);
    int run();
    int init();
    int videoRefresh();
    private:
    void videoDisplay(std::shared_ptr<AVFrame>& frame);
    void decodeFrame(std::unique_ptr<AVPacket, void(*)(AVPacket*)> pkg);
    void processEvents();
    void processWindowChangeEvent();
    void setDisplayRect(int x, int y, int w, int h);

    double calculateDur(std::shared_ptr<AVFrame>& frame);
    double calculateDelay(double lastDur);
    double syncThreshHold = 0.1;
    SyncClock videoCLK;

    SDL_Event event;
    SDL_Rect displayRect;
    std::atomic<int> quitFlag = 0;
    std::atomic<int> pauseFlag = 0;
    double ratio = 1;
    ResourceProxy& proxy;
    AVCodecContext* videoCodecCtx;
    std::shared_ptr<AVCodecContext*> videoCodecCtxPtr;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> texture;
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> displayWindow;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> renderer;
    std::unique_ptr<AVFrame, void(*)(AVFrame*)> decodedFrame;
    std::shared_ptr<AVFrame> displayFrame;
    std::shared_ptr<AVFrame> lastFrame;
    std::shared_ptr<AVFrame> nextFrame;
    // std::unique_ptr<AVFrame, void(*)(AVFrame*)> displayFrame;
    std::unique_ptr<SwsContext, void(*)(SwsContext*)> swsCtx;

    friend void videoThFunc(MyVideo* video);
};

