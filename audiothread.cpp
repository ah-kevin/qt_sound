//
// Created by bjke on 2022/4/26.
//

#include <QFile>
#include <QDebug>
#include <QDateTime>
#include "audiothread.h"
#include "FFmpegs.h"

extern "C" {
// 设备相关API
#include <libavdevice/avdevice.h>
// 格式相关API
#include <libavformat/avformat.h>
// 工具库(比如错误处理)
#include <libavutil/avutil.h>
}
// 格式名称、设备名称目前暂时使用宏定义固定死
#ifdef Q_OS_WIN
// 格式名称
#define FMT_NAME "dshow"
// 设备名称
#define DEVICE_NAME "audio=麦克风阵列 (Realtek(R) Audio)"
// PCM文件名
#define FILEPATH "D://"
#else
#define FMT_NAME "avfoundation"
#define DEVICE_NAME ":0"
#define FILEPATH "/Users/bjke/workspaces/c++/qt-sound/data/"
#endif

AudioThread::AudioThread(QObject *parent) : QThread(parent) {
    // 当监听到线程结束时候（finished），就调用deleteLater回收
    connect(this, &AudioThread::finished,
            this, &AudioThread::deleteLater);
}

AudioThread::~AudioThread() {
    // 内存回收之前，正常结束线程
    requestInterruption();
    quit();
    wait();
    qDebug() << this << "析构（内存被回收）";
}

void showSpec(AVFormatContext *ctx) {
    // 获取stream输入流
    AVStream *stream = ctx->streams[0];
    // 获取音频参数
    AVCodecParameters *params = stream->codecpar;
    // 声道数
    qDebug() << params->channels;
    // 采样率
    qDebug() << params->sample_rate;

//    // 采样格式（mac不行）
//    qDebug() << params->format;
//    // 每一个样本的一个声道占用多少个字节(mac不行）
//    qDebug() << av_get_bytes_per_sample((AVSampleFormat) params->format);

    // 编码ID（可以看出采样格式）
    qDebug() << params->codec_id;
    // 每一个样本的一个声道占用多少位
    qDebug() << "每一个样本的一个声道占用多少位" << av_get_bits_per_sample(params->codec_id);
}

// 当线程启动的时候(start),就会自动调用run函数
// run函数中的代码是在子线程中执行的x
// 耗时操作应该放在run函数中
void AudioThread::run() {
    qDebug() << this << "开始执行----------";
    // 获取输入格式对象
    const AVInputFormat *fmt = av_find_input_format(FMT_NAME);
    if (!fmt) {
        // 如果找不到输入格式
        qDebug() << "找不到输入格式" << FMT_NAME;
        return;
    }
    // 格式上下文（后面通过格式上下文操作设备）
    AVFormatContext *ctx = nullptr;
//    // -i
//    const char *deviceName = ":0";
    // 选项
    AVDictionary *options = nullptr;
    int ret = avformat_open_input(&ctx, DEVICE_NAME, fmt, &options);
    if (ret < 0) {
        char errbuf[1024];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "打开设备失败" << errbuf;
        return;
    }
    // 打印一下录音设备的参数信息
    showSpec(ctx);
    // 文件名
    QString filename = FILEPATH;
    filename += QDateTime::currentDateTime().toString("MM_dd_HH_mm_ss");
    QString wavname = filename;
    filename += ".pcm";
    wavname += ".wav";
    QFile file(filename);
    // 打开文件
    // WriteOnly:只写模式。如果文件不存在，创建文件;如果文件存在，就删除文件内容
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "打开文件失败" << FILEPATH;
        // 关闭设备
        avformat_close_input(&ctx);
        return;
    };


    // 第一种方式
    // 数据包
//    AVPacket pkt;
//    while (!isInterruptionRequested()) {
//        // 不断采集数据
//        ret = av_read_frame(ctx, &pkt);
//        if (ret == 0){ // 读取成功
//            // 将数据写入文件
//            file.write((const char *) pkt.data, pkt.size);
//        }
//        // 必须要加，释放pkt内部的资源
//        av_packet_unref(&pkt);
//    }

//     暂时假定只采集50个数据包
//    int count = 50;

    // 第二种方式数据包
    AVPacket *pkt = av_packet_alloc();

    while (!isInterruptionRequested()) {
        // 从设备中采集数据，返回值为0，代表采集数据成功
        ret = av_read_frame(ctx, pkt);

        if (ret == 0) { // 读取成功
            // 将数据写入文件
            file.write((const char *) pkt->data, pkt->size);

            // 释放资源
            av_packet_unref(pkt);
        } else if (ret == AVERROR(EAGAIN)) { // 资源临时不可用
            continue;
        } else { // 其他错误
            char errbuf[1024];
            av_strerror(ret, errbuf, sizeof(errbuf));
            qDebug() << "av_read_frame error" << errbuf << ret;
            break;
        }
    }
    // 关闭文件
    file.close();
    // 释放资源
    av_packet_free(&pkt);

    WAVHeader wavHeader;
    // 获取stream输入流
    AVStream *stream = ctx->streams[0];
    // 获取音频参数
    AVCodecParameters *params = stream->codecpar;
    // 采样率
    wavHeader.sampleRate = params->sample_rate;
    // 每一个样本的一个声道占用多少位
    wavHeader.bitsPerSample = av_get_bits_per_sample(params->codec_id);
    // 声道数
    wavHeader.numChannels = params->channels;;
    if (params->codec_id >= AV_CODEC_ID_PCM_F32BE) {
        wavHeader.audioFormat = AUDIO_FORMAT_FLOAT;
    }
    // pcm转wav文件
    FFmpegs::pcm2wav(wavHeader, filename.toUtf8().data(), wavname.toUtf8().data());

    // 关闭设备
    avformat_close_input(&ctx);
    qDebug() << this << "正常结束----------";
}


//void AudioThread::setStop(bool stop) {
//    _stop = stop;
//}
