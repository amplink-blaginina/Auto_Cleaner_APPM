// cantracebuffer — буфер кадров одной линии CAN за последние 20 минут (экран «GPIO ПУ»)
// версия: GPIO ПУ 2 (RPI-RES_260928_02), 2026-09-28; Qt 5.15.8
// изменения: переписан под стиль проекта (Qt-контейнеры, QMutex), логика прежняя
#ifndef CANTRACEBUFFER_H
#define CANTRACEBUFFER_H

#include <QtGlobal>
#include <QList>
#include <QMutex>

// один принятый кадр (24 байта)
struct CanTraceFrame
{
    qint64 timeUs;      // время приема от ядра (SO_TIMESTAMP), мкс от 1970
    quint32 id;         // идентификатор без флагов
    quint8 dlc;
    quint8 flags;       // CAN_TRACE_EXT | CAN_TRACE_RTR | CAN_TRACE_TX
    quint8 data[8];
};

#define CAN_TRACE_EXT   0x01    // 29 бит
#define CAN_TRACE_RTR   0x02    // запрос
#define CAN_TRACE_TX    0x04    // кадр отправлен этим же RPi (MyCan)

// хранилище кадров: пишет поток приема, читают экран и запись .trc
// кадры нумеруются сквозным номером (seq), номер не сдвигается при удалении старых
class CanTraceBuffer
{
public:
    // retentionSec_ - сколько секунд хранить, maxFrames_ - предел по памяти
    CanTraceBuffer(int retentionSec_ = 20 * 60, quint64 maxFrames_ = 2600000);
    ~CanTraceBuffer();

    // добавить пачку кадров (поток приема)
    void append(const CanTraceFrame *frames, int count);
    // диапазон доступных кадров [first, end)
    void range(quint64 &first, quint64 &end);
    // скопировать до count кадров начиная с seq; если начало уже удалено - seq сдвигается
    int copy(quint64 &seq, int count, CanTraceFrame *dst);

private:
    // кадры храним блоками, чтобы не двигать память при удалении старых
    static const int BLOCK_SIZE = 65536;   // 1,5 МБ на блок
    QList<CanTraceFrame *> blocks;
    quint64 blockBase;  // seq первого кадра в blocks.first()
    quint64 firstSeq;
    quint64 endSeq;

    qint64 retentionUs;
    quint64 maxFrames;
    QMutex mutex;

    CanTraceFrame &at(quint64 seq);
    void trim();
};

#endif // CANTRACEBUFFER_H
