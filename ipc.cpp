// self
#include "ipc.h"

// project
#include "../logger/logger.h"
#if defined(MPV_CLIENT)
#include "../client/common.h"
#else
#include "../server/common.h"
#endif

// qt
#include <QtCore/QThread>



IPC::IPC()
	: m_Type(IPC::Type::None)
	, m_IsSharedMemory1Locked(false)
	, m_IsSharedMemory2Locked(false)
	, m_IsSharedMemory3Locked(false)
	, m_pSharedMemory(nullptr)
	, m_pSharedMemory1(nullptr)
	, m_pSharedMemory2(nullptr)
	, m_pSharedMemory3(nullptr)
	, m_MemoryKey1("")
	, m_MemoryKey2("")
	, m_MemoryKey3("")
	, m_MaxBytes(0)
	, m_isCanceling(false)
	, m_index(0)
{
}


IPC::~IPC()
{
	if (m_Type == IPC::Type::Reader) {
		StopReader();
	}
	else if (m_Type == IPC::Type::Writer) {
		StopWriter();
	}
}


bool IPC::StartWriter(QString key, qsizetype maxBytes, quint64 index)
{
	m_MemoryKey1 = QString("%1_%2_1").arg(SharedMemoryKeyPrefix).arg(key);
	m_MemoryKey2 = QString("%1_%2_2").arg(SharedMemoryKeyPrefix).arg(key);
	m_MemoryKey3 = QString("%1_%2_3").arg(SharedMemoryKeyPrefix).arg(key);
	m_MaxBytes = maxBytes + 1;
	m_index = index;

	bool status = StartWriteShare(m_pSharedMemory1, m_MemoryKey1);
	status = StartWriteShare(m_pSharedMemory2, m_MemoryKey2);
	status = StartWriteShare(m_pSharedMemory3, m_MemoryKey3);

	memset(m_pSharedMemory3->data(), 0, 1 + sizeof(qint64) * 2);

	m_pSharedMemory = m_pSharedMemory1;

	m_Type = IPC::Type::Writer;

	m_isCanceling = false;

	return status;
}


bool IPC::StopWriter()
{
	m_Type = IPC::Type::None;
	m_isCanceling = true;

	bool status = StopAllShare(m_pSharedMemory1);
	status = StopAllShare(m_pSharedMemory2);
	status = StopAllShare(m_pSharedMemory3);

	TaskPool::GetInstance()->RemoveIntervalTask("KeepWriterAlive", false);

	return status;
}


bool IPC::StartReader(QString key, qsizetype maxBytes, quint64 index)
{
	m_MemoryKey1 = QString("%1_%2_1").arg(SharedMemoryKeyPrefix).arg(key);
	m_MemoryKey2 = QString("%1_%2_2").arg(SharedMemoryKeyPrefix).arg(key);
	m_MemoryKey3 = QString("%1_%2_3").arg(SharedMemoryKeyPrefix).arg(key);
	m_MaxBytes = maxBytes + 1;
	m_index = index;

	bool status = StartReadShare(m_pSharedMemory1, m_MemoryKey1);
	status = StartReadShare(m_pSharedMemory2, m_MemoryKey2);
	status = StartReadShare(m_pSharedMemory3, m_MemoryKey3);

	m_pSharedMemory = m_pSharedMemory1;

	m_Type = IPC::Type::Reader;

	m_isCanceling = false;

	return status;
}


bool IPC::StopReader()
{
	m_Type = IPC::Type::None;
	m_isCanceling = true;

	bool status = StopAllShare(m_pSharedMemory1);
	status = StopAllShare(m_pSharedMemory2);
	status = StopAllShare(m_pSharedMemory3);

	TaskPool::GetInstance()->RemoveIntervalTask("KeepReaderAlive", false);

	return status;
}


void IPC::Cancel()
{
	m_isCanceling = true;
}


bool IPC::IsCanceling()
{
	return m_isCanceling;
}


bool IPC::WaitUntilWriterAttached(qint32 msTimeout, bool lock)
{
	qint64 ms = 0;
	unsigned long timeout = 40;
	while (!m_isCanceling && msTimeout > 0 && !m_pSharedMemory1->attach()) {
		if (ms % 1000 == 0) {
			LogInfo() << QString("milliseconds: %1\n").arg(ms);
		}
		ms += timeout;
		msTimeout -= timeout;
		QThread::msleep(timeout);
	}

	ms = 0;
	while (!m_isCanceling && msTimeout > 0 && !m_pSharedMemory2->attach()) {
		if (ms % 1000 == 0) {
			LogInfo() << QString("milliseconds: %1\n").arg(ms);
		}
		ms += timeout;
		msTimeout -= timeout;
		QThread::msleep(timeout);
	}

	ms = 0;
	while (!m_isCanceling && msTimeout > 0 && !m_pSharedMemory3->attach()) {
		if (ms % 1000 == 0) {
			LogInfo() << QString("milliseconds: %1\n").arg(ms);
		}
		ms += timeout;
		msTimeout -= timeout;
		QThread::msleep(timeout);
	}

	bool attached = m_pSharedMemory1->isAttached() && m_pSharedMemory2->isAttached() && m_pSharedMemory3->isAttached();

	LogInfoC(attached ? "attached\n" : "cancelled\n");

	if (attached) {
		// 每300毫秒设置一次心跳
		TaskPool::GetInstance()->SubmitIntervalTask(
			"KeepReaderAlive", this, 300,
			[](void *ptr) {
				IPC *pThis = (IPC *)ptr;
				pThis->KeepReaderAlive();
				return true;
			}
		);
	}

	return attached;
}


bool IPC::WaitUntilReaderAttached(qint32 msTimeout, bool lock)
{
	qint64 ms = 0;
	unsigned long timeout = 40;
	while (!m_isCanceling && msTimeout > 0 && !IsReaderAttached(lock)) {
		if (ms % 1000 == 0) {
			LogInfo() << QString("milliseconds: %1\n").arg(ms);
		}
		ms += timeout;
		msTimeout -= timeout;
		QThread::msleep(timeout);
	}

	bool attached = IsReaderAttached(lock);

	LogInfoC(attached ? "attached\n" : "cancelled\n");

	if (!attached) {
		return false;
	}

	if (attached) {
		// 每300毫秒设置一次心跳
		TaskPool::GetInstance()->SubmitIntervalTask(
			"KeepWriterAlive", this, 300,
			[](void *ptr) {
				IPC *pThis = (IPC *)ptr;
				pThis->KeepWriterAlive();
				return true;
			}
		);
	}

	//if (!lock) {
	//	Lock(m_pSharedMemory1);
	//}
	SetCharType(m_pSharedMemory1, (char)CharType::InitForWriting, false);

	//if (!lock) {
	//	Lock(m_pSharedMemory2);
	//}
	SetCharType(m_pSharedMemory2, (char)CharType::InitForWriting, false);

	// 在当前时间戳基本上加一个10秒，防止定时器延迟造成误判
	KeepWriterAlive(10 * 1000);

	return attached;
}


bool IPC::Write(QByteArray &content, IPC::WriteError &error, bool lock)
{
	return Write(content.constData(), content.size(), error, lock);
}


bool IPC::Write(const char *buffer, qint64 nbytes, IPC::WriteError &error, bool lock)
{
	if (m_Type != IPC::Type::Writer) {
		error = IPC::WriteError::Stopped;
		return false;
	}

	if (m_isCanceling) {
		error = IPC::WriteError::Canceling;
		return false;
	}

	errno_t err = 0;
	char type = 0;
	qint64 ms = 0;
	unsigned long timeout = 4;
	while (!m_isCanceling) {
		if (lock && !Lock()) {
			error = IPC::WriteError::LockFail;

			QThread::msleep(timeout);
			ms += timeout;

			continue;
		}

		type = GetCharType(m_pSharedMemory, !lock);
		if (type == 0) {
			IncrCharType(m_pSharedMemory, !lock);

			err = memcpy_s((char *)m_pSharedMemory->data() + 1, m_MaxBytes, buffer, nbytes);

			break;
		}

		if (lock && !Unlock()) {
			error = IPC::WriteError::UnlockFail;
		}

		if (ms > 0 && ms % 1000 == 0) {
			LogInfo() << QString("wait space, milliseconds: %1\n").arg(ms);
		}

		QThread::msleep(timeout);
		ms += timeout;
	}

	if (lock && !Unlock()) {
		error = IPC::WriteError::UnlockFail;
	}

	if (err != 0) {
		error = IPC::WriteError::MemcopyFail;
		return false;
	}

	Swap();

	if (m_isCanceling) {
		error = IPC::WriteError::Canceling;
		return false;
	}

	bool status = false;
	if (type > 0) {
		error = IPC::WriteError::NoSpace;
	}
	else if (type == 0) {
		error = IPC::WriteError::NoError;
		status = true;
	}
	else {
		error = IPC::WriteError::NoReader;
	}

	return status;
}


qint32 IPC::ReadInt32(IPC::ReadError &error, bool lock)
{
	qint32 nbytes = -1;

	QByteArray buffer;
	bool status = Read(buffer, sizeof(qint32), error, lock);
	if (!status) {
		LogWarning() << QString("ipc read common header size fail, nbytes: %1, error: %2\n").arg(sizeof(qint32)).arg((qint32)error);
	}
	else {
		std::memcpy(&nbytes, buffer.constData(), sizeof(qint32));
	}

	return nbytes;
}


bool IPC::Read(QByteArray &content, qsizetype nbytes, IPC::ReadError &error, bool lock)
{
	if (m_Type != IPC::Type::Reader) {
		error = IPC::ReadError::Stopped;
		return false;
	}

	if (m_isCanceling) {
		error = IPC::ReadError::Canceling;
		return false;
	}

	char type = 0;
	qint64 ms = 0;
	unsigned long timeout = 4;
	while (!m_isCanceling) {
		if (lock && !Lock()) {
			error = IPC::ReadError::LockFail;

			QThread::msleep(timeout);
			ms += timeout;

			continue;
		}

		type = GetCharType(m_pSharedMemory, !lock);
		if (type > 0) {
			DecrCharType(m_pSharedMemory, !lock);

			content.append((const char *)m_pSharedMemory->constData() + 1, nbytes);

			break;
		}

		if (lock && !Unlock()) {
			error = IPC::ReadError::UnlockFail;
		}

		if (ms > 0 && ms % 1000 == 0) {
			LogInfo() << QString("wait space, milliseconds: %1\n").arg(ms);
		}

		QThread::msleep(timeout);
		ms += timeout;
	}

	if (lock && !Unlock()) {
		error = IPC::ReadError::UnlockFail;
	}

	Swap();

	if (m_isCanceling) {
		error = IPC::ReadError::Canceling;
		return false;
	}

	bool status = false;
	if (type > 0) {
		error = IPC::ReadError::NoError;
		status = true;
	}
	else if (type == (char)IPC::CharType::Quit) {
		error = IPC::ReadError::Quit;
	}
	else {
		error = IPC::ReadError::NoData;
	}

	return status;
}


bool IPC::StartWriteShare(QSharedMemory *&pSharedMemory, QString &key)
{
	if (IsNullPtr(pSharedMemory)) {
		pSharedMemory = new QSharedMemory();
		pSharedMemory->setKey(key);

		if (!pSharedMemory->isAttached()) {
			pSharedMemory->detach();
		}

		if (!pSharedMemory->create(pSharedMemory != m_pSharedMemory3 ? m_MaxBytes : 1 + sizeof(qint64) * 2)) {
			return false;
		}
	}

	return true;
}


bool IPC::StartReadShare(QSharedMemory *&pSharedMemory, QString &key)
{
	if (IsNullPtr(pSharedMemory)) {
		pSharedMemory = new QSharedMemory();
		pSharedMemory->setKey(key);

		if (!pSharedMemory->attach()) {
			return false;
		}
	}

	return true;
}


bool IPC::StopAllShare(QSharedMemory *&pSharedMemory)
{
	if (!IsNullPtr(pSharedMemory)) {
		pSharedMemory->detach();
		delete pSharedMemory;
		pSharedMemory = nullptr;
	}

	return true;
}


void IPC::Swap()
{
	m_pSharedMemory = m_pSharedMemory == m_pSharedMemory1 ? m_pSharedMemory2 : m_pSharedMemory1;
}


bool IPC::IsReaderAttached(bool lock)
{
	return IsReaderAttached(m_pSharedMemory1, lock) && IsReaderAttached(m_pSharedMemory2, lock) && IsReaderAttached(m_pSharedMemory3, lock);
}


bool IPC::IsWriterAlive(qint64 milliseconds)
{
	bool alive = true;

	// lock
	if (!IsNullPtr(m_pSharedMemory3) && m_pSharedMemory3 && m_pSharedMemory3->lock()) {
		// read
		qint64 ts = 0;
		std::memcpy(&ts, (char *)m_pSharedMemory3->data() + 1, sizeof(qint64));

		alive = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - ts < milliseconds;

		// unlock
		m_pSharedMemory3->unlock();
	}

	return alive;
}


bool IPC::IsReaderAlive(qint64 milliseconds)
{
	bool alive = true;

	// lock
	if (!IsNullPtr(m_pSharedMemory3) && m_pSharedMemory3 && m_pSharedMemory3->lock()) {
		// read
		qint64 ts = 0;
		std::memcpy(&ts, (char *)m_pSharedMemory3->data() + sizeof(qint64) + 1, sizeof(qint64));

		alive = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - ts < milliseconds;

		// unlock
		m_pSharedMemory3->unlock();
	}

	return alive;
}


void IPC::KeepWriterAlive(qint64 milliseconds)
{
	qint64 ts = milliseconds + std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	// lock
	if (!IsNullPtr(m_pSharedMemory3) && m_pSharedMemory3->lock()) {
		// write
		std::copy(
			static_cast<const char *>(static_cast<const void *>(&ts)),
			static_cast<const char *>(static_cast<const void *>(&ts)) + sizeof(ts),
			(char *)m_pSharedMemory3->data() + 1
		);

		// unlock
		m_pSharedMemory3->unlock();
	}
}


void IPC::KeepReaderAlive(qint64 milliseconds)
{
	qint64 ts = milliseconds + std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	// lock
	if (!IsNullPtr(m_pSharedMemory3) && m_pSharedMemory3->lock()) {
		// write
		std::copy(
			static_cast<const char *>(static_cast<const void *>(&ts)),
			static_cast<const char *>(static_cast<const void *>(&ts)) + sizeof(ts),
			(char *)m_pSharedMemory3->data() + sizeof(qint64) + 1
		);

		// unlock
		m_pSharedMemory3->unlock();
	}
}


void IPC::KeepAlive()
{
	if (m_Type == IPC::Type::Writer) {
		KeepWriterAlive();
	}
	else if (m_Type == IPC::Type::Reader) {
		KeepReaderAlive();
	}
}


bool IPC::IsReaderAttached(QSharedMemory *&pSharedMemory, bool lock)
{
	if (!IsNullPtr(pSharedMemory) && (!lock || Lock(pSharedMemory))) {
		if (GetCharType(pSharedMemory, !lock) == (char)CharType::ReaderAttach) {
			if (lock) {
				Unlock(pSharedMemory);
			}

			return true;
		}
		else {
			SetCharType(pSharedMemory, (char)CharType::WaitReaderAttached, !lock);
		}

		if (lock) {
			Unlock(pSharedMemory);
		}
	}

	return false;
}


void IPC::SetReaderAttachChar(bool lock)
{
	SetReaderAttachChar(m_pSharedMemory1, lock);
	SetReaderAttachChar(m_pSharedMemory2, lock);
	SetReaderAttachChar(m_pSharedMemory3, lock);
}


void IPC::SetReaderAttachChar(QSharedMemory *&pSharedMemory, bool lock)
{
	SetCharType(pSharedMemory, (char)CharType::ReaderAttach, lock);
}


void IPC::SetQuitChar(bool lock)
{
	SetQuitChar(m_pSharedMemory1, lock);
	SetQuitChar(m_pSharedMemory2, lock);
	SetQuitChar(m_pSharedMemory3, lock);
}


void IPC::SetQuitChar(QSharedMemory *&pSharedMemory, bool lock)
{
	SetCharType(pSharedMemory, (char)CharType::Quit, lock);
}


void IPC::SetReaderDetachChar(bool lock)
{
	SetReaderDetachChar(m_pSharedMemory1, lock);
	SetReaderDetachChar(m_pSharedMemory2, lock);
	SetReaderDetachChar(m_pSharedMemory3, lock);
}


void IPC::SetReaderDetachChar(QSharedMemory *&pSharedMemory, bool lock)
{
	SetCharType(pSharedMemory, (char)CharType::ReaderDetach, lock);
}


char IPC::IncrCharType(QSharedMemory *&pSharedMemory, bool lock)
{
	char v = 0;

	if (!IsNullPtr(pSharedMemory) && (!lock || Lock(pSharedMemory))) {
		v = ((char *)m_pSharedMemory->constData())[0] + 1;
		memset(m_pSharedMemory->data(), v, 1);

		if (lock) {
			Unlock(pSharedMemory);
		}
	}

	return v;
}


char IPC::DecrCharType(QSharedMemory *&pSharedMemory, bool lock)
{
	char v = 0;

	if (!IsNullPtr(pSharedMemory) && (!lock || Lock(pSharedMemory))) {
		v = ((char *)m_pSharedMemory->constData())[0] - 1;
		memset(m_pSharedMemory->data(), v, 1);

		if (lock) {
			Unlock(pSharedMemory);
		}
	}

	return v;
}


void IPC::SetCharType(QSharedMemory *&pSharedMemory, const char type, bool lock)
{
	if (!IsNullPtr(pSharedMemory) && (!lock || Lock(pSharedMemory))) {
		memset(pSharedMemory->data(), type, 1);

		if (lock) {
			Unlock(pSharedMemory);
		}
	}
}


char IPC::GetCharType(QSharedMemory *&pSharedMemory, bool lock)
{
	char type = 0;
	if (!IsNullPtr(pSharedMemory) && (!lock || Lock(pSharedMemory))) {
		type = ((const char *)pSharedMemory->constData())[0];

		if (lock) {
			Unlock(pSharedMemory);
		}
	}

	return type;
}


qint64 IPC::ReadHeartBeat(char *buffer)
{
	qint64 heartbeats = 0;
	std::memcpy(&heartbeats, buffer, sizeof(qint64));
	return heartbeats;
}


void IPC::WriteHeartBeat(char *buffer, qint64 value)
{
	qint64 heartbeats = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	std::copy(
		static_cast<const char *>(static_cast<const void *>(&heartbeats)),
		static_cast<const char *>(static_cast<const void *>(&heartbeats)) + sizeof(heartbeats),
		buffer
	);
}


bool IPC::Lock()
{
	return Lock(m_pSharedMemory);
}


bool IPC::Lock(QSharedMemory *&pSharedMemory)
{
	//LogInfoC("locking, pSharedMemory: %p\n", pSharedMemory);
	if (IsNullPtr(pSharedMemory) || !pSharedMemory->lock()) {
		//LogInfoC("lock fail, pSharedMemory: %p\n", pSharedMemory);
		return false;
	}
	//LogInfoC("locked, pSharedMemory: %p\n", pSharedMemory);

	SetLocked(pSharedMemory, true);

	return true;
}


bool IPC::Unlock()
{
	if (IsNullPtr(m_pSharedMemory)) {
		return true;
	}

	return Unlock(m_pSharedMemory);
}


bool IPC::Unlock(QSharedMemory *&pSharedMemory)
{
	if (IsNullPtr(m_pSharedMemory)) {
		return true;
	}

	//LogInfoC("unlocking, pSharedMemory: %p\n", pSharedMemory);
	if (!pSharedMemory->unlock()) {
		//LogInfoC("unlock fail, pSharedMemory: %p\n", pSharedMemory);
		return false;
	}
	//LogInfoC("unlocked, pSharedMemory: %p\n", pSharedMemory);

	SetLocked(pSharedMemory, false);

	return true;
}


void IPC::SetLocked(bool status)
{
	SetLocked(m_pSharedMemory, status);
}


void IPC::SetLocked(QSharedMemory *&pSharedMemory, bool status)
{
	if (pSharedMemory == m_pSharedMemory1) {
		m_IsSharedMemory1Locked = status;
	}
	else {
		m_IsSharedMemory2Locked = status;
	}
}
