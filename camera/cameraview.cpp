#include "cameraview.h"
#include <QDebug>
#include <cstring>

CameraView::CameraView(QWidget* parent)
    : QOpenGLWidget(parent)
{
    slot.rect = QRectF(0,0,1,1);
}

CameraView::~CameraView() {
    makeCurrent();
    if (texY) glDeleteTextures(1, &texY);
    if (texU) glDeleteTextures(1, &texU);
    if (texV) glDeleteTextures(1, &texV);
    doneCurrent();
}

void CameraView::initializeGL() {
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

    glGenTextures(1, &texY);
    glGenTextures(1, &texU);
    glGenTextures(1, &texV);
}

void CameraView::uploadPlane(GLuint tex, int w, int h, const QByteArray& data) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, data.isEmpty() ? nullptr : reinterpret_cast<const GLvoid*>(data.constData()));
}

void CameraView::drawQuad(const QRectF& r) {
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

void CameraView::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);
    program.bind();

    QMutexLocker lock(&mutex);
    if (!slot.dirty || slot.w == 0 || slot.h == 0) {
        program.release();
        return;
    }

    uploadPlane(texY, slot.w, slot.h, slot.y);
    uploadPlane(texU, slot.w/2, slot.h/2, slot.u);
    uploadPlane(texV, slot.w/2, slot.h/2, slot.v);

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texY);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, texU);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, texV);

    program.setUniformValue("Y", 0);
    program.setUniformValue("U", 1);
    program.setUniformValue("V", 2);

    drawQuad(slot.rect);

    program.release();
}

void CameraView::setFrameData(const QByteArray& y, const QByteArray& u, const QByteArray& v, int w, int h) {
    QMutexLocker lock(&mutex);
    slot.w = w;
    slot.h = h;
    slot.y = y;
    slot.u = u;
    slot.v = v;
    slot.dirty = true;
    update();
}

void CameraView::setViewRect(const QRectF& rect) {
    QMutexLocker lock(&mutex);
    slot.rect = rect;
    update();
}
