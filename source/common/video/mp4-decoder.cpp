#include "mp4-decoder.hpp"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
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

    std::vector<float> decodedAudio;
    unsigned decodedAudioChannels = 0;
    unsigned decodedAudioSampleRate = 0;
    bool hasDecodedAudio = false;

    void freeRgb() {
        if (rgbBuf) {
            av_freep(&rgbBuf);
            rgbBufSize = 0;
        }
    }

    void clearDecodedAudio() {
        decodedAudio.clear();
        decodedAudioChannels = 0;
        decodedAudioSampleRate = 0;
        hasDecodedAudio = false;
    }

    void destroy() {
        clearDecodedAudio();
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

    void appendResampledFrame(SwrContext* swr, AVCodecContext* actx, AVFrame* afrm) {
        const int outCh = 2;
        const int delay = swr_get_delay(swr, actx->sample_rate);
        int maxOut = av_rescale_rnd(delay + afrm->nb_samples, (int)decodedAudioSampleRate, actx->sample_rate, AV_ROUND_UP);
        if (maxOut <= 0) maxOut = afrm->nb_samples * 4;
        if (maxOut <= 0) return;

        const size_t oldFloats = decodedAudio.size();
        decodedAudio.resize(oldFloats + (size_t)maxOut * (size_t)outCh);

        uint8_t* outPtr = reinterpret_cast<uint8_t*>(decodedAudio.data() + oldFloats);
        const uint8_t** inData = const_cast<const uint8_t**>(afrm->extended_data ? afrm->extended_data : afrm->data);
        int converted = swr_convert(swr, &outPtr, maxOut, inData, afrm->nb_samples);
        if (converted < 0) {
            decodedAudio.resize(oldFloats);
            return;
        }
        decodedAudio.resize(oldFloats + (size_t)std::max(0, converted) * (size_t)outCh);
    }

    bool decodeEntireAudioTrack(unsigned outSampleRate) {
        decodedAudioSampleRate = outSampleRate;
        const AVCodec* adec = nullptr;
        int astream = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, &adec, 0);
        if (astream < 0 || !adec) {
            return true;
        }

        AVCodecContext* actx = avcodec_alloc_context3(adec);
        if (!actx) return false;
        if (avcodec_parameters_to_context(actx, fmt->streams[astream]->codecpar) < 0) {
            avcodec_free_context(&actx);
            return false;
        }
        actx->pkt_timebase = fmt->streams[astream]->time_base;
        if (avcodec_open2(actx, adec, nullptr) < 0) {
            std::cerr << "[Mp4Decoder] avcodec_open2 (audio) failed\n";
            avcodec_free_context(&actx);
            return false;
        }

        SwrContext* swr = nullptr;
        AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_STEREO;
        int ret = swr_alloc_set_opts2(
            &swr,
            &outLayout,
            AV_SAMPLE_FMT_FLT,
            (int)outSampleRate,
            &actx->ch_layout,
            actx->sample_fmt,
            actx->sample_rate,
            0,
            nullptr);
        if (ret < 0 || !swr || swr_init(swr) < 0) {
            std::cerr << "[Mp4Decoder] swresample init failed\n";
            if (swr) swr_free(&swr);
            avcodec_free_context(&actx);
            return false;
        }

        AVPacket* apkt = av_packet_alloc();
        AVFrame* afrm = av_frame_alloc();
        if (!apkt || !afrm) {
            av_frame_free(&afrm);
            av_packet_free(&apkt);
            swr_free(&swr);
            avcodec_free_context(&actx);
            return false;
        }

        decodedAudio.clear();

        auto drainDecodedFrames = [&]() {
            for (;;) {
                int r = avcodec_receive_frame(actx, afrm);
                if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) break;
                if (r < 0) break;
                appendResampledFrame(swr, actx, afrm);
                av_frame_unref(afrm);
            }
        };

        while (av_read_frame(fmt, apkt) >= 0) {
            if (apkt->stream_index != astream) {
                av_packet_unref(apkt);
                continue;
            }
            ret = avcodec_send_packet(actx, apkt);
            av_packet_unref(apkt);
            if (ret < 0 && ret != AVERROR(EAGAIN)) {
                break;
            }
            drainDecodedFrames();
        }

        avcodec_send_packet(actx, nullptr);
        drainDecodedFrames();

        int flushLeft = 0;
        while (swr_get_delay(swr, (int)outSampleRate) > 0 && flushLeft++ < 4096) {
            const int maxOut = 512;
            const size_t oldFloats = decodedAudio.size();
            decodedAudio.resize(oldFloats + (size_t)maxOut * 2u);
            uint8_t* outPtr = reinterpret_cast<uint8_t*>(decodedAudio.data() + oldFloats);
            int converted = swr_convert(swr, &outPtr, maxOut, nullptr, 0);
            if (converted <= 0) break;
            decodedAudio.resize(oldFloats + (size_t)converted * 2u);
        }

        av_frame_free(&afrm);
        av_packet_free(&apkt);
        swr_free(&swr);
        avcodec_free_context(&actx);

        if (!decodedAudio.empty()) {
            hasDecodedAudio = true;
            decodedAudioChannels = 2;
            decodedAudioSampleRate = outSampleRate;
        }

        if (avformat_seek_file(fmt, -1, INT64_MIN, 0, INT64_MAX, 0) < 0) {
            if (av_seek_frame(fmt, astream, 0, AVSEEK_FLAG_BACKWARD) < 0) {
                std::cerr << "[Mp4Decoder] avformat_seek_file to start failed\n";
                return false;
            }
        }
        return true;
    }
};

Mp4Decoder::Mp4Decoder() : impl(std::make_unique<Impl>()) {}

Mp4Decoder::~Mp4Decoder() { close(); }

bool Mp4Decoder::open(const std::string& path, unsigned audioOutputSampleRate) {
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

    if (!impl->decodeEntireAudioTrack(audioOutputSampleRate)) {
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

bool Mp4Decoder::takeDecodedAudio(std::vector<float>& outInterleavedPcm, unsigned& outChannels, unsigned& outSampleRate) {
    if (!impl->hasDecodedAudio || impl->decodedAudio.empty()) {
        outInterleavedPcm.clear();
        outChannels = 0;
        outSampleRate = 0;
        return false;
    }
    outInterleavedPcm = std::move(impl->decodedAudio);
    outChannels = impl->decodedAudioChannels;
    outSampleRate = impl->decodedAudioSampleRate;
    impl->clearDecodedAudio();
    return true;
}

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
