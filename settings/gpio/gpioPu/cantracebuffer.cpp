// cantracebuffer — буфер кадров одной линии CAN за последние 20 минут (экран «GPIO ПУ»)
// версия: GPIO ПУ 2 (RPI-RES_260928_02), 2026-09-28
#include "cantracebuffer.h"

#include <QMutexLocker>

CanTraceBuffer::CanTraceBuffer(int retentionSec_, quint64 maxFrames_)
{
    retentionUs = qint64(retentionSec_) * 1000000;
    maxFrames = maxFrames_;
    blockBase = 0;
    firstSeq = 0;
    endSeq = 0;
}

CanTraceBuffer::~CanTraceBuffer()
{
    for (int i = 0; i < blocks.size(); i++)
        delete[] blocks[i];
}

CanTraceFrame &CanTraceBuffer::at(quint64 seq)
{
    quint64 offset = seq - blockBase;
    return blocks[int(offset / BLOCK_SIZE)][offset % BLOCK_SIZE];
}

void CanTraceBuffer::append(const CanTraceFrame *frames, int count)
{
    if (count <= 0)
        return;

    QMutexLocker locker(&mutex);
    for (int i = 0; i < count; i++)
    {
        quint64 offset = endSeq - blockBase;
        if (offset / BLOCK_SIZE >= quint64(blocks.size()))
            blocks.append(new CanTraceFrame[BLOCK_SIZE]);// нужен новый блок
        blocks[int(offset / BLOCK_SIZE)][offset % BLOCK_SIZE] = frames[i];
        endSeq++;
    }
    trim();
}

void CanTraceBuffer::trim()
{// выкидываем старые кадры: старше retentionUs или сверх maxFrames
    if (endSeq == firstSeq)
        return;

    qint64 newest = at(endSeq - 1).timeUs;
    while (firstSeq < endSeq)
    {
        bool overLimit = endSeq - firstSeq > maxFrames;
        bool tooOld = newest - at(firstSeq).timeUs > retentionUs;
        if (!overLimit && !tooOld)
            break;
        firstSeq++;
    }

    // освобождаем блоки, которые целиком ушли
    while (!blocks.isEmpty() && firstSeq - blockBase >= quint64(BLOCK_SIZE))
    {
        delete[] blocks.takeFirst();
        blockBase += BLOCK_SIZE;
    }
}

void CanTraceBuffer::range(quint64 &first, quint64 &end)
{
    QMutexLocker locker(&mutex);
    first = firstSeq;
    end = endSeq;
}

int CanTraceBuffer::copy(quint64 &seq, int count, CanTraceFrame *dst)
{
    QMutexLocker locker(&mutex);
    if (seq < firstSeq)
        seq = firstSeq;// начало уже удалено

    int copied = 0;
    while (copied < count && seq + copied < endSeq)
    {
        dst[copied] = at(seq + copied);
        copied++;
    }
    return copied;
}
