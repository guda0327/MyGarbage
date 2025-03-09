#include "../inc/clock.h"
extern AsyncLogger logger;


SyncClock::SyncClock(){
    baseTime = av_gettime_relative()/1e6;
    systemTime = baseTime;
    curPts = 0;
    diff = curPts - baseTime;
}

void SyncClock::updateCur(double curPts){
    double curTime = av_gettime_relative()/1e6;
    this->curPts = curPts;
    this->diff = curPts - curTime;
    // this->systemTime = curTime;
    // printf("%f\n", tailPts);
}

double SyncClock::getCurTimePoint(){
    double curTime = (double)av_gettime_relative()/1e6;
    return curTime+diff;
}