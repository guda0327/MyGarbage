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

class FrameST{
    public:
    FrameST():frame(nullptr), serial(-1){}
    FrameST(int s):frame(nullptr), serial(s){}
    FrameST(AVFrame* ptr, int s):frame(ptr), serial(s){}
    AVFrame* frame;
    int serial;
};

class PacketST{
    public:
    PacketST():pkg(nullptr), serial(-1){}
    PacketST(int s):pkg(nullptr), serial(s){}
    PacketST(AVPacket* ptr, int s):pkg(ptr), serial(s){}
    AVPacket* pkg;
    int serial;
};

void swrCtxDeleter(SwrContext* ptr);
void audioDeviceDeleter(SDL_AudioDeviceID* id);
void channelLayoutDeleter(AVChannelLayout* ptr);
void frameBufferDeleter(uint8_t** ptr);
void windowDeleter(SDL_Window* windowPtr);
void rendererDeleter(SDL_Renderer* rendererPtr);
void textureDeleter(SDL_Texture* texturePtr);

void frameDeleter(AVFrame* framePtr);
void frameSTDeleter(FrameST* frameSTPtr);

void swsDeleter(SwsContext* swsPtr);
void avFormatCtxDeleter(AVFormatContext* ctx);
void avCodecCtxDeleter(AVCodecContext* ctx);

void avPacketDeleter(AVPacket* pkg);
void packetSTDeleter(PacketST* packetSTPtr);

double getCurPts(SyncClock& clock);