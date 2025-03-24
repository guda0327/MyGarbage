#include "./inc/MyPlayer.h"
#include "./inc/logger.h"
// #include <QApplication>
// #include <QLabel>
// #include <QMainWindow>

AsyncLogger logger;

int main(){
    MyPlayer player;
    player.run();
}