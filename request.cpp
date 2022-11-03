// self
#include "request.h"

// project
#include "../logger/logger.h"



std::mutex Request::sMutex;


void Request::Int32Serialization(int32_t size, char *bytes)
{
	// size 序列化成 byte array
	char pBufferSize[sizeof(size)];
	std::copy(static_cast<const char *>(static_cast<const void *>(&size)),
		static_cast<const char *>(static_cast<const void *>(&size)) + sizeof(size),
		bytes);
}


bool Request::Send(IPC &ipc, QByteArray &commonHeader, QByteArray &extendHeader)
{
	// size 序列化成 byte array
	int32_t nbytesCommonHeader = commonHeader.size();
	char pBufferSize[sizeof(nbytesCommonHeader)];
	Int32Serialization(nbytesCommonHeader, pBufferSize);

	// 加锁
	std::lock_guard<std::mutex> locker(sMutex);

	IPC::WriteError error;

	// 写 size
	bool status = ipc.Write(pBufferSize, sizeof(nbytesCommonHeader), error);
	if (!status) {
		LogWarning() << QString("ipc write common header size fail, nbytes: %1, error: %2\n").arg(sizeof(nbytesCommonHeader)).arg((qint32)error);
		return false;
	}

	// 写 common header
	status = ipc.Write(commonHeader, error);
	if (!status) {
		LogWarning() << QString("ipc write common header fail, nbytes: %1, error: %2\n").arg(commonHeader.size()).arg((qint32)error);
		return false;
	}

	// 写 extend header
	status = ipc.Write(extendHeader, error);
	if (!status) {
		LogWarning() << QString("ipc write extend header fail, nbytes: %1, error: %2\n").arg(extendHeader.size()).arg((qint32)error);
		return false;
	}

	return true;
}


bool Request::Send(IPC &ipc, QByteArray &commonHeader, QByteArray &extendHeader, QByteArray &content)
{
	// size 序列化成 byte array
	int32_t nbytesCommonHeader = commonHeader.size();
	char pBufferSize[sizeof(nbytesCommonHeader)];
	Int32Serialization(nbytesCommonHeader, pBufferSize);

	// 加锁
	std::lock_guard<std::mutex> locker(sMutex);

	IPC::WriteError error;

	// 写 size
	bool status = ipc.Write(pBufferSize, sizeof(nbytesCommonHeader), error);
	if (!status) {
		LogWarning() << QString("ipc write common header size fail, nbytes: %1, error: %2\n").arg(sizeof(nbytesCommonHeader)).arg((qint32)error);
		return false;
	}

	// 写 common header
	status = ipc.Write(commonHeader, error);
	if (!status) {
		LogWarning() << QString("ipc write common header fail, nbytes: %1, error: %2\n").arg(commonHeader.size()).arg((qint32)error);
		return false;
	}

	// 写 extend header
	status = ipc.Write(extendHeader, error);
	if (!status) {
		LogWarning() << QString("ipc write extend header fail, nbytes: %1, error: %2\n").arg(extendHeader.size()).arg((qint32)error);
		return false;
	}

	// 写 content
	status = ipc.Write(content, error);
	if (!status) {
		LogWarning() << QString("ipc write content fail, nbytes: %1, error: %2\n").arg(content.size()).arg((qint32)error);
		return false;
	}

	return true;
}


bool Request::Send(IPC &ipc, QByteArray &commonHeader, QByteArray &extendHeader, const char *content, int32_t nbytes)
{
	// size 序列化成 byte array
	int32_t nbytesCommonHeader = commonHeader.size();
	char pBufferSize[sizeof(nbytesCommonHeader)];
	Int32Serialization(nbytesCommonHeader, pBufferSize);

	// 加锁
	std::lock_guard<std::mutex> locker(sMutex);

	IPC::WriteError error;

	// 写 size
	bool status = ipc.Write(pBufferSize, sizeof(nbytesCommonHeader), error);
	if (!status) {
		LogWarning() << QString("ipc write common header size fail, nbytes: %1, error: %2\n").arg(sizeof(nbytesCommonHeader)).arg((qint32)error);
		return false;
	}

	// 写 common header
	status = ipc.Write(commonHeader, error);
	if (!status) {
		LogWarning() << QString("ipc write common header fail, nbytes: %1, error: %2\n").arg(commonHeader.size()).arg((qint32)error);
		return false;
	}

	// 写 extend header
	status = ipc.Write(extendHeader, error);
	if (!status) {
		LogWarning() << QString("ipc write extend header fail, nbytes: %1, error: %2\n").arg(extendHeader.size()).arg((qint32)error);
		return false;
	}

	// 写 content
	status = ipc.Write(content, nbytes, error);
	if (!status) {
		LogWarning() << QString("ipc write content fail, nbytes: %1, error: %2\n").arg(nbytes).arg((qint32)error);
		return false;
	}

	return true;
}


bool VideoRequest::Send(
	IPC &ipc, char *content, uint32_t nbytes,
	enum message::VideoHead_FrameType type, enum message::VideoHead_Codec codec,
	uint32_t sequence, uint32_t width, uint32_t height, uint64_t dts, uint64_t pts
)
{
	// 扩展消息头
	message::VideoHead videoHeader;
	videoHeader.set_next_size(nbytes);
	videoHeader.set_partial(false);  // 如果这是一个完整的I/P/B帧，将此字段设置为false
	videoHeader.set_codec(codec);  // 可选：编码类型
	videoHeader.set_type(type);  // 可选： 帧类型
	videoHeader.set_sequence(sequence);  // 可选：帧序号
	videoHeader.set_width(width);  // 可选：图像宽
	videoHeader.set_height(height);  // 可选：图像高
	videoHeader.set_dts(dts);  // 可选：送解码时间
	videoHeader.set_pts(pts);  // 可选： 送显示时间

	// video header size
	int32_t nbytesVideoHeader = videoHeader.ByteSize();

	// 基础消息头
	message::CommonHead commonheader;
	// 设置 消息类型
	commonheader.set_type(message::CommonHead_Type_Video);
	// 设置 是否有扩展头部
	commonheader.set_extend(true);
	// 设置 紧接下来的块大小 (如果没有扩展头部，那这里就是正文大小；如果有扩展头部，那就是扩展头部大小，扩展头部里面再设置正文大小)
	commonheader.set_next_size(nbytesVideoHeader);

	// common header size
	int32_t nbytesCommonHeader = commonheader.ByteSize();

	// common header 序列化成 byte array
	QByteArray pBufferCommonHeader(nbytesCommonHeader, 0);
	if (!commonheader.SerializeToArray(pBufferCommonHeader.data(), nbytesCommonHeader)) {
		LogWarningC("serilize common header fail\n");
		return false;
	}

	// video header 序列化成 byte array
	QByteArray pBufferVideoHeader(nbytesVideoHeader, 0);
	if (!videoHeader.SerializeToArray(pBufferVideoHeader.data(), nbytesVideoHeader)) {
		LogWarningC("serilize video header fail\n");
		return false;
	}

	// content 无需序列化

	if (!Request::Send(ipc, pBufferCommonHeader, pBufferVideoHeader, content, nbytes)) {
		LogWarningC("send video fail\n");
		return false;
	}

	return true;
}


bool EventSimpleRequest::Send(IPC &ipc, message::EventHead::Type type)
{
	// 扩展的事件消息头
	message::EventHead eventHeader;
	eventHeader.set_next_size(0);  // 没有正文
	eventHeader.set_type(type);  // 设置事件类型

	// event header size
	int32_t nbytesEventHeader = eventHeader.ByteSize();

	// 基础消息头
	message::CommonHead commonHeader;
	commonHeader.set_type(message::CommonHead_Type_Event);
	commonHeader.set_extend(true);
	commonHeader.set_next_size(nbytesEventHeader);

	// common header 序列化成 byte array
	QByteArray pBufferCommonHeader(commonHeader.ByteSize(), 0);
	if (!commonHeader.SerializeToArray(pBufferCommonHeader.data(), commonHeader.ByteSize())) {
		LogWarningC("serilize common header fail\n");
		return false;
	}

	// event header 序列化成 byte array
	QByteArray pBufferEventHeader(nbytesEventHeader, 0);
	if (!eventHeader.SerializeToArray(pBufferEventHeader.data(), nbytesEventHeader)) {
		LogWarningC("serilize event header fail\n");
		return false;
	}

	// 没有正文

	// 发送
	return Request::Send(ipc, pBufferCommonHeader, pBufferEventHeader);
}


bool EventCloseRequest::Send(IPC &ipc)
{
	return EventSimpleRequest::Send(ipc, message::EventHead_Type_Close);
}


bool EventSetIntRequest::Send(IPC &ipc, message::EventHead::Type type, int value)
{
	// 扩展的事件消息头
	message::EventHead eventHeader;
	eventHeader.set_next_size(sizeof(int));  // 正文是int型
	eventHeader.set_type(type);  // 设置type

	// event header size
	int32_t nbytesEventHeader = eventHeader.ByteSize();

	// 基础消息头
	message::CommonHead commonHeader;
	commonHeader.set_type(message::CommonHead_Type_Event);
	commonHeader.set_extend(true);
	commonHeader.set_next_size(nbytesEventHeader);

	// common header 序列化成 byte array
	QByteArray pBufferCommonHeader(commonHeader.ByteSize(), 0);
	if (!commonHeader.SerializeToArray(pBufferCommonHeader.data(), commonHeader.ByteSize())) {
		LogWarningC("serilize common header fail\n");
		return false;
	}

	// event header 序列化成 byte array
	QByteArray pBufferEventHeader(nbytesEventHeader, 0);
	if (!eventHeader.SerializeToArray(pBufferEventHeader.data(), nbytesEventHeader)) {
		LogWarningC("serilize event header fail\n");
		return false;
	}

	// 正文
	char content[sizeof(int)];
	Request::Int32Serialization(value, content);

	// 发送
	return Request::Send(ipc, pBufferCommonHeader, pBufferEventHeader, content, sizeof(int));
}


bool EventSetStringRequest::Send(IPC &ipc, message::EventHead::Type type, const char *content, int nbytes)
{
	// 扩展的事件消息头
	message::EventHead eventHeader;
	eventHeader.set_next_size(nbytes);  // 正文是char *型
	eventHeader.set_type(type);  // 设置type

	// event header size
	int32_t nbytesEventHeader = eventHeader.ByteSize();

	// 基础消息头
	message::CommonHead commonHeader;
	commonHeader.set_type(message::CommonHead_Type_Event);
	commonHeader.set_extend(true);
	commonHeader.set_next_size(nbytesEventHeader);

	// common header 序列化成 byte array
	QByteArray pBufferCommonHeader(commonHeader.ByteSize(), 0);
	if (!commonHeader.SerializeToArray(pBufferCommonHeader.data(), commonHeader.ByteSize())) {
		LogWarningC("serilize common header fail\n");
		return false;
	}

	// event header 序列化成 byte array
	QByteArray pBufferEventHeader(nbytesEventHeader, 0);
	if (!eventHeader.SerializeToArray(pBufferEventHeader.data(), nbytesEventHeader)) {
		LogWarningC("serilize event header fail\n");
		return false;
	}

	// content 无需序列化

	// 发送
	return Request::Send(ipc, pBufferCommonHeader, pBufferEventHeader, content, nbytes);
}


bool EventSetBrightnessFilterRequest::Send(IPC &ipc, int brightness)
{
	return EventSetIntRequest::Send(ipc, message::EventHead_Type_SetBrightnessFilter, brightness);
}


bool EventSetContrastFilterRequest::Send(IPC &ipc, int contrast)
{
	return EventSetIntRequest::Send(ipc, message::EventHead_Type_SetContrastFilter, contrast);
}


bool EventSetSaturationFilterRequest::Send(IPC &ipc, int saturation)
{
	return EventSetIntRequest::Send(ipc, message::EventHead_Type_SetSaturationFilter, saturation);
}


bool EventSetGammaFilterrRequest::Send(IPC &ipc, int gamma)
{
	return EventSetIntRequest::Send(ipc, message::EventHead_Type_SetGammaFilter, gamma);
}


bool EventSetSpeedRequest::Send(IPC &ipc, int speed)
{
	return EventSetIntRequest::Send(ipc, message::EventHead_Type_SetSpeed, speed);
}


bool EventTakeSnapshotRequest::Send(IPC &ipc, const char *path, int nbytes)
{
	return EventSetStringRequest::Send(ipc, message::EventHead_Type_TakeSnapshot, path, nbytes);
}


bool EventStartRecordStreamRequest::Send(IPC& ipc, const char* path, int nbytes)
{
	return EventSetStringRequest::Send(ipc, message::EventHead_Type_StartRecordStream, path, nbytes);
}


bool EventStopRecordStreamRequest::Send(IPC& ipc)
{
	return EventSimpleRequest::Send(ipc, message::EventHead_Type_StopRecordStream);
}



bool EventPauseRequest::Send(IPC &ipc)
{
	return EventSimpleRequest::Send(ipc, message::EventHead_Type_Pause);
}


bool EventResumeRequest::Send(IPC &ipc)
{
	return EventSimpleRequest::Send(ipc, message::EventHead_Type_Resume);
}


bool EventStepForwardRequest::Send(IPC &ipc)
{
	return EventSimpleRequest::Send(ipc, message::EventHead_Type_StepForward);
}



bool EventStepBackwardRequest::Send(IPC &ipc)
{
	return EventSimpleRequest::Send(ipc, message::EventHead_Type_StepBackward);
}

