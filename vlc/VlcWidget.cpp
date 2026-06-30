#include "VlcWidget.hpp"
#include <QPainter>
#include <cstring>

VlcWidget_EGL::VlcWidget_EGL(const QString &url, QWidget *parent)
    : QWidget(parent)
{
    m_inst = libvlc_new(0, nullptr);
    m_media = libvlc_media_new_location(m_inst, url.toUtf8().constData());
    m_player = libvlc_media_player_new_from_media(m_media);
    libvlc_media_release(m_media);

    // Установим формат: RGB24 (3 bytes per pixel)
    // Подставь желаемое разрешение, лучше опрашивать из потока, но для простоты — фиксируем
    m_width = 640;
    m_height = 360;
    m_stride = m_width * 3;

    libvlc_video_set_format(m_player, "RV32", m_width, m_height, m_stride);
    // "RV32" = 32-bit little-endian (B G R X), можно попробовать "RGB24" в зависимости от VLC build

    libvlc_video_set_callbacks(m_player,
        &VlcWidget_EGL::lock_cb,
        &VlcWidget_EGL::unlock_cb,
        &VlcWidget_EGL::display_cb,
        this);

    libvlc_media_player_play(m_player);
}

VlcWidget_EGL::~VlcWidget_EGL()
{
    if (m_player) {
        libvlc_media_player_stop(m_player);
        libvlc_media_player_release(m_player);
    }
    if (m_inst) libvlc_release(m_inst);
}

void *VlcWidget_EGL::lock_cb(void *opaque, void **planes)
{
    VlcWidget_EGL *self = static_cast<VlcWidget_EGL*>(opaque);
    self->m_image_mutex.lock();
    if (self->m_image.size() != QSize(self->m_width, self->m_height))
        self->m_image = QImage(self->m_width, self->m_height, QImage::Format_RGB32);
    *planes = self->m_image.bits();
    return *planes;
}

void VlcWidget_EGL::unlock_cb(void *opaque, void *picture, void *const *planes)
{
    VlcWidget_EGL *self = static_cast<VlcWidget_EGL*>(opaque);
    // nothing here for now
    self->m_image_mutex.unlock();
}

void VlcWidget_EGL::display_cb(void *opaque, void *picture)
{
    VlcWidget_EGL *self = static_cast<VlcWidget_EGL*>(opaque);
    // schedule repaint on GUI thread
    QMetaObject::invokeMethod(self, "update", Qt::QueuedConnection);
}

void VlcWidget_EGL::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    QMutexLocker locker(&m_image_mutex);
    if (!m_image.isNull()) {
        QImage img = m_image.scaled(size(), Qt::KeepAspectRatio);
        p.drawImage(QPoint(0,0), img);
    }
}
