#pragma once
#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include <variant>
#include "clock.h"
#include "tools.h"
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
class ResourceProxy;
extern AsyncLogger logger;

enum class demuxerTaskSeek{
    SEEK_FORWARD_DURATION,
    SEEK_BACKWARD_DURATION,
    SEEK_FORWARD_TIME_POINT,
    SEEK_BACKWARD_TIME_POINT,
};

enum class demuxerTaskType{
    TYPE_DEMUX,
    TYPE_SEEK,
};

struct demuxTaskST{};
struct seekTaskST{
    int time;
    demuxerTaskSeek type;
};
using demuxerTaskArgs = std::variant<
    demuxTaskST,
    seekTaskST
    >;
struct taskST{
    demuxerTaskType ty;
    demuxerTaskArgs args;
};

class ResourceProxy{
    public:
    ResourceProxy(){
        videoCodecCtx = std::move(std::shared_ptr<AVCodecContext*>(&videoCodecCtx_offset,[](AVCodecContext** ptr){
                    if(ptr){
                        avcodec_free_context(ptr);
                    }
                }
            )
        );
        audioCodecCtx = std::move(std::shared_ptr<AVCodecContext*>(&audioCodecCtx_offset,[](AVCodecContext** ptr){
                    if(ptr){
                        avcodec_free_context(ptr);
                    }
                }
            )
        );
    }
    //最长流的时长
    double maxDuration;
    //音、视频流的time_base(double 格式)
    double audioTimeBase;
    double videoTimeBase;

    int addVPkg(std::unique_ptr<PacketST, void(*)(PacketST*)>&& vPkg);
    int addAPkg(std::unique_ptr<PacketST, void(*)(PacketST*)>&& aPkg);

    void popVFrameQ();
    std::shared_ptr<FrameST> peekVFrame();
    int addVFrame(std::shared_ptr<FrameST>&& frame);
    bool isVFrameQFull();
    

    
    //两个get函数都会对对应的queue加锁后以右值形式返回一个pkg指针
    //queue里边没东西会向demuxer提交demux任务
    //queue里边东西太少也会提交demux任务
    std::unique_ptr<PacketST, void(*)(PacketST*)> getVPkg();
    std::unique_ptr<PacketST, void(*)(PacketST*)> getAPkg();

    void popAFrameQ();
    std::shared_ptr<FrameST> peekAFrame();
    int addAFrame(std::shared_ptr<FrameST>&& frame);
    bool isAFrameQFull();
    // AVPacket* getAPkg();
    SyncClock systemCLK;
    void syncSystenCLK(SyncClock& clock);
    double getClock(SyncClock& clock);
    double syncThreshHold = 0.1;

    void commitTask(demuxerTaskType ty, demuxerTaskArgs args);
    bool isTasksEmpty();
    taskST getTask();
    
    void waitForDemux();

    //时钟同步
    void synchronizeClock(SyncClock& clk);

    std::shared_ptr<AVCodecContext*> getVideoParCtx();
    std::shared_ptr<AVCodecContext*> getAudioParCtx();
    //暂停
    std::atomic<int> STOP = 0;
    //退出
    std::atomic<int> EXIT = 0;
    //av_read_frame读完了
    std::atomic<int> PROXY_EOF = 0;
    //全局serial
    std::atomic<int> seekSerial = 0;
    
    std::mutex taskQMtx;
    std::mutex vCodecMtx;
    std::mutex aCodecMtx;
    std::mutex demuxMtx;
    std::mutex videoPkgQMtx;
    std::mutex audioPkgQMtx;
    std::mutex videoFrameQMtx;
    std::mutex audioFrameQMtx;
    std::mutex systemCLKMtx;
    std::condition_variable demuxCv;
    std::condition_variable videoPkgQCv;
    std::condition_variable aDecoderCV;
    static constexpr int vFrameQLimit = 8;
    static constexpr int aFrameQLimit = 32;
    std::atomic<int> videoCtxReady = 0;
    std::atomic<int> audioCtxReady = 0;
    double remainingTime = 0.0;

    private:

    std::atomic<int> vFrameQOverflow = 0;
    std::atomic<int> aFrameQOverflow = 0;
    //此指针被demuxer和video（audio）线程共享
    //内容应该由demuxer初始化，析构函数由代理对象设置
    //每次换文件或者换解码器，要保证顺序
    AVCodecContext* videoCodecCtx_offset = nullptr;
    AVCodecContext* audioCodecCtx_offset = nullptr;
    std::shared_ptr<AVCodecContext*> videoCodecCtx;
    std::shared_ptr<AVCodecContext*> audioCodecCtx;

    std::queue<taskST> demuxerTasks;
    std::queue<std::unique_ptr<PacketST, void(*)(PacketST*)>> videoPkgQ;
    std::queue<std::unique_ptr<PacketST, void(*)(PacketST*)>> audioPkgQ;

    std::queue<std::shared_ptr<FrameST>> videoFrameQ;
    std::queue<std::shared_ptr<FrameST>> audioFrameQ;
    // std::queue<AVPacket*> audioPkgQ;
};