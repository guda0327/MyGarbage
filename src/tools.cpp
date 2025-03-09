#include "../inc/tools.h"

void swrCtxDeleter(SwrContext* ptr){
    if(ptr){
        swr_free(&ptr);
    }
}

void audioDeviceDeleter(SDL_AudioDeviceID* id){
    if(id){
        SDL_CloseAudioDevice(*id);
        delete id;
    }
}

void channelLayoutDeleter(AVChannelLayout* ptr){
    if(ptr){
        av_channel_layout_uninit(ptr);
        delete ptr;
    }
}

void frameBufferDeleter(uint8_t** ptr){
    if(ptr && *ptr){
        av_freep(&ptr[0]);
    }
}

void avFormatCtxDeleter(AVFormatContext* ctx){
    if(ctx){
        avformat_close_input(&ctx);
    }
}

void avCodecCtxDeleter(AVCodecContext* ctx){
    if(ctx){
        avcodec_free_context(&ctx);
    }
}

void avPacketDeleter(AVPacket* pkg){
    if(pkg){
        av_packet_unref(pkg);
        av_packet_free(&pkg);
    }
}

void windowDeleter(SDL_Window* windowPtr){
    if(windowPtr){
        SDL_DestroyWindow(windowPtr);
    }
}

void rendererDeleter(SDL_Renderer* rendererPtr){
    if(rendererPtr){
        SDL_DestroyRenderer(rendererPtr);
    }
}

void textureDeleter(SDL_Texture* texturePtr){
    if(texturePtr){
        SDL_DestroyTexture(texturePtr);
    }
}

void frameDeleter(AVFrame* framePtr){
    if(framePtr){
        av_frame_free(&framePtr);
    }
}

void swsDeleter(SwsContext* swsPtr){
    if(swsPtr){
        sws_freeContext(swsPtr);
    }
}

double getCurPts(SyncClock& clock){
    double curTime = av_gettime_relative()/1e6;
    return curTime + clock.diff;
}