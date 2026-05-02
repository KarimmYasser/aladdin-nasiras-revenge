#include "mp4-decoder.hpp"

#include <cmath>
#include <cstring>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace our {

struct Mp4Decoder::Impl {
    AVFormatContext* fmt = nullptr;
    AVCodecContext* codec = nullptr;
    const AVCodec* decoder = nullptr;
    SwsContext* sws = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* frameRgb = nullptr;
    AVPacket* pkt = nullptr;
    uint8_t* rgbBuf = nullptr;
    int rgbBufSize = 0;

    int videoStream = -1;
    double timeBase = 0.0;
    double durationSec = 0.0;
    double playback = 0.0;
    double lastShownPts = -1.0e30;
    bool eof = false;

    void freeRgb() {
        if (rgbBuf) {
            av_freep(&rgbBuf);
            rgbBufSize = 0;
        }
    }

    void destroy() {
        freeRgb();
        if (sws) {
            sws_freeContext(sws);
            sws = nullptr;
        }
        if (pkt) {
            av_packet_free(&pkt);
        }
        if (frameRgb) {
            av_frame_free(&frameRgb);
            frameRgb = nullptr;
        }
        if (frame) {
            av_frame_free(&frame);
        }
        if (codec) {
            avcodec_free_context(&codec);
        }
        if (fmt) {
            avformat_close_input(&fmt);
        }
        videoStream = -1;
        timeBase = 0.0;
        durationSec = 0.0;
        playback = 0.0;
        lastShownPts = -1.0e30;
        eof = false;
        decoder = nullptr;
    }

    static double tsToSec(int64_t pts, AVRational tb) {
        if (pts == AV_NOPTS_VALUE) return NAN;
        return av_q2d(tb) * (double)pts;
    }

    bool ensureRgbBuffer(int w, int h) {
        const int need = av_image_get_buffer_size(AV_PIX_FMT_RGBA, w, h, 32);
        if (need <= 0) return false;
        if (need == rgbBufSize && rgbBuf) return true;
        freeRgb();
        rgbBuf = (uint8_t*)av_malloc((size_t)need + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!rgbBuf) return false;
        rgbBufSize = need;
        return true;
    }

    bool scaleToRgba(AVFrame* src, std::vector<uint8_t>& outRgba, int& outW, int& outH) {
        outW = src->width;
        outH = src->height;
        if (!ensureRgbBuffer(outW, outH)) return false;

        if (frameRgb) {
            av_frame_unref(frameRgb);
        } else {
            frameRgb = av_frame_alloc();
        }
        if (!frameRgb) return false;

        frameRgb->format = AV_PIX_FMT_RGBA;
        frameRgb->width = outW;
        frameRgb->height = outH;
        if (av_image_fill_arrays(frameRgb->data, frameRgb->linesize, rgbBuf, AV_PIX_FMT_RGBA, outW, outH, 32) < 0)
            return false;

        sws = sws_getCachedContext(
            sws,
            src->width,
            src->height,
            (AVPixelFormat)src->format,
            outW,
            outH,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr);
        if (!sws) return false;

        if (sws_scale(sws, src->data, src->linesize, 0, src->height, frameRgb->data, frameRgb->linesize) <= 0)
            return false;

        const int stride = frameRgb->linesize[0];
        outRgba.resize((size_t)stride * (size_t)outH);
        for (int y = 0; y < outH; ++y) {
            memcpy(outRgba.data() + (size_t)y * (size_t)outW * 4,
                   frameRgb->data[0] + (size_t)y * (size_t)stride,
                   (size_t)outW * 4);
        }
        return true;
    }

    /// Read muxed packets until one packet is sent to the video decoder (or EOF).
    /// Returns false on fatal error; sets eof on stream end.
    bool sendNextVideoPacket() {
        for (;;) {
            int err = av_read_frame(fmt, pkt);
            if (err < 0) {
                if (err == AVERROR_EOF) {
                    avcodec_send_packet(codec, nullptr);
                    eof = true;
                    return true;
                }
                std::cerr << "[Mp4Decoder] av_read_frame: " << err << "\n";
                return false;
            }

            if (pkt->stream_index != videoStream) {
                av_packet_unref(pkt);
                continue;
            }

            err = avcodec_send_packet(codec, pkt);
            av_packet_unref(pkt);
            if (err < 0 && err != AVERROR(EAGAIN)) {
                std::cerr << "[Mp4Decoder] avcodec_send_packet: " << err << "\n";
                return false;
            }
            return true;
        }
    }
};

Mp4Decoder::Mp4Decoder() : impl(std::make_unique<Impl>()) {}

Mp4Decoder::~Mp4Decoder() { close(); }

bool Mp4Decoder::open(const std::string& path) {
    close();
    impl->pkt = av_packet_alloc();
    impl->frame = av_frame_alloc();
    if (!impl->pkt || !impl->frame) return false;

    if (avformat_open_input(&impl->fmt, path.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "[Mp4Decoder] avformat_open_input failed: " << path << "\n";
        close();
        return false;
    }
    if (avformat_find_stream_info(impl->fmt, nullptr) < 0) {
        std::cerr << "[Mp4Decoder] avformat_find_stream_info failed\n";
        close();
        return false;
    }

    impl->videoStream = av_find_best_stream(impl->fmt, AVMEDIA_TYPE_VIDEO, -1, -1, &impl->decoder, 0);
    if (impl->videoStream < 0 || !impl->decoder) {
        std::cerr << "[Mp4Decoder] No video stream\n";
        close();
        return false;
    }

    AVStream* st = impl->fmt->streams[impl->videoStream];
    impl->timeBase = av_q2d(st->time_base);
    if (st->duration > 0) {
        impl->durationSec = (double)st->duration * impl->timeBase;
    } else if (impl->fmt->duration > 0) {
        impl->durationSec = (double)impl->fmt->duration / (double)AV_TIME_BASE;
    } else {
        impl->durationSec = 0.0;
    }

    impl->codec = avcodec_alloc_context3(impl->decoder);
    if (!impl->codec) {
        close();
        return false;
    }
    if (avcodec_parameters_to_context(impl->codec, st->codecpar) < 0) {
        close();
        return false;
    }
    impl->codec->pkt_timebase = st->time_base;
    if (avcodec_open2(impl->codec, impl->decoder, nullptr) < 0) {
        std::cerr << "[Mp4Decoder] avcodec_open2 failed\n";
        close();
        return false;
    }

    impl->playback = 0.0;
    impl->lastShownPts = -1.0e30;
    impl->eof = false;
    return true;
}

void Mp4Decoder::close() { impl->destroy(); }

bool Mp4Decoder::isOpen() const { return impl->fmt != nullptr && impl->codec != nullptr; }

double Mp4Decoder::durationSeconds() const { return impl->durationSec; }

bool Mp4Decoder::advance(float deltaSeconds, std::vector<uint8_t>& outRgba, int& outWidth, int& outHeight) {
    if (!isOpen()) return false;

    impl->playback += (double)deltaSeconds;
    const double target = impl->playback;
    constexpr double kSlop = 1.0 / 60.0;

    AVStream* st = impl->fmt->streams[impl->videoStream];

    for (int guard = 0; guard < 4096; ++guard) {
        int err = avcodec_receive_frame(impl->codec, impl->frame);
        if (err == 0) {
            double ptsSec = Impl::tsToSec(impl->frame->best_effort_timestamp, st->time_base);
            if (std::isnan(ptsSec)) {
                ptsSec = impl->lastShownPts > -1.0e20 ? impl->lastShownPts + 1.0 / 30.0 : 0.0;
            }

            if (ptsSec + kSlop >= target) {
                if (!impl->scaleToRgba(impl->frame, outRgba, outWidth, outHeight)) {
                    av_frame_unref(impl->frame);
                    return false;
                }
                impl->lastShownPts = ptsSec;
                av_frame_unref(impl->frame);
                return true;
            }
            av_frame_unref(impl->frame);
            continue;
        }

        if (err == AVERROR(EAGAIN)) {
            if (!impl->sendNextVideoPacket()) return false;
            if (impl->eof) {
                // Drain remaining decoded frames after flush
                continue;
            }
            continue;
        }

        if (err == AVERROR_EOF) {
            impl->eof = true;
            return false;
        }

        std::cerr << "[Mp4Decoder] avcodec_receive_frame " << err << "\n";
        return false;
    }

    std::cerr << "[Mp4Decoder] advance guard hit\n";
    return false;
}

} // namespace our
