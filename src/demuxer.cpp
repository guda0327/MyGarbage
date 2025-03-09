#include "../inc/demuxer.h"
#include <iostream>

MyDemuxer::MyDemuxer(ResourceProxy &resrc):
                     proxy(resrc),
                     pkg(nullptr, avPacketDeleter),
                     fileCtx(nullptr, avFormatCtxDeleter){
    pkg.reset(av_packet_alloc());
    fileCtx.reset(avformat_alloc_context());
    int videoStreamIdx = 0;
    int audioStreamIdx = 1;
}

int MyDemuxer::run(){
    openInput("./videos/catastrophic_fabrications.mp4");
    while(!proxy.EXIT){
        proxy.waitForDemux();
        if(proxy.EXIT) break;
        while(!proxy.isTasksEmpty()){
            auto task = proxy.getTask();
            switch(task.ty){
                case demuxerTaskType::TYPE_DEMUX:{
                    auto ret = demux(3);
                    if(ret==-1) break;
                    break;
                }
                case demuxerTaskType::TYPE_SEEK:{

                    break;
                }
                default:break;
            }
        }
        
    }
    return 0;
}

void MyDemuxer::openInput(const std::string& fileName){
    auto fileCtxPtr = fileCtx.get();
    avformat_open_input(&fileCtxPtr, "./videos/summer_rendering.mp4", nullptr, nullptr);
    avformat_find_stream_info(fileCtxPtr, nullptr);

    videoStreamIdx = av_find_best_stream(fileCtxPtr, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    audioStreamIdx = av_find_best_stream(fileCtxPtr, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    auto videoStreamPtr = fileCtx->streams[videoStreamIdx];
    auto audioStreamPtr = fileCtx->streams[audioStreamIdx];

    videoDuration = (double)videoStreamPtr->duration*
                    videoStreamPtr->time_base.num/videoStreamPtr->time_base.den;
    audioDuration = (double)audioStreamPtr->duration*
                    audioStreamPtr->time_base.num/audioStreamPtr->time_base.den;

    auto videoCodecPar = videoStreamPtr->codecpar;
    auto videoCodec = avcodec_find_decoder(videoCodecPar->codec_id);
    auto audioCodecPar = audioStreamPtr->codecpar;
    auto audioCodec = avcodec_find_decoder(audioCodecPar->codec_id);
    
    {
        proxy.vCodecMtx.lock();
        auto videoCodecCtxPtr = proxy.getVideoParCtx();
        auto tmpPtr = videoCodecCtxPtr.get();
        avcodec_free_context(tmpPtr);
        
        *videoCodecCtxPtr = avcodec_alloc_context3(videoCodec);
        auto videoCodecCtx = *videoCodecCtxPtr;
        avcodec_parameters_to_context(videoCodecCtx, videoCodecPar);
        avcodec_open2(videoCodecCtx, videoCodec, nullptr);
        proxy.videoCtxReady = 1;
        proxy.vCodecMtx.unlock();
    }

    {
        proxy.aCodecMtx.lock();
        auto audioCodecCtxPtr = proxy.getAudioParCtx();
        auto tmpPtr = audioCodecCtxPtr.get();
        avcodec_free_context(tmpPtr);
        
        *audioCodecCtxPtr = avcodec_alloc_context3(audioCodec);
        auto audioCodecCtx = *audioCodecCtxPtr;
        avcodec_parameters_to_context(audioCodecCtx, audioCodecPar);
        avcodec_open2(audioCodecCtx, audioCodec, nullptr);
        proxy.audioCtxReady = 1;
        proxy.aCodecMtx.unlock();
    }
    proxy.videoTimeBase = av_q2d(fileCtx->streams[videoStreamIdx]->time_base);
    proxy.audioTimeBase = av_q2d(fileCtx->streams[audioStreamIdx]->time_base);
    proxy.maxDuration = std::max(videoStreamPtr->duration * av_q2d(videoStreamPtr->time_base), audioStreamPtr->duration * av_q2d(audioStreamPtr->time_base));
}

/*
 *返回值：-1：文件读取完了，av_read_frame结束，调用者应该有所处理 
 * 
 * 
 * 
 * 
 */
int MyDemuxer::demux(int sec){
    auto curPts = proxy.systemCLK.curPts;
    auto tarPts = curPts + sec;
    tarPts = std::min(tarPts, proxy.maxDuration);
    printf("the system clock is %f \n\n", tarPts);
    printf("the target pts is %f \n\n", tarPts);
    int accomplished = 0;
    //accomplished低两位代表音，视频是否读取到了tarpts
    while(!proxy.EXIT && accomplished!=3){
        auto ret = av_read_frame(fileCtx.get(), pkg.get());
        auto pkgPts = p2d(pkg.get());
        if(ret<0) return -1;
        if(pkgPts>=tarPts){
            if(pkg->stream_index==videoStreamIdx) accomplished|=(1<<0);
            else if(pkg->stream_index==audioStreamIdx) accomplished|=(1<<1);
        }
        if(pkg->stream_index==videoStreamIdx){
            proxy.addVPkg(std::move(pkg));
            pkg.reset(av_packet_alloc());
        }
        if(pkg->stream_index==audioStreamIdx){
            proxy.addAPkg(std::move(pkg));
            pkg.reset(av_packet_alloc());
        }
        else{
            pkg.reset(av_packet_alloc());
        }
    }
    printf("end this time demux\n\n");
    return 0;
}

double MyDemuxer::p2d(AVPacket* pkg){
    if(pkg->pts<0) return 0.0;
    if(pkg->stream_index == videoStreamIdx){
        return av_q2d(fileCtx->streams[videoStreamIdx]->time_base)*pkg->pts;
    }
    else{
        return av_q2d(fileCtx->streams[audioStreamIdx]->time_base)*pkg->pts;
    }
}

void func(){
    
}