#include "camerawidget.h"
#include <gst/video/video.h>
#include <QDebug>
#include <cstring>

CameraWidget::CameraWidget(int cameraCount, QWidget* parent)
    : QOpenGLWidget(parent),
      cams(cameraCount),
      texY(cameraCount),
      texU(cameraCount),
      texV(cameraCount)
{
    for (int i = 0; i < cameraCount; ++i) {
        cams[i].rect = QRectF(
            (i % 2) * 0.5f,
            (i / 2) * 0.5f,
            0.5f,
            0.5f
        );
    }
}

CameraWidget::~CameraWidget() {
    makeCurrent();
    if (!texY.empty()) glDeleteTextures(static_cast<GLsizei>(texY.size()), texY.data());
    if (!texU.empty()) glDeleteTextures(static_cast<GLsizei>(texU.size()), texU.data());
    if (!texV.empty()) glDeleteTextures(static_cast<GLsizei>(texV.size()), texV.data());
    doneCurrent();
}

void CameraWidget::initializeGL() {
    initializeOpenGLFunctions();

    program.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "attribute vec2 vertexIn;"
        "attribute vec2 texIn;"
        "varying vec2 tex;"
        "void main(){"
        " gl_Position=vec4(vertexIn,0,1);"
        " tex=texIn;"
        "}"
    );

    program.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "precision mediump float;"
        "varying vec2 tex;"
        "uniform sampler2D Y;"
        "uniform sampler2D U;"
        "uniform sampler2D V;"
        "void main(){"
        " float y=texture2D(Y,tex).r;"
        " float u=texture2D(U,tex).r-0.5;"
        " float v=texture2D(V,tex).r-0.5;"
        " gl_FragColor=vec4("
        " y+1.402*v,"
        " y-0.344*u-0.714*v,"
        " y+1.772*u,"
        " 1.0);"
        "}"
    );

    program.link();

    glGenTextures(static_cast<GLsizei>(texY.size()), texY.data());
    glGenTextures(static_cast<GLsizei>(texU.size()), texU.data());
    glGenTextures(static_cast<GLsizei>(texV.size()), texV.data());
}

void CameraWidget::uploadPlane(GLuint tex, int w, int h, const QByteArray& data) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // GL_LUMINANCE usually works on Raspberry Pi EGLFS; if using newer GL, may need GL_RED and shader adjust.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, data.isEmpty() ? nullptr : reinterpret_cast<const GLvoid*>(data.constData()));
}

void CameraWidget::drawQuad(const QRectF& r) {
    GLfloat v[] = {
        float(r.left()*2-1),  float(1-r.top()*2),
        float(r.right()*2-1), float(1-r.top()*2),
        float(r.left()*2-1),  float(1-r.bottom()*2),
        float(r.right()*2-1), float(1-r.bottom()*2)
    };

    GLfloat t[] = {0,0, 1,0, 0,1, 1,1};

    program.enableAttributeArray("vertexIn");
    program.enableAttributeArray("texIn");
    program.setAttributeArray("vertexIn", GL_FLOAT, v, 2);
    program.setAttributeArray("texIn", GL_FLOAT, t, 2);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void CameraWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);
    program.bind();

    QMutexLocker lock(&mutex);

    for (size_t i = 0; i < cams.size(); ++i) {
        const auto& c = cams[i];
        if (!c.dirty || c.w == 0 || c.h == 0) continue;

        uploadPlane(texY[i], c.w, c.h, c.y);
        uploadPlane(texU[i], c.w / 2, c.h / 2, c.u);
        uploadPlane(texV[i], c.w / 2, c.h / 2, c.v);

        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texY[i]);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, texU[i]);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, texV[i]);

        program.setUniformValue("Y", 0);
        program.setUniformValue("U", 1);
        program.setUniformValue("V", 2);

        drawQuad(c.rect);
    }

    program.release();
}

void CameraWidget::setFrame(int camId, GstBuffer* buffer, GstVideoInfo* info) {
    // Legacy: keep but don't call this from non-GUI thread.
    if (camId < 0 || camId >= (int)cams.size()) return;
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) return;

    QMutexLocker lock(&mutex);
    auto& c = cams[camId];

    c.w = GST_VIDEO_INFO_WIDTH(info);
    c.h = GST_VIDEO_INFO_HEIGHT(info);

    int ySize = c.w * c.h;
    int uvSize = ySize / 4;

    c.y = QByteArray(reinterpret_cast<const char*>(map.data), ySize);
    c.u = QByteArray(reinterpret_cast<const char*>(map.data + ySize), uvSize);
    c.v = QByteArray(reinterpret_cast<const char*>(map.data + ySize + uvSize), uvSize);
    c.dirty = true;

    gst_buffer_unmap(buffer, &map);
    update();
}

void CameraWidget::setFrameData(int camId, const QByteArray& y, const QByteArray& u, const QByteArray& v, int w, int h) {
    // This is the thread-safe entry point (runs in GUI thread because of queued connection)
    if (camId < 0 || camId >= (int)cams.size()) return;

    QMutexLocker lock(&mutex);
    auto& c = cams[camId];

    c.w = w;
    c.h = h;

    c.y = y;
    c.u = u;
    c.v = v;
    c.dirty = true;

    // Request repaint on GUI thread
    update();
}

void CameraWidget::setCameraRect(int camId, const QRectF& rect) {
    if (camId < 0 || camId >= (int)cams.size()) return;
    QMutexLocker lock(&mutex);
    cams[camId].rect = rect;
}
