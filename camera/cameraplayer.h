/*
 * Назначение : GstPlayer — приём RTSP H.264 с IP-камеры, аппаратное декодирование (v4l2h264dec),
 *              выдача кадров I420 в CameraView; автопереподключение при ошибке/обрыве потока.
 * Версия     : 07 (RPI-RES_260928_27, экран «Регистратор»)
 * Дата       : 2026-09-25
 * Платформа  : Raspberry Pi 4 Model B Rev 1.5, Raspbian 12 bookworm armhf,
 *              Qt 5 (Debian bookworm), GStreamer 1.22.0 (app, video, v4l2, debug/capssetter)
 * Файл в проекте: ~/work/ttt/camera/cameraplayer.h
 * Изменения v07: setOutputSize(w, h) — масштаб кадра в ISP (v4l2convert, dmabuf) до копирования в память;
 *               по замерам RPI-RES_260928_17 снижает нагрузку экрана камер почти вдвое.
 * Изменения v06 относительно исходного cameraplayer.h (2025-12-25):
 *  - публичный API не изменён: GstPlayer(CameraView*, QObject*), play(url), stop(), pipeline, frameReady(...)
 *  - appsink через gst_app_sink_set_callbacks вместо GObject-сигнала "new-sample"
 *    (убирает предупреждения gsignal.c "no handler with id")
 *  - контроль шины GStreamer (ERROR/EOS/WARNING) и сторожевой таймер кадров,
 *    переподключение с нарастающей задержкой 2..30 с
 *  - новый сигнал streamStateChanged(bool) для индикации в UI (подключать не обязательно)
 */
#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <atomic>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>

class CameraView;

class GstPlayer : public QObject {
    Q_OBJECT
public:
    explicit GstPlayer(CameraView* widget, QObject* parent = nullptr);
    ~GstPlayer();

    void play(const QString& url);   // запуск и удержание потока (с автопереподключением)
    void stop();                     // полная остановка, переподключение отключается
    void setOutputSize(int w, int h);// размер кадра после ISP (0 - без масштаба), вызывать до play()

    GstElement* pipeline = nullptr;

signals:
    // Эмитится из потока GStreamer; подключать к CameraView::setFrameData через Qt::QueuedConnection
    void frameReady(const QByteArray& y, const QByteArray& u, const QByteArray& v, int w, int h);
    // true — кадры идут, false — поток потерян/ошибка (эмитится в GUI-потоке при смене состояния)
    void streamStateChanged(bool ok);

private slots:
    void pollBus();        // разбор сообщений шины + сторожевой таймер кадров
    void reconnect();      // повторный запуск пайплайна

private:
    static GstFlowReturn onNewSample(GstAppSink* sink, gpointer user_data);
    bool startPipeline();
    void destroyPipeline();
    void scheduleReconnect(const char* reason);
    void setStreamOk(bool ok);
    QString safeUrl() const;   // URL без пароля для логов

private:
    CameraView* cameraWidget = nullptr;
    int outWidth = 0;       // 0 - кадр как с камеры
    int outHeight = 0;
    QString currentUrl;
    bool wantPlaying = false;
    bool streamOk = false;

    QTimer busTimer;        // 200 мс: шина + сторожевой таймер
    QTimer reconnectTimer;  // single-shot
    int reconnectDelayMs = 2000;
    qint64 startedAtMs = 0;

    std::atomic<qint64> lastFrameMs{0};

    static constexpr int kBusPollMs        = 200;
    static constexpr int kFirstFrameTimeMs = 10000; // ожидание первого кадра после старта
    static constexpr int kFrameTimeoutMs   = 5000;  // нет кадров дольше — переподключение
    static constexpr int kReconnectMinMs   = 2000;
    static constexpr int kReconnectMaxMs   = 30000;
};
