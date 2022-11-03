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

    // ���л�int32
    static void Int32Serialization(int32_t size, char *bytes);
    // ����ֻ�й���ͷ����չͷ��û�����ĵ�����
    static bool Send(IPC &ipc, QByteArray &commonHeader, QByteArray &extendHeader);
    // ���ͺ�����ͷ����չͷ���ֽ��������ĵ�����
    static bool Send(IPC &ipc, QByteArray &commonHeader, QByteArray &extendHeader, QByteArray &content);
    // ���ͺ�����ͷ����չͷ���ֽ����鼰��С�������ĵ�����
    static bool Send(IPC &ipc, QByteArray &commonHeader, QByteArray &extendHeader, const char *content, int32_t nbytes);
};


class VideoRequest : public Request
{
public:
    // ������Ƶ��
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
    // ����ֻ�й���ͷ����չͷ��û�����ĵ��¼�����
    static bool Send(IPC &ipc, message::EventHead::Type type);
};


class EventCloseRequest : public Request
{
public:
    // ���͹ر�������
    static bool Send(IPC &ipc);
};


class EventSetIntRequest : public Request
{
public:
    // ���ͺ�����ͷ����չͷ����Ϣ���ͺ�intֵ����Ϣ���ĵ��¼����� (��eq �˾�)
    static bool Send(IPC &ipc, message::EventHead::Type type, int value);
};


class EventSetStringRequest : public Request
{
public:
    // ���ͺ�����ͷ����չͷ����Ϣ���ͺ�char *ֵ����Ϣ���ĵ��¼����� (���ͼ)
    static bool Send(IPC &ipc, message::EventHead::Type type, const char *content, int nbytes);
};



class EventSetBrightnessFilterRequest : public EventSetIntRequest
{
public:
    // �������������˾�������
    static bool Send(IPC &ipc, int brightness);
};


class EventSetContrastFilterRequest : public EventSetIntRequest
{
public:
    // �������öԱȶ��˾�������
    static bool Send(IPC &ipc, int contrast);
};


class EventSetSaturationFilterRequest : public EventSetIntRequest
{
public:
    // �������ñ��Ͷ��˾�������
    static bool Send(IPC &ipc, int saturation);
};


class EventSetGammaFilterrRequest : public EventSetIntRequest
{
public:
    // �������ý���٤���˾�������
    static bool Send(IPC &ipc, int gamma);
};


class EventSetSpeedRequest : public Request
{
public:
    // ��������ٶ�
    static bool Send(IPC &ipc, int speed);
};


class EventTakeSnapshotRequest : public Request
{
public:
    // ��ͼ
    static bool Send(IPC &ipc, const char *path, int nbytes);
};


class EventStartRecordStreamRequest : public Request
{
public:
    // ��ʼ¼��
    static bool Send(IPC& ipc, const char* path, int nbytes);
};


class EventStopRecordStreamRequest : public Request
{
public:
    // ֹͣ¼��
    static bool Send(IPC& ipc);
};



class EventPauseRequest : public Request
{
public:
    // ������ͣ������
    static bool Send(IPC &ipc);
};


class EventResumeRequest : public Request
{
public:
    // ���ͻָ�������
    static bool Send(IPC &ipc);
};


class EventStepForwardRequest : public Request
{
public:
    // ������󲥷�һ֡����
    static bool Send(IPC &ipc);
};


class EventStepBackwardRequest : public Request
{
public:
    // ������ǰ����һ֡����
    static bool Send(IPC &ipc);
};
