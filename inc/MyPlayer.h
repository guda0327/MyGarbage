#pragma once
#include "./demuxer.h"
#include "./audio.h"
#include "./video.h"
#include "./proxy.h"

class MyPlayer{
    public:
    MyPlayer();
    ~MyPlayer(){};
    void run();
    private:
    ResourceProxy proxy;
    MyVideo video;
    MyAudio audio;
    MyDemuxer demuxer;
};



