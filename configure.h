#ifndef CONFIGURE_H
#define CONFIGURE_H

#include <QObject>

class SystemConfigure
{
public:
    SystemConfigure()
    {
        configurationVersion = 0;
        clear();
    }
    void clear()
    {
        for (int i = 0; i < 9; i++)
        {
            boardsType[i] = 0;
            for (int k = 0; k < 12; k++)
            {
                channelsType[i][k] = 0;
                channelsMedianSize[i][k] = 0;
                channelsPWMSize[i][k] = 0;
                channelsLowPFM[i][k] = 0;
                channelsHighPFM[i][k] = 0;
                channelsFrameId[i][k] = 0;
                channelsByteId[i][k] = 0;
                channelsBitId[i][k] = 0;
                channelsOutFrameId[i][k] = 0;
                channelsOutByteId[i][k] = 0;
                channelsOutBitId[i][k] = 0;
                channelsValueChangeSpeed[i][k] = 0;
                channelsValueNeed[i][k] = 0;
                channelsValueCalculated[i][k] = 0;
                id[i][k] = 0;
            }
        }
    }
    void copy(SystemConfigure* dst)
    {
        for (int i = 0; i < 9; i++)
        {
            dst->boardsType[i] = boardsType[i];
            for (int k = 0; k < 12; k++)
            {
                dst->channelsType[i][k] = channelsType[i][k];
                dst->channelsMedianSize[i][k] = channelsMedianSize[i][k];
                dst->channelsPWMSize[i][k] = channelsPWMSize[i][k];
                dst->channelsLowPFM[i][k] = channelsLowPFM[i][k];
                dst->channelsHighPFM[i][k] = channelsHighPFM[i][k];
                dst->channelsFrameId[i][k] = channelsFrameId[i][k];
                dst->channelsByteId[i][k] = channelsByteId[i][k];
                dst->channelsBitId[i][k] = channelsBitId[i][k];
                dst->channelsOutFrameId[i][k] = channelsOutFrameId[i][k];
                dst->channelsOutByteId[i][k] = channelsOutByteId[i][k];
                dst->channelsOutBitId[i][k] = channelsOutBitId[i][k];
                dst->channelsValueChangeSpeed[i][k] = channelsValueChangeSpeed[i][k];
                dst->channelsValueNeed[i][k] = channelsValueNeed[i][k];
                dst->channelsValueCalculated[i][k] = channelsValueCalculated[i][k];
                dst->id[i][k] = id[i][k];
            }
        }
    }

    quint32 configurationVersion;
    quint8 boardsType[9]; // тип платы (входа, выхода)
    quint8 channelsType[9][12]; // тип канала на плате (просто выход, шим, чим или аналоговый вход)
    quint8 channelsMedianSize[9][12]; // тип медианного усреднения (только для входов)
    quint8 channelsPWMSize[9][12]; // параметры ШИМ (и для частотного шима) - какая частота
    quint8 channelsLowPFM[9][12]; // нижня частота в частотно импульсном управлении
    quint8 channelsHighPFM[9][12]; // верхняя частота в частотно импульсном управлении (определяет ширину импульса)
    quint8 channelsValueChangeSpeed[9][12]; // скорость изменгения больших значений (0-100) количество единиц в секунду
    quint8 channelsValueNeed[9][12]; // заполняется автоматически-используется для плавного изменения значения с применением channelsValueChangeSpeed
    float channelsValueCalculated[9][12]; // заполняется автоматически-используется для плавного изменения значения с применением channelsValueChangeSpeed (считает дробное накопление)
    quint8 channelsFrameId[9][12]; // заполняется автоматически
    quint8 channelsByteId[9][12]; // заполняется автоматически
    quint8 channelsBitId[9][12]; // заполняется автоматически
    quint8 channelsOutFrameId[9][12]; // заполняется автоматически
    quint8 channelsOutByteId[9][12]; // заполняется автоматически
    quint8 channelsOutBitId[9][12]; // заполняется автоматически
    int id[9][12]; // связка железа с условным элемента управления - может быть пустым (0) или имеет ссылку на устройство
};

class SystemElement
{
public:
    SystemElement(QString name_, quint8 board_, quint8 channel_)
    {
        name = name_;
        board = board_;
        channel = channel_;
    }
    SystemElement(SystemElement const *src)
    {
        name = src->name;
        board = src->board;
        channel = src->channel;
    }
    void copy(SystemElement dst)
    {
        name = dst.name;
        board = dst.board;
        channel = dst.channel;
    }
    QString name;
    quint8 board;
    quint8 channel;
};

#endif // CONFIGURE_H
