#include "../inc/proxy.h"

int ResourceProxy::addVPkg(std::unique_ptr<AVPacket, void(*)(AVPacket*)>&& vPkg){
    videoPkgQMtx.lock();
    videoPkgQ.emplace(std::move(vPkg));
    videoPkgQMtx.unlock();
    return 0;
}

int ResourceProxy::addAPkg(std::unique_ptr<AVPacket, void(*)(AVPacket*)>&& aPkg){
    audioPkgQMtx.lock();
    audioPkgQ.emplace(move(aPkg));
    audioPkgQMtx.unlock();
    return 0;
}

void ResourceProxy::popVFrameQ(){
    videoFrameQMtx.lock();
    if(!videoFrameQ.empty()){
        videoFrameQ.pop();
    }
    videoFrameQMtx.unlock();
}
std::shared_ptr<AVFrame> ResourceProxy::peekVFrame(){
    videoFrameQMtx.lock();
    if(!videoFrameQ.empty()){
        auto ret = videoFrameQ.front();
        videoFrameQMtx.unlock();
        return ret;
    }
    else{
        videoFrameQMtx.unlock();
        return std::unique_ptr<AVFrame, void(*)(AVFrame*)>(nullptr, frameDeleter);
    }
}

int ResourceProxy::addVFrame(std::shared_ptr<AVFrame>&& frame){
    videoFrameQMtx.lock();
    videoFrameQ.emplace(frame);
    videoFrameQMtx.unlock();
    return 0;
}

bool ResourceProxy::isVFrameQFull(){
    videoFrameQMtx.lock();
    if(videoFrameQ.size()>vFrameQLimit){
        videoFrameQMtx.unlock();
        return true;
    }
    videoFrameQMtx.unlock();
    return false;
}

int ResourceProxy::addAFrame(std::shared_ptr<AVFrame>&& frame){
    audioFrameQMtx.lock();
    audioFrameQ.emplace(move(frame));
    audioFrameQMtx.unlock();
    return 0;
}

std::unique_ptr<AVPacket, void(*)(AVPacket*)> ResourceProxy::getVPkg(){
    videoPkgQMtx.lock();
    if(videoPkgQ.size()==0){
        videoPkgQMtx.unlock();
        while(videoPkgQ.size()==0){
            printf("video is asking for demux\n\n");
            commitTask(demuxerTaskType::TYPE_DEMUX, demuxTaskST{});
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::this_thread::yield();
            if(EXIT || STOP){
                return std::unique_ptr<AVPacket, void(*)(AVPacket*)>(nullptr, avPacketDeleter);
            }
        }
        videoPkgQMtx.lock();
        //与else的逻辑一样，区别在于这里要加一次锁
        auto ret = move(videoPkgQ.front());
        videoPkgQ.pop();
        if(videoPkgQ.size()<20 && !PROXY_EOF){
            printf("video is asking for demux\n\n");
            commitTask(demuxerTaskType::TYPE_DEMUX, demuxTaskST{});
        }
        videoPkgQMtx.unlock();
        return ret;
    }
    else{
        auto ret = move(videoPkgQ.front());
        videoPkgQ.pop();
        if(videoPkgQ.size()<20 && !PROXY_EOF){
            printf("video is asking for demux\n\n");
            commitTask(demuxerTaskType::TYPE_DEMUX, demuxTaskST{});
        }
        videoPkgQMtx.unlock();
        return ret;
    }
}

std::unique_ptr<AVPacket, void(*)(AVPacket*)> ResourceProxy::getAPkg(){
    audioPkgQMtx.lock();
    // printf("there is %d pkgs in audioPkgQ\n\n", audioPkgQ.size());
    if(audioPkgQ.size()==0){
        audioPkgQMtx.unlock();
        while(audioPkgQ.size()==0){
            printf("audio is asking for demux\n\n");
            commitTask(demuxerTaskType::TYPE_DEMUX, demuxTaskST{});
            std::this_thread::yield();
            if(EXIT || STOP){
                return std::unique_ptr<AVPacket, void(*)(AVPacket*)>(nullptr, avPacketDeleter);
            }
        }
        audioPkgQMtx.lock();
        //与else的逻辑一样，区别在于这里要加一次锁
        auto ret = move(audioPkgQ.front());
        audioPkgQ.pop();
        if(audioPkgQ.size()<20 && !PROXY_EOF){
            printf("audio is asking for demux\n\n");
            commitTask(demuxerTaskType::TYPE_DEMUX, demuxTaskST{});
        }
        audioPkgQMtx.unlock();
        return ret;
    }
    else{
        auto ret = move(audioPkgQ.front());
        audioPkgQ.pop();
        if(audioPkgQ.size()<20 && !PROXY_EOF){
            printf("audio is asking for demux\n\n");
            commitTask(demuxerTaskType::TYPE_DEMUX, demuxTaskST{});
        }
        audioPkgQMtx.unlock();
        return ret;
    }
}

void ResourceProxy::popAFrameQ(){
    audioFrameQMtx.lock();
    if(!audioFrameQ.empty()){
        audioFrameQ.pop();
    }
    audioFrameQMtx.unlock();
}

std::shared_ptr<AVFrame> ResourceProxy::peekAFrame(){
    audioFrameQMtx.lock();
    if(!audioFrameQ.empty()){
        auto ret = audioFrameQ.front();
        audioFrameQMtx.unlock();
        return ret;
    }
    else{
        audioFrameQMtx.unlock();
        return std::unique_ptr<AVFrame, void(*)(AVFrame*)>(nullptr, frameDeleter);
    }
}

bool ResourceProxy::isAFrameQFull(){
    audioFrameQMtx.lock();
    if(audioFrameQ.size()>=aFrameQLimit){
        audioFrameQMtx.unlock();
        return true;
    }
    audioFrameQMtx.unlock();
    return false;
}

void ResourceProxy::syncSystenCLK(SyncClock& clock){
    auto masterClock = getClock(clock);
    systemCLKMtx.lock();
    systemCLK.tailPts = clock.tailPts;
    systemCLK.updateCur(masterClock);
    logger.commit(LogStrategy::DEFAULT_FORMAT, "proxy", systemCLK);
    systemCLKMtx.unlock();
}

double ResourceProxy::getClock(SyncClock& clock){
    double cur = (double)av_gettime_relative()/1e6;
    return cur+clock.diff;
}

void ResourceProxy::commitTask(demuxerTaskType ty, demuxerTaskArgs args){
    taskQMtx.lock();
    demuxerTasks.emplace(taskST{ty, args});
    taskQMtx.unlock();
    demuxCv.notify_one();
}

bool ResourceProxy::isTasksEmpty(){
    return demuxerTasks.empty();
}

taskST ResourceProxy::getTask(){
    if(demuxerTasks.empty()){
        //这里有线程同步问题，有待解决
        std::runtime_error("task Q is empty");
    }
    taskQMtx.lock();
    auto ret = demuxerTasks.front();
    demuxerTasks.pop();
    taskQMtx.unlock();
    return std::move(ret);
}

void ResourceProxy::waitForDemux(){
    std::unique_lock<std::mutex> uMtx(demuxMtx);
    demuxCv.wait(uMtx);
}

void ResourceProxy::synchronizeClock(SyncClock& clk){
    systemCLKMtx.lock();
    memcpy(&systemCLK, &clk, sizeof(SyncClock));
    systemCLKMtx.unlock();
}

std::shared_ptr<AVCodecContext*> ResourceProxy::getVideoParCtx(){
    return videoCodecCtx;
}

std::shared_ptr<AVCodecContext*> ResourceProxy::getAudioParCtx(){
    return audioCodecCtx;
}




