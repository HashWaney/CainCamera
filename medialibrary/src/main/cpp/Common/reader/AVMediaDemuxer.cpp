//
// Created by CainHuang on 2020-01-10.
//

#include "AVMediaDemuxer.h"

AVMediaDemuxer::AVMediaDemuxer() {
    iformat = nullptr;
    pFormatCtx = nullptr;
    mInputPath = nullptr;
}

AVMediaDemuxer::~AVMediaDemuxer() {
    if (mInputPath) {
        av_freep(&mInputPath);
        mInputPath = nullptr;
    }
}

/**
 * 设置输入文件
 * @param path
 */
void AVMediaDemuxer::setInputPath(const char *path) {
    mInputPath = av_strdup(path);
}

/**
 * 设置输入格式
 * @param format
 */
void AVMediaDemuxer::setInputFormat(const char *format) {
    iformat = av_find_input_format(format);
    if (!iformat) {
        LOGE("Unknown input format: %s", format);
    }
}

/**
 * 打开解复用器
 * @param formatOptions
 * @return
 */
int AVMediaDemuxer::openDemuxer(std::map<std::string, std::string> formatOptions) {
    if (!mInputPath) {
        LOGE("input path is null");
        return -1;
    }
    int ret;
    AVDictionary *options = nullptr;
    auto it = formatOptions.begin();
    for (; it != formatOptions.end(); it++) {
        av_dict_set(&options, (*it).first.c_str(), (*it).second.c_str(), 0);
    }
    if ((ret = avformat_open_input(&pFormatCtx, mInputPath, iformat, &options)) < 0) {
        LOGE("Failed to call avformat_open_input: %s, error: %s", mInputPath, av_err2str(ret));
        av_dict_free(&options);
        return ret;
    }
    av_dict_free(&options);

    // 设置时钟
    mDuration = pFormatCtx->duration;

    // 查找媒体流信息
    if ((ret = avformat_find_stream_info(pFormatCtx, nullptr)) < 0) {
        LOGE("Failed to call avformat_find_stream_info - %s", av_err2str(ret));
        return ret;
    }
    return 0;
}

/**
 * 定位到某个时间点
 * @param timeMs 时间/ms
 * @return
 */
int AVMediaDemuxer::seekTo(float timeMs) {
    if (mDuration < 0) {
        return -1;
    }
    int seekFlags = 0;
    seekFlags &= ~AVSEEK_FLAG_BYTE;
    int64_t start_time = 0;
    int64_t seek_pos = av_rescale(timeMs, AV_TIME_BASE, 1000);
    start_time = pFormatCtx ? pFormatCtx->start_time : 0;
    if (start_time > 0 && start_time != AV_NOPTS_VALUE) {
        seek_pos += start_time;
    }
    int ret = avformat_seek_file(pFormatCtx, -1, INT64_MIN, seek_pos, INT64_MAX, seekFlags);
    if (ret < 0) {
        LOGE("%s: could not seek to position %0.3f\n", mInputPath, (double)seek_pos / AV_TIME_BASE);
        return ret;
    }
    return 0;
}

/**
 * 关闭解复用器
 */
void AVMediaDemuxer::closeDemuxer() {
    if (pFormatCtx != nullptr) {
        avformat_close_input(&pFormatCtx);
        pFormatCtx = nullptr;
    }
}

/**
 * 打开解复用器信息
 */
void AVMediaDemuxer::printInfo() {
    if (pFormatCtx && mInputPath) {
        av_dump_format(pFormatCtx, 0, mInputPath, 0);
    }
}

/**
 * 获取总时长
 * @return
 */
int64_t AVMediaDemuxer::getDuration() {
    return mDuration;
}

/**
 * 获取解复用上下文
 * @return
 */
AVFormatContext* AVMediaDemuxer::getContext() {
    return pFormatCtx;
}

/**
 * 判断是否存在音频流
 * @return
 */
bool AVMediaDemuxer::hasAudioStream() {
    if (!pFormatCtx) {
        return false;
    }
    return (av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0) >= 0);
}

/**
 * 判断是否存在视频流
 * @return
 */
bool AVMediaDemuxer::hasVideoStream() {
    if (!pFormatCtx) {
        return false;
    }
    return (av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0) >= 0);
}