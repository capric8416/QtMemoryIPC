#pragma once

// qt
#include <QtCore/QSharedMemory>



class IPC
{
public:
    // 共享内存键前缀
    const QString SharedMemoryKeyPrefix = "shared_memory_key";

    // 端类型
    enum class Type
    {
        // 未定义
        None,
        // 写入端
        Writer,
        // 读取端
        Reader,
    };

    // 共享内存读取错误
    enum class ReadError
    {
        // 无错误
        NoError = 0,
        // 加锁失败
        LockFail = -1,
        // 解锁失败
        UnlockFail = -2,
        // 对端已退出
        Quit = -3,
        // 无数据可供读取
        NoData = -4,
    };

    // 共享内存写入错误
    enum class WriteError
    {
        // 无错误
        NoError = 0,
        // 加锁失败
        LockFail = -1,
        // 解锁失败
        UnlockFail = -2,
        // 内存拷贝错误
        MemcopyFail = -3,
        // 不可写入，因为上次写入的内容还未被读取
        NoSpace = -4,
        // 读取端已退出
        NoReader = -5,
    };

    // 共享内存首字节类型
    enum class CharType : char
    {
        // 通知对端退出
        Quit = -128,
        // 等待读取者上线
        WaitReaderAttached = -64,
        // 读取端上线
        ReaderAttach = -32,
        // 读取端下线
        ReaderDetach = -16,
        // 写入前置位
        InitForWriting = 0,
    };


public:
    // 共享内存键和大小，注意平台差异
    IPC(QString key, qsizetype maxBytes);
    ~IPC();

    // 开启写入端
    bool StartWriter();
    // 终止写入端
    bool StopWriter();

    // 开启读取端
    bool StartReader();
    // 终止读取端
    bool StopReader();

    // 等待读取端上线
    void WaitUntilReaderAttached(bool lock = true);

    // 写入数据
    bool Write(QByteArray &content, IPC::WriteError &error, bool lock = true);
    // 读取数据
    bool Read(QByteArray &content, qsizetype nbytes, IPC::ReadError &error, bool lock = true);

    // 读取端上线，通知写入端
    void SetReaderAttachChar(bool lock = true);
    // 通知对端我方已下线
    void SetQuitChar(bool lock = true);
    // 读取端下线，通知写入端
    void SetReaderDetachChar(bool lock = true);

    // 读取端是否已上线
    bool IsReaderAttached(bool lock = true);


private:
    // 开启写入端共享内存
    bool StartWriteShare(QSharedMemory *&pSharedMemory, QString &key);
    // 开启读取端共享内存
    bool StartReadShare(QSharedMemory *&pSharedMemory, QString &key);
    // 终止写入端/读取端共享内存
    bool StopAllShare(QSharedMemory *&pSharedMemory);

    // 交互双缓冲区
    void Swap();

    // 读取端是否已上线
    bool IsReaderAttached(QSharedMemory *&pSharedMemory, bool lock = true);

    // 读取端上线，通知写入端
    void SetReaderAttachChar(QSharedMemory *&pSharedMemory, bool lock = true);
    // 通知对端我方已下线
    void SetQuitChar(QSharedMemory *&pSharedMemory, bool lock = true);
    // 读取端下线，通知写入端
    void SetReaderDetachChar(QSharedMemory *&pSharedMemory, bool lock = true);

    // 写入端，增加计数
    char IncrCharType(QSharedMemory *&pSharedMemory, bool lock = true);
    // 读取端，减少计数
    char DecrCharType(QSharedMemory *&pSharedMemory, bool lock = true);
    // 写入共享内存首字节类型
    void SetCharType(QSharedMemory *&pSharedMemory, const char type, bool lock = true);
    // 读取共享内存首字节类型
    char GetCharType(QSharedMemory *&pSharedMemory, bool lock = true);

    // 加锁
    bool Lock();
    // 解锁
    bool Unlock();
    // 设置锁状态
    void SetLocked(bool status);


    // 标记端类型
    IPC::Type m_Type;

    // 标记锁状态
    bool m_IsSharedMemory1Locked;
    bool m_IsSharedMemory2Locked;

    // 双缓冲
    QSharedMemory *m_pSharedMemory;
    QSharedMemory *m_pSharedMemory1;
    QSharedMemory *m_pSharedMemory2;

    // 共享内存键
    QString m_MemoryKey1;
    QString m_MemoryKey2;

    // 共享内存大小
    qsizetype m_MaxBytes;
};
