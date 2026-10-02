// trcwriter — запись буферов CAN в файлы PCAN .trc v1.1 (кнопка «↑CAN» экрана «GPIO ПУ»)
// версия: GPIO ПУ 2 (RPI-RES_260928_02), 2026-09-28; Qt 5.15.8
// изменения: переписан по образцу MyCan (QObject в своем QThread, результат - сигналом)
#ifndef TRCWRITER_H
#define TRCWRITER_H

#include <QObject>
#include <QThread>
#include <QList>
#include <QSharedPointer>

#include "cantracebuffer.h"

// одноразовый объект: создали - он в своем треде пишет все файлы,
// шлет finished() и сам удаляется вместе с тредом
class TrcWriter : public QObject
{
    Q_OBJECT
public:
    // что писать: буфер одной линии -> файл
    struct Job
    {
        QSharedPointer<CanTraceBuffer> buffer;
        QString fileName;
        QString canName;
        quint32 bitrate;
    };

    explicit TrcWriter(QList<Job> jobs_, QObject *parent = nullptr);

    // тред для отделения в отдельный поток
    QThread *mThread;

public slots:
    void writeAll();

signals:
    // ok - все файлы записаны; message - что показать на экране
    void finished(bool ok, QString message);

private:
    bool writeFile(const Job &job, quint64 &frames, QString &error);
    QByteArray header(qint64 startUs, const Job &job);

    QList<Job> jobs;
};

#endif // TRCWRITER_H
