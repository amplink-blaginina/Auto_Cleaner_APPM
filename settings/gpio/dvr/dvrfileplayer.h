// dvrfileplayer — проигрывание сегмента регистратора (.ts, H.264) в CameraView: пауза, перемотка, скорость
// версия: Регистратор 2 (RPI-RES_260929_21), 2026-09-29; RPi 4B, Raspbian 12 armhf, Qt 5.15.8, GStreamer 1.22
// изменения к версии 1 (RPI-RES_260928_51): pause/resume, seek (перемотка по опорным кадрам), setRate (x0.25..x4),
//         position/duration, стартовая позиция; события шины ASYNC_DONE для отложенной перемотки
//
// пайплайн: filesrc -> tsdemux -> h264parse -> v4l2h264dec (аппаратный, без ISP) -> appsink (sync=true)
// скорость и перемотка - обычный flush-seek с rate; appsink sync=true выдает кадры по часам конвейера
#ifndef DVRFILEPLAYER_H
#define DVRFILEPLAYER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>

class DvrFilePlayer : public QObject
{
    Q_OBJECT

public:
    explicit DvrFilePlayer(QObject *parent = nullptr);
    ~DvrFilePlayer();

    void setOutputSize(int w, int h);   // масштаб в ISP (0 - без ISP, по умолчанию), вызывать до play()
    void play(const QString &fileName, double rate = 1.0, qint64 startNs = 0); // файл с позиции startNs
    void stop();
    void pause();
    void resume();
    void seek(qint64 posNs);            // абсолютная позиция в файле, нс (встает на ближайший опорный кадр)
    void setRate(double rate);          // x0.25 ... x4, только вперед

    bool isPlaying() const { return pipeline != nullptr; }
    bool isPaused() const { return paused; }
    double rate() const { return currentRate; }
    qint64 position();                  // нс, -1 если неизвестно
    qint64 duration();                  // нс, -1 если неизвестно
    QString fileName() const { return currentFile; }

signals:
    // эмитится из потока GStreamer; подключать через Qt::QueuedConnection
    void frameReady(const QByteArray& y, const QByteArray& u, const QByteArray& v, int w, int h);
    // файл доиграл (ok=true) или ошибка (ok=false); эмитится в GUI-потоке
    void finished(bool ok);

private slots:
    void pollBus();         // 100 мс: EOS / ошибки / ASYNC_DONE

private:
    static GstFlowReturn onNewSample(GstAppSink *sink, gpointer user_data);
    void destroyPipeline();
    bool doSeek(qint64 posNs, double rate);

    GstElement *pipeline;
    QTimer busTimer;
    QString currentFile;
    int outWidth;
    int outHeight;
    bool paused;
    bool prerolled;         // конвейер прошел preroll - перемотка разрешена
    double currentRate;
    qint64 pendingSeekNs;   // перемотка, запрошенная до preroll (-1 - нет)
};

#endif // DVRFILEPLAYER_H
