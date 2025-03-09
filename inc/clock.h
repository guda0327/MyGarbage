#pragma once
#include <fstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <tuple>
#include "./logger.h"
extern "C"{
    #include <libavcodec/packet.h>
    #include <libavutil/avutil.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/log.h>
    #include <libavutil/time.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libavutil/opt.h>
    #include <SDL2/SDL.h>
}

class SyncClock{
    public:
    SyncClock();
    double baseTime;
    double systemTime;
    //当前帧的pts
    double curPts;
    //diff是curPts被更新的时候的系统时间与curPts的差值
    //用于获取当前播放时间点
    double diff;
    //播放完当前帧之后，流的pts，这项参数针对的是解码pts，而不是播放pts（我猜的）
    //更正：不是解码pts，是取出一帧新帧时的时间点，与解码时间点有区别
    double tailPts;

    void updateCur(double curPts);

    //retVal:av_gettime_relative()/1e6+diff
    //当前系统时间-上次更新curPts时候的系统时间+curPts
    double getCurTimePoint();

    void setTailPts(double time);
};