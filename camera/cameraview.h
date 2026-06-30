#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QMutex>
#include <QRectF>
#include <QByteArray>

class CameraView : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit CameraView(QWidget* parent = nullptr);
    ~CameraView();

public slots:
    // Thread-safe slot: connected with Qt::QueuedConnection from GstPlayer
    void setFrameData(const QByteArray& y, const QByteArray& u, const QByteArray& v, int w, int h);

    void setViewRect(const QRectF& rect); // optional, if you want different aspect/display rects

protected:
    void initializeGL() override;
    void paintGL() override;

private:
    void uploadPlane(GLuint tex, int w, int h, const QByteArray& data);
    void drawQuad(const QRectF& rect);

    struct SlotData {
        QByteArray y, u, v;
        int w = 0, h = 0;
        QRectF rect;
        bool dirty = false;
    };

    QOpenGLShaderProgram program;
    SlotData slot;
    GLuint texY = 0, texU = 0, texV = 0;
    QMutex mutex;
};
