#pragma once
#include <QWidget>
#include <QImage>
#include <QMutex>
#include <libvlc.h>
#include <libvlc_media.h>
#include <libvlc_media_player.h>

class VlcWidget_EGL : public QWidget {
    Q_OBJECT
public:
    explicit VlcWidget_EGL(const QString &url, QWidget *parent = nullptr);
    ~VlcWidget_EGL();

    void play();
    void stop();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    static void *lock_cb(void *opaque, void **planes);
    static void unlock_cb(void *opaque, void *picture, void *const *planes);
    static void display_cb(void *opaque, void *picture);

    libvlc_instance_t *m_inst = nullptr;
    libvlc_media_player_t *m_player = nullptr;
    libvlc_media_t *m_media = nullptr;

    QImage m_image;
    QMutex m_image_mutex;
    int m_width = 0;
    int m_height = 0;
    int m_stride = 0;
};
