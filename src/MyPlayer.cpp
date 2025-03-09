#include "../inc/MyPlayer.h"
#include "../inc/video.h"

MyPlayer::MyPlayer():proxy(),
                     demuxer(proxy),
                     video(proxy),
                     audio(proxy)
                     {

}

void MyPlayer::run(){
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER)<0) std::runtime_error("SDL_Init failed\n");
    std::thread demuxerTh([this](){demuxer.run();});
    std::thread audioTh([this](){audio.run();});
    video.run();

    demuxerTh.join();
    audioTh.join();
    SDL_Quit();
}
