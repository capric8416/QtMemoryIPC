#pragma once

// qt
#include <QtCore/QSharedMemory>





class IPC
{
public:
    // �����ڴ��ǰ׺
    const QString SharedMemoryKeyPrefix = "shared_memory_key";

    // ������
    enum class Type
    {
        // δ����
        None,
        // д���
        Writer,
        // ��ȡ��
        Reader,
    };

    // �����ڴ��ȡ����
    enum class ReadError
    {
        // �޴���
        NoError = 0,
        // ����ʧ��
        LockFail = -1,
        // ����ʧ��
        UnlockFail = -2,
        // �Զ����˳�
        Quit = -3,
        // �����ݿɹ���ȡ
        NoData = -4,
    };

    // �����ڴ�д�����
    enum class WriteError
    {
        // �޴���
        NoError = 0,
        // ����ʧ��
        LockFail = -1,
        // ����ʧ��
        UnlockFail = -2,
        // �ڴ濽������
        MemcopyFail = -3,
        // ����д�룬��Ϊ�ϴ�д������ݻ�δ����ȡ
        NoSpace = -4,
        // ��ȡ�����˳�
        NoReader = -5,
    };

    // �����ڴ����ֽ�����
    enum class CharType : char
    {
        // ֪ͨ�Զ��˳�
        Quit = -128,
        // �ȴ���ȡ������
        WaitReaderAttached = -64,
        // ��ȡ������
        ReaderAttach = -32,
        // ��ȡ������
        ReaderDetach = -16,
        // д��ǰ��λ
        InitForWriting = 0,
    };


public:
    // �����ڴ���ʹ�С��ע��ƽ̨����
    IPC(QString key, qsizetype maxBytes);
    ~IPC();

    // ����д���
    bool StartWriter();
    // ��ֹд���
    bool StopWriter();

    // ������ȡ��
    bool StartReader();
    // ��ֹ��ȡ��
    bool StopReader();

    // �ȴ���ȡ������
    void WaitUntilReaderAttached(bool lock = true);

    // д������
    bool Write(QByteArray &content, IPC::WriteError &error);
    // ��ȡ����
    bool Read(QByteArray &content, qsizetype nbytes, IPC::ReadError &error);

    // ��ȡ�����ߣ�֪ͨд���
    void SetReaderAttachChar(bool lock = true);
    // ֪ͨ�Զ��ҷ�������
    void SetQuitChar(bool lock = true);
    // ��ȡ�����ߣ�֪ͨд���
    void SetReaderDetachChar(bool lock = true);

    // ��ȡ���Ƿ�������
    bool IsReaderAttached(bool lock = true);


private:
    // ����д��˹����ڴ�
    bool StartWriteShare(QSharedMemory *&pSharedMemory, QString &key);
    // ������ȡ�˹����ڴ�
    bool StartReadShare(QSharedMemory *&pSharedMemory, QString &key);
    // ��ֹд���/��ȡ�˹����ڴ�
    bool StopAllShare(QSharedMemory *&pSharedMemory);

    // ����˫������
    void Swap();

    // ��ȡ���Ƿ�������
    bool IsReaderAttached(QSharedMemory *&pSharedMemory, bool lock = true);

    // ��ȡ�����ߣ�֪ͨд���
    void SetReaderAttachChar(QSharedMemory *&pSharedMemory, bool lock = true);
    // ֪ͨ�Զ��ҷ�������
    void SetQuitChar(QSharedMemory *&pSharedMemory, bool lock = true);
    // ��ȡ�����ߣ�֪ͨд���
    void SetReaderDetachChar(QSharedMemory *&pSharedMemory, bool lock = true);

    // д��ˣ����Ӽ���
    char IncrCharType(QSharedMemory *&pSharedMemory, bool lock = true);
    // ��ȡ�ˣ����ټ���
    char DecrCharType(QSharedMemory *&pSharedMemory, bool lock = true);
    // д�빲���ڴ����ֽ�����
    void SetCharType(QSharedMemory *&pSharedMemory, const char type, bool lock = true);
    // ��ȡ�����ڴ����ֽ�����
    char GetCharType(QSharedMemory *&pSharedMemory, bool lock = true);

    // ����
    bool Lock();
    // ����
    bool Unlock();
    // ������״̬
    void SetLocked(bool status);


    // ��Ƕ�����
    IPC::Type m_Type;

    // �����״̬
    bool m_IsSharedMemory1Locked;
    bool m_IsSharedMemory2Locked;

    // ˫����
    QSharedMemory *m_pSharedMemory;
    QSharedMemory *m_pSharedMemory1;
    QSharedMemory *m_pSharedMemory2;

    // �����ڴ��
    QString m_MemoryKey1;
    QString m_MemoryKey2;

    // �����ڴ��С
    qsizetype m_MaxBytes;
};
