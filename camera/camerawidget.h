#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QMutex>
#include <QRectF>
#include <vector>
#include <QByteArray>

#include <gst/gst.h>
#include <gst/video/video.h>

class CameraWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit CameraWidget(int cameraCount, QWidget* parent = nullptr);
    ~CameraWidget();

    // Legacy API kept (not thread-safe) — prefer slot below
    void setFrame(int camId, GstBuffer* buffer, GstVideoInfo* info);
    void setCameraRect(int camId, const QRectF& rect);

public slots:
    // Thread-safe slot: called via queued signal from GstPlayer
    void setFrameData(int camId, const QByteArray& y, const QByteArray& u, const QByteArray& v, int w, int h);

protected:
    void initializeGL() override;
    void paintGL() override;

private:
    void uploadPlane(GLuint tex, int w, int h, const QByteArray& data);
    void drawQuad(const QRectF& rect);

private:
    struct CameraSlot {
        QByteArray y, u, v;
        int w = 0;
        int h = 0;
        QRectF rect;
        bool dirty = false;
    };

    QOpenGLShaderProgram program;
    std::vector<CameraSlot> cams;
    std::vector<GLuint> texY, texU, texV;
    QMutex mutex;
};
