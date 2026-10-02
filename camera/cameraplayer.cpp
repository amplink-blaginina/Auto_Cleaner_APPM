/*
 * Назначение : GstPlayer — приём RTSP H.264 с IP-камеры, аппаратное декодирование (v4l2h264dec),
 *              выдача кадров I420 в CameraView; автопереподключение при ошибке/обрыве потока.
 * Версия     : 07 (RPI-RES_260928_27): setOutputSize() - масштаб кадра в ISP (v4l2convert, dmabuf)
 * Дата       : 2026-09-25
 * Платформа  : Raspberry Pi 4 Model B Rev 1.5, Raspbian 12 bookworm armhf,
 *              Qt 5 (Debian bookworm), GStreamer 1.22.0 (app, video, v4l2, debug/capssetter)
 * Файл в проекте: ~/work/ttt/camera/cameraplayer.cpp (пара к RPI-RES_260925_06_cameraplayer.h)
 * Изменения относительно исходного cameraplayer.cpp (2025-12-17):
 *  1. Пайплайн: protocols=tcp, tcp-timeout=5 с; capssetter colorimetry=bt601 после h264parse —
 *     камера .27 передаёт в VUI colorimetry 1:4:5:3 (full range), v4l2h264dec 1.22 не согласует
 *     такие caps (not-negotiated). См. RPI-RES_260925_05.
 *  2. appsink: gst_app_sink_set_callbacks вместо сигнала "new-sample" (нет g_signal_connect/disconnect).
 *  3. Копирование плоскостей через GstVideoFrame с учётом stride (раньше — предположение о плотной упаковке).
 *  4. Шина GStreamer опрашивается QTimer'ом (200 мс) в GUI-потоке: ERROR/EOS → переподключение,
 *     WARNING → лог. Сторожевой таймер: нет первого кадра 10 с / нет кадров 5 с → переподключение.
 *     Задержка переподключения 2 → 4 → 8 → 16 → 30 с, сброс после получения кадра.
 *  5. В конструкторе больше НЕ выполняется connect(frameReady → setFrameData): mainwindow.cpp уже
 *     делает этот connect, и каждый кадр доставлялся дважды (двойное копирование и загрузка текстуры).
 *  6. Пароль из URL в логи не пишется. Время — монотонное (g_get_monotonic_time), не зависит от скачков часов.
 */
#include "cameraplayer.h"
#include "cameraview.h"

#include <gst/app/gstappsink.h>
#include <QDebug>
#include <QRegularExpression>

static inline qint64 monoMs() { return g_get_monotonic_time() / 1000; }

GstPlayer::GstPlayer(CameraView* widget, QObject* parent)
    : QObject(parent),
      cameraWidget(widget)
{
    gst_init(nullptr, nullptr);

    // frameReady → CameraView::setFrameData подключается в mainwindow.cpp (Qt::QueuedConnection).

    busTimer.setInterval(kBusPollMs);
    connect(&busTimer, &QTimer::timeout, this, &GstPlayer::pollBus);

    reconnectTimer.setSingleShot(true);
    connect(&reconnectTimer, &QTimer::timeout, this, &GstPlayer::reconnect);
}

GstPlayer::~GstPlayer() {
    stop();
}

QString GstPlayer::safeUrl() const {
    QString s = currentUrl;
    s.replace(QRegularExpression("//[^/@]*@"), "//***@");
    return s;
}

void GstPlayer::play(const QString& url) {
    stop();
    currentUrl = url;
    wantPlaying = true;
    reconnectDelayMs = kReconnectMinMs;
    if (!startPipeline())
        scheduleReconnect("start failed");
    busTimer.start();
}

void GstPlayer::stop() {
    wantPlaying = false;
    reconnectTimer.stop();
    busTimer.stop();
    destroyPipeline();
    setStreamOk(false);
}

void GstPlayer::setOutputSize(int w, int h) {
    outWidth = w;
    outHeight = h;
}

bool GstPlayer::startPipeline() {
    destroyPipeline();

    const QString pipelineStr = QString(
        "rtspsrc location=%1 protocols=tcp tcp-timeout=5000000 latency=200 ! "
        "rtph264depay ! h264parse ! "
        "capssetter caps=\"video/x-h264,colorimetry=(string)bt601\" ! "
        "%2"
        "appsink name=sink sync=false max-buffers=1 drop=true"
    ).arg(currentUrl).arg(outWidth > 0 && outHeight > 0
        ? QString("v4l2h264dec capture-io-mode=dmabuf ! v4l2convert output-io-mode=dmabuf-import ! "
                  "video/x-raw,format=I420,width=%1,height=%2 ! ").arg(outWidth).arg(outHeight)
        : QString("v4l2h264dec ! video/x-raw,format=I420 ! "));

    GError* err = nullptr;
    GstElement* p = gst_parse_launch(pipelineStr.toUtf8().constData(), &err);
    if (err) {
        if (p) qWarning() << "GstPlayer:" << safeUrl() << "pipeline warning:" << err->message;
        else   qCritical() << "GstPlayer:" << safeUrl() << "pipeline error:" << err->message;
        g_error_free(err);
    }
    if (!p) return false;

    GstElement* sink = gst_bin_get_by_name(GST_BIN(p), "sink");
    if (!sink) {
        qCritical() << "GstPlayer: appsink not found in pipeline";
        gst_object_unref(p);
        return false;
    }

    GstAppSinkCallbacks cb = {};
    cb.new_sample = &GstPlayer::onNewSample;
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &cb, this, nullptr);
    gst_app_sink_set_emit_signals(GST_APP_SINK(sink), FALSE);
    gst_app_sink_set_max_buffers(GST_APP_SINK(sink), 1);
    gst_app_sink_set_drop(GST_APP_SINK(sink), TRUE);
    gst_object_unref(sink);

    pipeline = p;
    lastFrameMs.store(0);
    startedAtMs = monoMs();

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        qWarning() << "GstPlayer:" << safeUrl() << "set_state(PLAYING) failed";
        destroyPipeline();
        return false;
    }
    qInfo() << "GstPlayer:" << safeUrl() << "started";
    return true;
}

void GstPlayer::destroyPipeline() {
    if (!pipeline) return;
    GstElement* p = pipeline;
    pipeline = nullptr;
    // После перехода в NULL потоки стриминга остановлены, колбэки appsink больше не вызываются
    gst_element_set_state(p, GST_STATE_NULL);
    gst_object_unref(p);
}

void GstPlayer::scheduleReconnect(const char* reason) {
    if (!wantPlaying) return;
    destroyPipeline();
    setStreamOk(false);
    if (reconnectTimer.isActive()) return;
    qWarning() << "GstPlayer:" << safeUrl() << "lost:" << reason
               << "- reconnect in" << reconnectDelayMs << "ms";
    reconnectTimer.start(reconnectDelayMs);
    reconnectDelayMs = qMin(reconnectDelayMs * 2, kReconnectMaxMs);
}

void GstPlayer::reconnect() {
    if (!wantPlaying) return;
    if (!startPipeline())
        scheduleReconnect("restart failed");
}

void GstPlayer::setStreamOk(bool ok) {
    if (streamOk == ok) return;
    streamOk = ok;
    emit streamStateChanged(ok);
}

void GstPlayer::pollBus() {
    if (!wantPlaying || !pipeline) return;

    GstBus* bus = gst_element_get_bus(pipeline);
    if (bus) {
        GstMessage* msg;
        while ((msg = gst_bus_pop_filtered(bus, GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS |
                                                               GST_MESSAGE_WARNING))) != nullptr) {
            const GstMessageType t = GST_MESSAGE_TYPE(msg);
            if (t == GST_MESSAGE_ERROR || t == GST_MESSAGE_WARNING) {
                GError* e = nullptr;
                gchar* dbg = nullptr;
                if (t == GST_MESSAGE_ERROR) gst_message_parse_error(msg, &e, &dbg);
                else                        gst_message_parse_warning(msg, &e, &dbg);
                const QString src = GST_OBJECT_NAME(GST_MESSAGE_SRC(msg));
                if (t == GST_MESSAGE_ERROR)
                    qWarning() << "GstPlayer:" << safeUrl() << "ERROR from" << src << ":" << (e ? e->message : "?")
                               << "|" << (dbg ? dbg : "");
                else
                    qInfo() << "GstPlayer:" << safeUrl() << "WARNING from" << src << ":" << (e ? e->message : "?");
                if (e) g_error_free(e);
                g_free(dbg);
                if (t == GST_MESSAGE_ERROR) {
                    gst_message_unref(msg);
                    gst_object_unref(bus);
                    scheduleReconnect("bus error");
                    return;
                }
            } else if (t == GST_MESSAGE_EOS) {
                gst_message_unref(msg);
                gst_object_unref(bus);
                scheduleReconnect("EOS");
                return;
            }
            gst_message_unref(msg);
        }
        gst_object_unref(bus);
    }

    // Сторожевой таймер кадров
    const qint64 now = monoMs();
    const qint64 last = lastFrameMs.load();
    if (last == 0) {
        if (now - startedAtMs > kFirstFrameTimeMs)
            scheduleReconnect("no first frame");
    } else if (now - last > kFrameTimeoutMs) {
        scheduleReconnect("frame timeout");
    } else {
        reconnectDelayMs = kReconnectMinMs;
        setStreamOk(true);
    }
}

GstFlowReturn GstPlayer::onNewSample(GstAppSink* sink, gpointer user_data) {
    GstPlayer* self = static_cast<GstPlayer*>(user_data);

    GstSample* sample = gst_app_sink_pull_sample(sink);
    if (!sample) return GST_FLOW_OK;

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstCaps* caps = gst_sample_get_caps(sample);
    GstVideoInfo info;
    if (!buffer || !caps || !gst_video_info_from_caps(&info, caps) ||
        GST_VIDEO_INFO_FORMAT(&info) != GST_VIDEO_FORMAT_I420) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    GstVideoFrame frame;
    if (!gst_video_frame_map(&frame, &info, buffer, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    const int w = GST_VIDEO_FRAME_WIDTH(&frame);
    const int h = GST_VIDEO_FRAME_HEIGHT(&frame);

    // Плотная упаковка плоскостей (формат, который ожидает CameraView), с учётом stride источника
    auto copyPlane = [&frame](int plane, int cw, int ch) {
        QByteArray out(cw * ch, Qt::Uninitialized);
        const guint8* src = static_cast<const guint8*>(GST_VIDEO_FRAME_PLANE_DATA(&frame, plane));
        const int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, plane);
        char* dst = out.data();
        for (int r = 0; r < ch; ++r)
            memcpy(dst + r * cw, src + r * stride, cw);
        return out;
    };

    const int cw = GST_VIDEO_FRAME_COMP_WIDTH(&frame, 1);
    const int ch = GST_VIDEO_FRAME_COMP_HEIGHT(&frame, 1);
    QByteArray yData = copyPlane(0, w, h);
    QByteArray uData = copyPlane(1, cw, ch);
    QByteArray vData = copyPlane(2, cw, ch);

    gst_video_frame_unmap(&frame);
    gst_sample_unref(sample);

    self->lastFrameMs.store(monoMs());
    emit self->frameReady(yData, uData, vData, w, h);
    return GST_FLOW_OK;
}
