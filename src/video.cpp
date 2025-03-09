#include "../inc/video.h"

MyVideo::MyVideo(ResourceProxy& resrc):renderer(nullptr, rendererDeleter),
                                       displayWindow(nullptr, windowDeleter),
                                       texture(nullptr, textureDeleter),
                                       decodedFrame(nullptr, frameDeleter),
                                       displayFrame(nullptr, frameDeleter),
                                       lastFrame(nullptr, frameDeleter),
                                       swsCtx(nullptr, swsDeleter),
                                       proxy(resrc)
                                       {
    decodedFrame.reset(av_frame_alloc());
    displayFrame = std::shared_ptr<AVFrame>(av_frame_alloc(), frameDeleter);
}

void videoThFunc(MyVideo* video){
    while(!video->proxy.EXIT){
        auto pkg = video->proxy.getVPkg();
        //EXIT被置位才会返回空包
        if(!pkg) return;
        while(video->proxy.isVFrameQFull() && !video->proxy.EXIT) std::this_thread::yield();
        if(!video->proxy.EXIT && pkg){
            video->decodeFrame(move(pkg));
        }
    }
}

int MyVideo::run(){
    init();
    auto videoTh = std::thread(videoThFunc, this);
    while(!proxy.EXIT){
        while(!proxy.EXIT && SDL_PollEvent(&event)){
            processEvents();
        }
        // std::cout<<proxy.remainingTime<<std::endl;
        if(proxy.remainingTime>0) {
            std::this_thread::sleep_for(std::chrono::microseconds((int)(proxy.remainingTime*1e6)));
        }
        proxy.remainingTime = 0.01;
        videoRefresh();
    }
    videoTh.join();
    return 0;
}

int MyVideo::init(){
    auto ptr = SDL_CreateWindow("title", 
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                800,
                                600,
                                SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    displayWindow.reset(ptr);
    renderer.reset(SDL_CreateRenderer(displayWindow.get(), -1, 0));

    while(!proxy.videoCtxReady && !proxy.EXIT) std::this_thread::yield();
    if(proxy.EXIT) return 0;
    videoCodecCtxPtr = proxy.getVideoParCtx();
    videoCodecCtx = *videoCodecCtxPtr;
    texture.reset(SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_IYUV, 
                                    SDL_TEXTUREACCESS_STREAMING, 
                                    videoCodecCtx->width, 
                                    videoCodecCtx->height));
    setDisplayRect(0, 0, videoCodecCtx->width, videoCodecCtx->height);
    ratio = (double)videoCodecCtx->width/videoCodecCtx->height;
    auto swsCtxPtr = sws_getContext(videoCodecCtx->width, videoCodecCtx->height, videoCodecCtx->pix_fmt,
                                 videoCodecCtx->width, videoCodecCtx->height, AV_PIX_FMT_YUV420P,
                                 SWS_BILINEAR, nullptr, nullptr, nullptr);
    swsCtx.reset(swsCtxPtr);
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, videoCodecCtx->width, videoCodecCtx->height, 32);
    unsigned char* buffer = (unsigned char*)av_malloc(bufferSize*sizeof(unsigned char));
    av_image_fill_arrays(displayFrame->data, displayFrame->linesize, 
                         buffer, AV_PIX_FMT_YUV420P, 
                         videoCodecCtx->width, 
                         videoCodecCtx->height, 32);
    // av_free(buffer);
    return 0;
}

int MyVideo::videoRefresh(){
    while(!proxy.EXIT){
        nextFrame = proxy.peekVFrame();
        if(!nextFrame) {
            std::cout<<"lacking frame\n";
            return -1;
        }
        auto lastFrameDur = calculateDur(nextFrame);
        auto delay = calculateDelay(lastFrameDur);
        double curTime = av_gettime_relative()/1e6;
        if(!proxy.EXIT && videoCLK.systemTime+delay<curTime){
            videoCLK.updateCur(proxy.videoTimeBase * nextFrame->pts);
            lastFrame = nextFrame;
            proxy.popVFrameQ();
            videoCLK.systemTime += delay;
            if(videoCLK.systemTime < curTime-0.1){
                videoCLK.systemTime = curTime;
            }
            continue;
        }
        else if(!proxy.EXIT){
            videoCLK.updateCur(proxy.videoTimeBase * nextFrame->pts);
            lastFrame = nextFrame;
            logger.commit(LogStrategy::DEFAULT_FORMAT, "video", videoCLK);
            videoDisplay(nextFrame);
            proxy.remainingTime = std::min(proxy.remainingTime,  videoCLK.systemTime + delay - curTime);
            return 0;
        }
    }
    return 0;
}

void MyVideo::decodeFrame(std::unique_ptr<AVPacket, void(*)(AVPacket*)> pkg){
    if(!pkg || !videoCodecCtx) return;
    if((avcodec_send_packet(videoCodecCtx, pkg.get())==0) && !proxy.EXIT){
        while((avcodec_receive_frame(videoCodecCtx, decodedFrame.get())==0) && !proxy.EXIT){
            proxy.addVFrame(move(decodedFrame));
            decodedFrame.reset(av_frame_alloc());
        }
    }
    return;
}

void MyVideo::videoDisplay(std::shared_ptr<AVFrame>& frame){
    // std::cout<<proxy.videoTimeBase * frame->pts<<std::endl;
    sws_scale(swsCtx.get(), (const unsigned char *const*)frame->data,
                      frame->linesize, 0, videoCodecCtx->height, 
                      displayFrame->data, displayFrame->linesize);
    SDL_UpdateYUVTexture(texture.get(), nullptr, 
                         displayFrame->data[0], displayFrame->linesize[0],
                         displayFrame->data[1], displayFrame->linesize[1],
                         displayFrame->data[2], displayFrame->linesize[2]);
    SDL_RenderClear(renderer.get());
    SDL_RenderCopy(renderer.get(), texture.get(), nullptr, &displayRect);
    SDL_RenderPresent(renderer.get());
}

double MyVideo::calculateDur(std::shared_ptr<AVFrame>& frame){
    if(!lastFrame){
        if(frame->pts==NAN) return 0;
        else return proxy.videoTimeBase * frame->pts;
    }
    else if(lastFrame->pts==NAN) return proxy.videoTimeBase * frame->pts;
    else if(lastFrame->pts>frame->pts) return 0;
    else return proxy.videoTimeBase*(frame->pts - lastFrame->pts);
}

double MyVideo::calculateDelay(double lastDur){
    auto diff = getCurPts(videoCLK) - getCurPts(proxy.systemCLK);
    //视频时钟落后主时钟lastDur以上，说明上一帧显示时间过头了
    if(diff<-proxy.syncThreshHold){
        diff = 0;
    }
    //视频时钟落后，但上一帧显示时间没过
    else if(diff<0){
        diff = std::max(0.0, diff+lastDur);
    }
    //视频超前，延迟时间需要拉长
    else{
        diff += lastDur;
    }
    return diff;
}

void MyVideo::processEvents(){
    while(SDL_PollEvent(&event) || pauseFlag && (!quitFlag)){
        switch(event.type){
            case SDL_KEYDOWN:{
                auto* keyEvent = reinterpret_cast<SDL_KeyboardEvent*>(&event);
                if(keyEvent->keysym.sym==SDLK_SPACE){
                    if(pauseFlag){
                        pauseFlag = 0;
                    }
                    else{
                        pauseFlag = 1;
                    }
                }
                else if(keyEvent->keysym.sym==SDLK_LEFT){
                    proxy.commitTask(demuxerTaskType::TYPE_SEEK, seekTaskST{5, demuxerTaskSeek::SEEK_BACKWARD_DURATION});
                }
                else if(keyEvent->keysym.sym==SDLK_RIGHT){
                    proxy.commitTask(demuxerTaskType::TYPE_SEEK, seekTaskST{5, demuxerTaskSeek::SEEK_FORWARD_DURATION});
                }
                break;
            }
            case SDL_QUIT:{
                proxy.EXIT = 1;
                proxy.demuxCv.notify_all();
                break;
            }
            case SDL_WINDOWEVENT:{
                if(event.window.event==SDL_WINDOWEVENT_SIZE_CHANGED){
                    // printf("window size is changed\n");
                    processWindowChangeEvent();
                }
            }
            default:break;
        }
    }
}

void MyVideo::processWindowChangeEvent(){
    int newWidth = event.window.data1;
    int newHeight = event.window.data2;
    double newRatio = (double)newWidth/newHeight;
    if(newRatio>ratio){
        setDisplayRect(newWidth/2-displayRect.w/2, 0, (double)newHeight*ratio, newHeight);
    }
    else{
        setDisplayRect(0, newHeight/2-displayRect.h/2, newWidth, newWidth/ratio);
    }
    // 需要向video线程提交刷新任务
    // SDL_RenderClear(renderer.get());
    // SDL_RenderCopy(renderer.get(), texture.get(), nullptr, &displayRect);
    // SDL_RenderPresent(renderer.get());
}

void MyVideo::setDisplayRect(int x, int y, int w, int h){
    displayRect.w = w;
    displayRect.h = h;
    displayRect.x = x;
    displayRect.y = y;
}

