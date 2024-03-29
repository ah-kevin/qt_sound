#include <QApplication>
#include <QTranslator>
#include "mainWindow.h"
#include "FFmpegs.h"

// 纯C语言的
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
}

int main(int argc, char *argv[]) {
//    WAVHeader wavHeader;
//    wavHeader.sampleRate=44100;
//    wavHeader.bitsPerSample = 32;
//    wavHeader.numChannels =1;
//    wavHeader.audioFormat = 3;
//    // pcm转wav文件
//    FFmpegs::pcm2wav(wavHeader, "/Users/bjke/workspaces/c++/qt-sound/data/48000_f32le_1.pcm",
//                     "/Users/bjke/workspaces/c++/qt-sound/data/48000_f32le_1.wav");

    // 注册设备
    avdevice_register_all();
    // 打印版本号
    qDebug() << av_version_info();
    // 创建一个QApplication对象
    // 调用QApplication的构造函数时，传递了2个参数
    // 一个Qt程序中永远只有一个QApplication对象
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle("hello world");
    w.show();

    // 运行QT程序
    return QApplication::exec();
}