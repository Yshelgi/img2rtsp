//
// Created by shelgi on 2024/4/16.
//

#include "Media.h"
#include <utility>

extern "C" {        // 用C规则编译指定的代码
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavdevice/avdevice.h"
}

#define USE_WINDOWS 1

void Media::initFFmpeg() {
    avdevice_register_all();
    avdevice_register_all();
    avformat_network_init();
}


Media::Media(std::string rtspPath, int frameRate) {
    initFFmpeg();
    m_rtspPath = std::move(rtspPath);
    m_frameRate = frameRate;

    // 从屏幕截图
#if USE_WINDOWS
    m_inputFormat = av_find_input_format("gdigrab");
#else
    m_inputFormat = av_find_input_format("x11grab");
#endif
    if(!m_inputFormat){
        std::cout<<"[Error] 截图初始化失败  \n";
    }
    m_inputFormatContext = avformat_alloc_context();
}

Media::~Media() {
    clear();
    free();
}


bool Media::initObject() {
    m_packet = av_packet_alloc();
    m_Frame = av_frame_alloc();
    m_YUVFrame = av_frame_alloc();

    std::cout<<"[INFO] grab size: "<<m_inputCodecContext->width<<" "<<m_inputCodecContext->height<<"\n";

    m_YUVFrame->format = AV_PIX_FMT_YUV420P;
    m_YUVFrame->width = m_inputCodecContext->width;
    m_YUVFrame->height = m_inputCodecContext->height;

    m_bufferSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,m_inputCodecContext->width,m_inputCodecContext->height,4);
    m_swsContext = sws_getContext(m_inputCodecContext->width,m_inputCodecContext->height,(AVPixelFormat)m_inputCodecContext->pix_fmt,
                                        m_inputCodecContext->width,m_inputCodecContext->height,AV_PIX_FMT_YUV420P,SWS_BILINEAR,
                                        nullptr, nullptr, nullptr);
    if(!m_packet || !m_Frame || !m_swsContext){
        return false;
    }

    // 定义输出流
    avformat_alloc_output_context2(&m_outputFormatContext, nullptr,"rtsp",m_rtspPath.c_str());

    //定义传输协议
    av_opt_set(m_outputFormatContext->priv_data,"rtsp_transport","tcp",0);
    av_dump_format(m_outputFormatContext,0,m_outputFormatContext->url,1);
    m_outputCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    m_outputCodeContext = avcodec_alloc_context3(m_outputCodec);

    m_outputCodeContext->pix_fmt = AV_PIX_FMT_YUV420P;
    m_outputCodeContext->codec_type = AVMEDIA_TYPE_VIDEO;
    m_outputCodeContext->width = m_inputCodecContext->width;
    m_outputCodeContext->height = m_inputCodecContext->height;
    m_outputCodeContext->framerate = {m_frameRate,1};
    m_outputCodeContext->time_base = {1,m_frameRate};

    // 输出流码率,I帧，B帧以及间隔设置
    m_outputCodeContext->bit_rate = 4000000;
    m_outputCodeContext->gop_size =10;
    m_outputCodeContext->max_b_frames = 0;
    m_outputCodeContext->time_base.num = 1;
    m_outputCodeContext->time_base.den = 25;
    m_outputCodeContext->qmin = 10;
    m_outputCodeContext->qmax = 51;
    if(m_outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER){
        m_outputCodeContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    AVDictionary* param = nullptr;
    av_opt_set(m_outputCodeContext->priv_data,"tune","zerolatency",AV_DICT_MATCH_CASE);
    avcodec_open2(m_outputCodeContext,m_outputCodec,&param);

    av_dict_free(&param);
    // 创建输出流
    m_outputStream = avformat_new_stream(m_outputFormatContext,m_outputCodec);
    m_outputStream->time_base.num = 1;
    m_outputStream->time_base.den = 25;
    m_outputStream->codecpar->codec_type = m_outputCodeContext->codec_type;
    avcodec_parameters_from_context(m_outputStream->codecpar,m_outputCodeContext);
    av_dump_format(m_outputFormatContext,0,m_outputFormatContext->url,1);

    return true;
}


bool Media::open() {
    int ret;
    AVDictionary* dict = nullptr;
    AVStream* videoStream = nullptr;
    const AVCodec* inputCodec = nullptr;
#if USE_WINDOWS
    av_dict_set(&dict, "framerate", std::to_string(m_frameRate).c_str(), 0);
    av_dict_set(&dict,"draw_mouse","1",0);
#endif
    av_dict_set(&dict,"video_size","1920x1080",0);
    av_dict_set(&dict,"preset","ultrafast",0);
    av_dict_set(&dict,"probesize","42M",0);

#if USE_WINDOWS
    ret = avformat_open_input(&m_inputFormatContext,"desktop",m_inputFormat,&dict);
#else
    ret = avformat_open_input(&m_inputFormatContext,"",m_inputFormat,&dict);
#endif

    if(ret<0){
        std::cout<<"[ERROR] 截取屏幕失败"<<__FUNCTION__ <<" "<<__LINE__<<" \n";
        free();
        return false;
    }

    ret = avformat_find_stream_info(m_inputFormatContext, nullptr);
    if(ret < 0){
        std::cout<<"[ERROR] stream info get error"<<__FUNCTION__ <<" "<<__LINE__<<"\n";
    }

    // 根据视频找到对应解码器
    m_videoIndex = av_find_best_stream(m_inputFormatContext,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0);
    videoStream = m_inputFormatContext->streams[m_videoIndex];
    inputCodec = avcodec_find_decoder(videoStream->codecpar->codec_id);

    //分配解码器
    m_inputCodecContext = avcodec_alloc_context3(inputCodec);
    avcodec_parameters_to_context(m_inputCodecContext,videoStream->codecpar);
    m_inputCodecContext->flags |= AV_CODEC_FLAG2_FAST;
    m_inputCodecContext->thread_count = 12;

    // 打开编码器
    avcodec_open2(m_inputCodecContext, inputCodec, nullptr);
    initObject();

    if(dict){
        av_dict_free(&dict);
        dict = nullptr;
    }

    return false;
}


void Media::push() {
    int ret = 0;
    int frameIndex = 0;
    AVPacket* pkt = av_packet_alloc();
    uint8_t* outBuffer = (uint8_t*) av_malloc(m_bufferSize);
    av_image_fill_arrays(m_YUVFrame->data,m_YUVFrame->linesize,
                         outBuffer,AV_PIX_FMT_YUV420P,
                         m_inputCodecContext->width,m_inputCodecContext->height,4);

    if(!(m_outputFormatContext->oformat->flags & AVFMT_NOFILE)){
        if(avio_open(&m_outputFormatContext->pb,m_outputFormatContext->url,AVIO_FLAG_WRITE) < 0){
            std::cout<<"输出上下文创建错误 "<<__FUNCTION__<<" "<<__LINE__<<"\n";
            return;
        }
    }

    // 写输出文件头
    ret = avformat_write_header(m_outputFormatContext, nullptr);
    if(ret<0){
        std::cout<<"[ERROR] 文件头写入失败 "<<__FUNCTION__<<" "<<__LINE__<<"\n";
    }

    av_new_packet(pkt,m_bufferSize);

    // 当有屏幕帧可以读入时
    while(av_read_frame(m_inputFormatContext,m_packet) >= 0){
        if(m_packet->stream_index == m_videoIndex){
            avcodec_send_packet(m_inputCodecContext,m_packet);
            avcodec_receive_frame(m_inputCodecContext,m_Frame);
            sws_scale(m_swsContext,m_Frame->data,m_Frame->linesize,0,
                      m_inputCodecContext->height,m_YUVFrame->data,m_YUVFrame->linesize);

            avcodec_send_frame(m_outputCodeContext,m_YUVFrame);
            avcodec_receive_packet(m_outputCodeContext,pkt);

            pkt->stream_index = m_outputStream->index;
            AVRational time_base = m_outputStream->time_base;//{ 1, 1000 };
            AVRational r_framerate1 = { 50, 2 };//{ 50, 2 };
            int64_t calc_pts = (double)frameIndex * (AV_TIME_BASE) * (1 / av_q2d(r_framerate1));
            pkt->pts = av_rescale_q(calc_pts, { 1, AV_TIME_BASE }, time_base);
            pkt->dts = pkt->pts;
            frameIndex++;
            av_interleaved_write_frame(m_outputFormatContext,pkt);
        }
        av_packet_unref(m_packet);
        av_packet_unref(pkt);
        av_frame_unref(m_Frame);
    }
    av_packet_free(&m_packet);
    av_write_trailer(m_outputFormatContext);
    delete outBuffer;
    outBuffer = nullptr;
}


// 清空缓冲区
void Media::clear(){
    if(m_inputFormatContext && m_inputFormatContext->pb){
        avio_flush(m_inputFormatContext->pb);
    }
    if(m_inputFormatContext){
        avformat_flush(m_inputFormatContext);
    }

    if(m_outputFormatContext && m_outputFormatContext->pb){
        avio_flush(m_outputFormatContext->pb);
    }
    if(m_outputFormatContext){
        avformat_flush(m_outputFormatContext);
    }
}

void Media::free() {
    if(m_swsContext){
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    if(m_inputCodecContext || m_outputCodeContext){
        avcodec_free_context(&m_inputCodecContext);
        avcodec_free_context(&m_outputCodeContext);
        m_inputCodecContext = nullptr;
        m_outputCodeContext = nullptr;
    }

    if(m_packet){
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }

    if(m_Frame){
        av_frame_free(&m_Frame);
        m_Frame = nullptr;
    }

    if(m_YUVFrame){
        av_frame_free(&m_YUVFrame);
        m_YUVFrame = nullptr;
    }

    if(m_inputFormatContext || m_outputFormatContext){
        avformat_close_input(&m_inputFormatContext);
        avformat_close_input(&m_outputFormatContext);
        m_inputFormatContext = nullptr;
        m_outputFormatContext = nullptr;
    }

}