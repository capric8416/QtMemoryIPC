#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

#include "ipc.h"


int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    qsizetype nbytes = 10 * 1000 * 1024;
    IPC ipc("test", nbytes);

    if (!ipc.StartReader()) {
        return -1;
    }

    ipc.SetReaderAttachChar();

    FILE *fp = fopen("D:\\test.mp3", "wb");

    IPC::ReadError error;

    qint64 i = 1;
    while (i < nbytes) {
        QByteArray content = ipc.Read(32768, error);
        if (content.isEmpty() && error == IPC::ReadError::NoData) {
            continue;
        }
        if (error == IPC::ReadError::Quit) {
            qInfo() << QString("[%1] send quit char").arg(i);

            break;
        }

        size_t nbytes = fwrite(content.data(), sizeof(char), content.size(), fp);

        qInfo() << QString("[%1] receive content, write %2 nbytes").arg(i).arg(nbytes);

        i++;

        QThread::msleep(1);
    }

    fclose(fp);

    return app.exec();
}
