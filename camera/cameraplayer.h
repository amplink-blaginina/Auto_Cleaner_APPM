#pragma once

#include <QObject>
#include <QString>
#include <gst/gst.h>
#include <gst/video/video.h>

class CameraView;

class GstPlayer : public QObject {
    Q_OBJECT
public:
    explicit GstPlayer(CameraView* widget, QObject* parent = nullptr);
    ~GstPlayer();

    void play(const QString& url);
    void stop();

    GstElement* pipeline = nullptr;

signals:
    // Emitted from GStreamer thread, connected to CameraView slot with Qt::QueuedConnection
    void frameReady(const QByteArray& y, const QByteArray& u, const QByteArray& v, int w, int h);

private:
    static GstFlowReturn onNewSample(GstElement* sink, gpointer user_data);

private:
    CameraView* cameraWidget = nullptr;
    gulong signalHandlerId = 0;
};
