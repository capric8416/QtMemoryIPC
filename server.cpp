#include <QtCore/QCoreApplication>
#include <QtCore/QSharedMemory>
#include <QtCore/QThread>

#include "ipc.h"



int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qsizetype nbytes = 10 * 1000 * 1024;
    IPC ipc = IPC("test", nbytes);
    if (!ipc.StartWriter()) {
        return -1;
    }

    ipc.WaitUntilReaderAttached();

    FILE *fp = fopen("D:\\Hedgehog_Huoche_master_2448_rev09.mp3", "rb");

    IPC::WriteError error;

    qint64 i = 1;
    while (true) {
        QByteArray content;
        content.resize(32768);
        size_t nbytes = fread(content.data(), sizeof(char), 32768, fp);
        if (nbytes > 0) {
            if (content.size() > nbytes) {
                content.resize(nbytes);
            }
            ipc.Write(content, error);

            qInfo() << QString("[%1] send content, read %2 nbytes").arg(i).arg(nbytes);
        } else {
            if (feof(fp)) {
                ipc.SetQuitChar();

                qInfo() << QString("[%1] send quit char").arg(i);

                break;
            }
        }

        i++;

        QThread::msleep(1);
    }

    ipc.StopWriter();

    return app.exec();
}
