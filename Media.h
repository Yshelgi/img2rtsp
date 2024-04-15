//
// Created by shelgi on 2024/4/16.
//

#ifndef IMG2RTSP_MEDIA_H
#define IMG2RTSP_MEDIA_H
#include<iostream>

struct AVFormatContext;
struct AVCodecContext;
struct AVRational;
struct AVPacket;
struct AVFrame;
struct AVCodec;
struct AVStream;
struct SwsContext;
struct AVBufferRef;
struct AVInputFormat;

class Media {
public:
    Media(std::string rtspPath,int frameRate);
    ~Media();
    bool open();
    void push();

private:
    static void initFFmpeg();
    bool initObject();
    void free();
    void clear();

    std::string m_rtspPath;
    int m_frameRate = 0;
    int m_videoIndex = -1;
    int m_bufferSize = 0;

    AVCodecContext* m_inputCodecContext = nullptr;
    AVFormatContext* m_inputFormatContext = nullptr;

    AVCodecContext* m_outputCodeContext = nullptr;
    AVFormatContext* m_outputFormatContext = nullptr;
    const AVCodec* m_outputCodec = nullptr;
    AVStream* m_outputStream = nullptr;

    SwsContext* m_swsContext = nullptr;
    AVPacket* m_packet = nullptr;
    AVFrame* m_Frame = nullptr;
    AVFrame* m_YUVFrame = nullptr;
    const AVInputFormat* m_inputFormat = nullptr;
};


#endif //IMG2RTSP_MEDIA_H
