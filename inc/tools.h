#pragma once
#include "clock.h"
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


void swrCtxDeleter(SwrContext* ptr);
void audioDeviceDeleter(SDL_AudioDeviceID* id);
void channelLayoutDeleter(AVChannelLayout* ptr);
void frameBufferDeleter(uint8_t** ptr);
void windowDeleter(SDL_Window* windowPtr);
void rendererDeleter(SDL_Renderer* rendererPtr);
void textureDeleter(SDL_Texture* texturePtr);
void frameDeleter(AVFrame* framePtr);
void swsDeleter(SwsContext* swsPtr);
void avFormatCtxDeleter(AVFormatContext* ctx);
void avCodecCtxDeleter(AVCodecContext* ctx);
void avPacketDeleter(AVPacket* pkg);
double getCurPts(SyncClock& clock);