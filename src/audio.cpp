#include "../inc/audio.h"
#include <iostream>

MyAudio::MyAudio(ResourceProxy& resrc):swrCtx(nullptr, swrCtxDeleter),
                                       audioDevice(nullptr, audioDeviceDeleter),
                                       decodedFrameST(nullptr, frameSTDeleter),
                                       channelLayout(nullptr, channelLayoutDeleter),
                                       proxy(resrc){
    setSpec(desiredSpec);
    swrCtx.reset(swr_alloc());
    decodedFrameST.reset(new FrameST(av_frame_alloc(), static_cast<int>(proxy.seekSerial)));
}

int MyAudio::run(){
    //拿音频编码上下文
    while(!proxy.EXIT && !proxy.audioCtxReady) std::this_thread::yield();
    if(proxy.EXIT) return 0;
    auto audioCodecCtxPtr = proxy.getAudioParCtx();
    audioCodecCtx = *audioCodecCtxPtr;
    //设置期望参数
    setSpec(desiredSpec);
    //必须先开设备，后设置重采样相关的参数
    auto tmpID = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, &obtainedSpec, 0);
    bytesPerSec = av_samples_get_buffer_size(nullptr, 
                                             obtainedSpec.channels, 
                                             obtainedSpec.freq, 
                                             AV_SAMPLE_FMT_S16,
                                             1);
    //默认输出通道数为2
    channelLayout.reset(new AVChannelLayout);
    av_channel_layout_default(channelLayout.get(), obtainedSpec.channels);
    //分配重采样器
    setSwrCtx(audioCodecCtx);
    callBackRequiredSize = obtainedSpec.size;
    audioDevice.reset(new SDL_AudioDeviceID(tmpID));
    SDL_PauseAudioDevice(*audioDevice, 0);
    while(!proxy.EXIT){
        auto pkg = proxy.getAPkg();
        if(!pkg) continue;
        while(proxy.isAFrameQFull() && !proxy.EXIT){
            // std::cout<<"audio frame q is full\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::this_thread::yield();
        }
        if(!proxy.EXIT && pkg){
            decodeFrame(move(pkg));
        }
    }
    return 0;
}

void MyAudio::setSwrCtx(AVCodecContext* ptr){
    auto swrPtr = swrCtx.get();
    auto ret = swr_alloc_set_opts2(&swrPtr, 
                        channelLayout.get(),
                        AV_SAMPLE_FMT_S16,
                        obtainedSpec.freq,
                        &ptr->ch_layout,
                        ptr->sample_fmt,
                        ptr->sample_rate,
                        0,
                        nullptr);
    if(!ptr || swr_init(swrCtx.get())<0){
        std::runtime_error("alloc swrCtx error");
    }
}



FrameBuffer::FrameBuffer(){
    buffer = nullptr;
    size = 0;
    idx = 0;
}

void MyAudio::decodeFrame(std::unique_ptr<PacketST, void(*)(PacketST*)>&& packet){
    if(!packet || !audioCodecCtx) return;
    if(!packet->pkg || packet->serial!=proxy.seekSerial) return;
    if((avcodec_send_packet(audioCodecCtx, packet->pkg)==0) && !proxy.EXIT){
        while((avcodec_receive_frame(audioCodecCtx, decodedFrameST->frame)==0) && !proxy.EXIT){
            proxy.addAFrame(move(decodedFrameST));
            decodedFrameST.reset(new FrameST(av_frame_alloc(), static_cast<int>(proxy.seekSerial)));
        }
    }
    return;
}

int MyAudio::editFrame(){
    auto frameST = proxy.peekAFrame();
    if(!frameST) return -1;
    auto frame = frameST->frame;
    if(!frame) return -1;
    int dataSize = av_samples_get_buffer_size(nullptr, 
                                              frame->ch_layout.nb_channels, 
                                              frame->nb_samples, 
                                              static_cast<AVSampleFormat>(frame->format), 1);
    auto outputSamples = swr_get_out_samples(swrCtx.get(), frame->nb_samples);
    auto requiredSize = outputSamples*obtainedSpec.channels*2;
    if(requiredSize>resampledBuffer.size){
        if(resampledBuffer.buffer) av_free(resampledBuffer.buffer);
        resampledBuffer.buffer = nullptr;
        av_samples_alloc(&resampledBuffer.buffer, nullptr,
                         obtainedSpec.channels, outputSamples,
                         AV_SAMPLE_FMT_S16, 0);
    }
    resampledBuffer.size = requiredSize;
    resampledBuffer.idx = 0;
    auto realSample = swr_convert(swrCtx.get(), &resampledBuffer.buffer, outputSamples, 
                                    (const uint8_t**)frame->data, 
                                    frame->nb_samples);
    if(realSample>0){
        dataSize = realSample*2*sizeof(int16_t);
        resampledBuffer.size = dataSize;
    }
    if(frame->pts>=0){
        audioCLK.tailPts = av_q2d(audioCodecCtx->time_base)*frame->pts + 
                                 (double)realSample/obtainedSpec.freq;
    }
    proxy.popAFrameQ();
    return dataSize;
}

void SDLAudioCallback(void *userdata, Uint8 * stream, int len){
    MyAudio* audioST = (MyAudio*)userdata;
    while(len>0){
        auto& idx = audioST->resampledBuffer.idx;
        auto& size = audioST->resampledBuffer.size;
        if(idx>=size){
            //假如resampleBuffer的数据不够就读取frame,这个函数内会修改idx和size大小
            //这里的edit固定会进行重采样，可以后续修改逻辑
            auto ret = audioST->editFrame();
            if(ret==-1){
                std::cout<<"audio callback lacks frame\n";
                memset(stream, 0, len);
                return;
            }
        }
        int len1 = (size - idx)> len ? len : (size - idx);
        memcpy(stream, audioST->resampledBuffer.buffer + idx, len1);
        len-=len1;
        stream+=len1;
        idx+=len1;
    }
    if(audioST->audioCLK.tailPts){
        audioST->audioCLK.updateCur(audioST->audioCLK.tailPts - 
                                   (double)(audioST->callBackRequiredSize + len + 
                                   (audioST->resampledBuffer.size - 
                                    audioST->resampledBuffer.idx))/audioST->bytesPerSec);
        logger.commit(LogStrategy::DEFAULT_FORMAT, "audio", audioST->audioCLK);
        audioST->proxy.syncSystenCLK(audioST->audioCLK);
    }
}

void MyAudio::setSpec(SDL_AudioSpec& spec){
    SDL_zero(spec); 
    spec.freq = 48000;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples = 1<<12;
    spec.callback = SDLAudioCallback;
    spec.userdata = this;
}
