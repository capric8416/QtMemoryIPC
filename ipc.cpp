// self
#include "ipc.h"

// qt
#include <QtCore/QThread>



IPC::IPC(QString key, qsizetype maxBytes)
    : m_Type(IPC::Type::None)
    , m_IsSharedMemory1Locked(false)
    , m_IsSharedMemory2Locked(false)
    , m_pSharedMemory(nullptr)
    , m_pSharedMemory1(nullptr)
    , m_pSharedMemory2(nullptr)
    , m_MemoryKey1(QString("%1_%2_1").arg(SharedMemoryKeyPrefix).arg(key))
    , m_MemoryKey2(QString("%1_%2_2").arg(SharedMemoryKeyPrefix).arg(key))
    , m_MaxBytes(maxBytes)
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


bool IPC::StartWriter()
{
    bool status = StartWriteShare(m_pSharedMemory1, m_MemoryKey1) && StartWriteShare(m_pSharedMemory2, m_MemoryKey2);

    m_pSharedMemory = m_pSharedMemory1;

    m_Type = IPC::Type::Writer;

    return status;
}


bool IPC::StopWriter()
{
    m_Type = IPC::Type::None;
    return StopAllShare(m_pSharedMemory1) && StopAllShare(m_pSharedMemory2);
}


bool IPC::StartReader()
{
    bool status = StartReadShare(m_pSharedMemory1, m_MemoryKey1) && StartReadShare(m_pSharedMemory2, m_MemoryKey2);

    m_pSharedMemory = m_pSharedMemory1;

    m_Type = IPC::Type::Reader;

    return status;
}


bool IPC::StopReader()
{
    m_Type = IPC::Type::None;
    return StopAllShare(m_pSharedMemory1) && StopAllShare(m_pSharedMemory2);
}


void IPC::WaitUntilReaderAttached(bool lock)
{
    while (!IsReaderAttached(lock)) {
        QThread::msleep(33);
    }

    if (!lock) {
        m_pSharedMemory1->lock();
    }
    SetCharType(m_pSharedMemory1, (char)CharType::InitForWriting, false);

    if (!lock) {
        m_pSharedMemory2->lock();
    }
    SetCharType(m_pSharedMemory2, (char)CharType::InitForWriting, false);
}


bool IPC::Write(QByteArray &content, IPC::WriteError &error, bool lock)
{
    if (lock && !Lock()) {
        error = IPC::WriteError::LockFail;
        return false;
    }

    errno_t err = 0;
    char type = GetCharType(m_pSharedMemory, !lock);
    if (type == 0) {
        IncrCharType(m_pSharedMemory, !lock);

        err = memcpy_s((char *)m_pSharedMemory->data() + 1, m_MaxBytes, content.constData(), content.size());
    }

    if (lock && !Unlock()) {
        error = IPC::WriteError::UnlockFail;
    }

    if (err != 0) {
        error = IPC::WriteError::MemcopyFail;
        return false;
    }

    Swap();

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


bool IPC::Read(QByteArray &content, qsizetype nbytes, IPC::ReadError &error, bool lock)
{
    if (lock && !Lock()) {
        error = IPC::ReadError::LockFail;
        return false;
    }

    char type = GetCharType(m_pSharedMemory, !lock);
    if (type > 0) {
        DecrCharType(m_pSharedMemory, !lock);

        content.append((const char *)m_pSharedMemory->constData() + 1, nbytes);
    }

    if (lock && !Unlock()) {
        error = IPC::ReadError::UnlockFail;
    }

    Swap();

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
    if (!pSharedMemory) {
        pSharedMemory = new QSharedMemory();
        pSharedMemory->setKey(key);

        if (!pSharedMemory->isAttached()) {
            pSharedMemory->detach();
        }

        if (!pSharedMemory->create(m_MaxBytes)) {
            return false;
        }
    }

    return true;
}


bool IPC::StartReadShare(QSharedMemory *&pSharedMemory, QString &key)
{
    if (!pSharedMemory) {
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
    if (!pSharedMemory) {
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
    return IsReaderAttached(m_pSharedMemory1, lock) && IsReaderAttached(m_pSharedMemory2, lock);
}


bool IPC::IsReaderAttached(QSharedMemory *&pSharedMemory, bool lock)
{
    if (!lock || pSharedMemory->lock()) {
        if (GetCharType(pSharedMemory, !lock) == (char)CharType::ReaderAttach) {
            return true;
        }
        else {
            SetCharType(pSharedMemory, (char)CharType::WaitReaderAttached, !lock);
        }

        if (lock) {
            pSharedMemory->unlock();
        }
    }

    return false;
}


void IPC::SetReaderAttachChar(bool lock)
{
    SetReaderAttachChar(m_pSharedMemory1, lock);
    SetReaderAttachChar(m_pSharedMemory2, lock);
}


void IPC::SetReaderAttachChar(QSharedMemory *&pSharedMemory, bool lock)
{
    SetCharType(pSharedMemory, (char)CharType::ReaderAttach, lock);
}


void IPC::SetQuitChar(bool lock)
{
    SetQuitChar(m_pSharedMemory1, lock);
    SetQuitChar(m_pSharedMemory2, lock);
}


void IPC::SetQuitChar(QSharedMemory *&pSharedMemory, bool lock)
{
    SetCharType(pSharedMemory, (char)CharType::Quit, lock);
}


void IPC::SetReaderDetachChar(bool lock)
{
    SetReaderDetachChar(m_pSharedMemory1, lock);
    SetReaderDetachChar(m_pSharedMemory2, lock);
}


void IPC::SetReaderDetachChar(QSharedMemory *&pSharedMemory, bool lock)
{
    SetCharType(pSharedMemory, (char)CharType::ReaderDetach, lock);
}


char IPC::IncrCharType(QSharedMemory *&pSharedMemory, bool lock)
{
    char v = 0;

    if (!lock || pSharedMemory->lock()) {
        v = ((char *)m_pSharedMemory->constData())[0] + 1;
        memset(m_pSharedMemory->data(), v, 1);

        if (lock) {
            pSharedMemory->unlock();
        }
    }

    return v;
}


char IPC::DecrCharType(QSharedMemory *&pSharedMemory, bool lock)
{
    char v = 0;

    if (!lock || pSharedMemory->lock()) {
        v = ((char *)m_pSharedMemory->constData())[0] - 1;
        memset(m_pSharedMemory->data(), v, 1);

        if (lock) {
            pSharedMemory->unlock();
        }
    }

    return v;
}


void IPC::SetCharType(QSharedMemory *&pSharedMemory, const char type, bool lock)
{
    if (!lock || pSharedMemory->lock()) {
        memset(pSharedMemory->data(), type, 1);

        if (lock) {
            pSharedMemory->unlock();
        }
    }
}


char IPC::GetCharType(QSharedMemory *&pSharedMemory, bool lock)
{
    char type = 0;
    if (!lock || pSharedMemory->lock()) {
        type = ((const char *)pSharedMemory->constData())[0];

        if (lock) {
            pSharedMemory->unlock();
        }
    }

    return type;
}


bool IPC::Lock()
{
    if (!m_pSharedMemory->lock()) {
        return false;
    }

    SetLocked(true);

    return true;
}


bool IPC::Unlock()
{
    bool status = m_pSharedMemory->unlock();

    SetLocked(false);

    return true;
}


void IPC::SetLocked(bool status)
{
    if (m_pSharedMemory == m_pSharedMemory1) {
        m_IsSharedMemory1Locked = status;
    }
    else {
        m_IsSharedMemory2Locked = status;
    }
}
