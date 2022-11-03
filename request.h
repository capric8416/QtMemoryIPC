#pragma once

// project
#include "ipc.h"
#include "../proto/message.pb.h"

// c/c++
#include <mutex>


class Request
{
public:
    static std::mutex sMutex;

    // 序列化int32
    static void Int32Serialization(int32_t size, char *bytes);
    // 发送只有公共头和扩展头，没有正文的请求
    static bool Send(IPC &ipc, QByteArray &commonHeader, QByteArray &extendHeader);
    // 发送含公共头、扩展头和字节数组正文的请求
    static bool Send(IPC &ipc, QByteArray &commonHeader, QByteArray &extendHeader, QByteArray &content);
    // 发送含公共头、扩展头、字节数组及大小描述正文的请求
    static bool Send(IPC &ipc, QByteArray &commonHeader, QByteArray &extendHeader, const char *content, int32_t nbytes);
};


class VideoRequest : public Request
{
public:
    // 发送视频流
    static bool Send(
        IPC &ipc, char *content, uint32_t nbytes,
        enum message::VideoHead_FrameType type,
        enum message::VideoHead_Codec codec = message::VideoHead_Codec_NoMansLand1,
        uint32_t sequence = 0, uint32_t width = 0, uint32_t height = 0, uint64_t dts = 0, uint64_t pts = 0
    );
};


class EventSimpleRequest : public Request
{
public:
    // 发送只有公共头和扩展头，没有正文的事件请求
    static bool Send(IPC &ipc, message::EventHead::Type type);
};


class EventCloseRequest : public Request
{
public:
    // 发送关闭流请求
    static bool Send(IPC &ipc);
};


class EventSetIntRequest : public Request
{
public:
    // 发送含公共头、扩展头、消息类型和int值的消息正文的事件请求 (如eq 滤镜)
    static bool Send(IPC &ipc, message::EventHead::Type type, int value);
};


class EventSetStringRequest : public Request
{
public:
    // 发送含公共头、扩展头、消息类型和char *值的消息正文的事件请求 (如截图)
    static bool Send(IPC &ipc, message::EventHead::Type type, const char *content, int nbytes);
};



class EventSetBrightnessFilterRequest : public EventSetIntRequest
{
public:
    // 发送设置亮度滤镜的请求
    static bool Send(IPC &ipc, int brightness);
};


class EventSetContrastFilterRequest : public EventSetIntRequest
{
public:
    // 发送设置对比度滤镜的请求
    static bool Send(IPC &ipc, int contrast);
};


class EventSetSaturationFilterRequest : public EventSetIntRequest
{
public:
    // 发送设置饱和度滤镜的请求
    static bool Send(IPC &ipc, int saturation);
};


class EventSetGammaFilterrRequest : public EventSetIntRequest
{
public:
    // 发送设置近似伽马滤镜的请求
    static bool Send(IPC &ipc, int gamma);
};


class EventSetSpeedRequest : public Request
{
public:
    // 变更播放速度
    static bool Send(IPC &ipc, int speed);
};


class EventTakeSnapshotRequest : public Request
{
public:
    // 截图
    static bool Send(IPC &ipc, const char *path, int nbytes);
};


class EventStartRecordStreamRequest : public Request
{
public:
    // 开始录像
    static bool Send(IPC& ipc, const char* path, int nbytes);
};


class EventStopRecordStreamRequest : public Request
{
public:
    // 停止录像
    static bool Send(IPC& ipc);
};



class EventPauseRequest : public Request
{
public:
    // 发送暂停流请求
    static bool Send(IPC &ipc);
};


class EventResumeRequest : public Request
{
public:
    // 发送恢复流请求
    static bool Send(IPC &ipc);
};


class EventStepForwardRequest : public Request
{
public:
    // 发送向后播放一帧请求
    static bool Send(IPC &ipc);
};


class EventStepBackwardRequest : public Request
{
public:
    // 发送向前播放一帧请求
    static bool Send(IPC &ipc);
};
