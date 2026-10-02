// dvrfileplayer — проигрывание сегмента регистратора (.ts, H.264) в CameraView: пауза, перемотка, скорость
// версия: Регистратор 2 (RPI-RES_260929_21), 2026-09-29
#include "dvrfileplayer.h"

#include <QFileInfo>
#include <QDebug>
#include <cstring>

DvrFilePlayer::DvrFilePlayer(QObject *parent) :
    QObject(parent)
{
    pipeline = nullptr;
    outWidth = 0;   // по умолчанию без ISP: см. комментарий в play()
    outHeight = 0;
    paused = false;
    prerolled = false;
    currentRate = 1.0;
    pendingSeekNs = -1;

    if (!gst_is_initialized())
        gst_init(nullptr, nullptr);

    connect(&busTimer, &QTimer::timeout, this, &DvrFilePlayer::pollBus);
}

DvrFilePlayer::~DvrFilePlayer()
{
    stop();
}

void DvrFilePlayer::setOutputSize(int w, int h)
{
    outWidth = w;
    outHeight = h;
}

void DvrFilePlayer::play(const QString &fileName, double rate, qint64 startNs)
{
    stop();
    currentFile = fileName;
    currentRate = rate;
    paused = false;
    prerolled = false;
    // перемотку и скорость можно выставить только после preroll - запоминаем до ASYNC_DONE
    pendingSeekNs = (startNs > 0 || rate != 1.0) ? qMax<qint64>(startNs, 0) : -1;

    // масштаб в ISP (v4l2convert) - только если задан setOutputSize. На экране регистратора ISP не используем:
    // при 5 декодерах + 5 ISP одновременно (экран, плеер и отдельный тест) на стенде зависла прошивка VideoCore
    // (bcm2835-codec "Mutex fail", firmware timeout, 28.09.2026) - поэтому файл декодируется без ISP
    const QString scale = (outWidth > 0 && outHeight > 0)
        ? QString("v4l2h264dec capture-io-mode=dmabuf ! v4l2convert output-io-mode=dmabuf-import ! "
                  "video/x-raw,format=I420,width=%1,height=%2 ! ").arg(outWidth).arg(outHeight)
        : QString("v4l2h264dec ! video/x-raw,format=I420 ! ");
    // colorimetry bt601 - как в GstPlayer: камеры Dahua передают full range, v4l2h264dec 1.22 иначе не согласует
    const QString pipelineStr = QString(
        "filesrc location=\"%1\" ! tsdemux ! h264parse ! "
        "capssetter caps=\"video/x-h264,colorimetry=(string)bt601\" ! "
        "%2"
        "appsink name=sink sync=true max-buffers=2 drop=false"
    ).arg(fileName).arg(scale);

    GError *err = nullptr;
    GstElement *p = gst_parse_launch(pipelineStr.toUtf8().constData(), &err);
    if (err)
    {
        qWarning() << "DVR player: pipeline:" << err->message;
        g_error_free(err);
    }
    if (!p)
    {
        emit finished(false);
        return;
    }

    GstElement *sink = gst_bin_get_by_name(GST_BIN(p), "sink");
    if (!sink)
    {
        gst_object_unref(p);
        emit finished(false);
        return;
    }
    GstAppSinkCallbacks cb = {};
    cb.new_sample = &DvrFilePlayer::onNewSample;
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &cb, this, nullptr);
    gst_object_unref(sink);

    pipeline = p;
    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        qWarning() << "DVR player: не удалось запустить" << fileName;
        destroyPipeline();
        emit finished(false);
        return;
    }
    qDebug() << "DVR player: воспроизведение" << QFileInfo(fileName).fileName() << "x" << rate << "с" << startNs / 1000000 << "мс";
    busTimer.start(100);
}

void DvrFilePlayer::stop()
{
    busTimer.stop();
    destroyPipeline();
    paused = false;
    prerolled = false;
    pendingSeekNs = -1;
}

void DvrFilePlayer::pause()
{
    if (!pipeline || paused)
        return;
    gst_element_set_state(pipeline, GST_STATE_PAUSED);// последний кадр остается на экране
    paused = true;
}

void DvrFilePlayer::resume()
{
    if (!pipeline || !paused)
        return;
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    paused = false;
}

void DvrFilePlayer::seek(qint64 posNs)
{
    if (!pipeline)
        return;
    if (posNs < 0)
        posNs = 0;
    if (!prerolled)
    {
        pendingSeekNs = posNs;
        return;
    }
    doSeek(posNs, currentRate);
}

void DvrFilePlayer::setRate(double rate)
{
    if (rate <= 0)
        return;
    currentRate = rate;
    if (!pipeline)
        return;
    const qint64 pos = position();
    if (!prerolled)
    {
        pendingSeekNs = qMax<qint64>(pos, 0);
        return;
    }
    doSeek(qMax<qint64>(pos, 0), rate);
}

bool DvrFilePlayer::doSeek(qint64 posNs, double rate)
{// flush-seek с ключевым кадром: встаем на ближайший опорный кадр (у камер GOP 50 = 2 с)
    const bool ok = gst_element_seek(pipeline, rate, GST_FORMAT_TIME,
                                     GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                                     GST_SEEK_TYPE_SET, posNs, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
    if (!ok)
        qWarning() << "DVR player: перемотка не выполнена" << posNs / 1000000 << "мс x" << rate;
    return ok;
}

qint64 DvrFilePlayer::position()
{
    gint64 pos = -1;
    if (!pipeline || !gst_element_query_position(pipeline, GST_FORMAT_TIME, &pos))
        return -1;
    return pos;
}

qint64 DvrFilePlayer::duration()
{
    gint64 dur = -1;
    if (!pipeline || !gst_element_query_duration(pipeline, GST_FORMAT_TIME, &dur))
        return -1;
    return dur;
}

void DvrFilePlayer::destroyPipeline()
{
    if (!pipeline)
        return;
    GstElement *p = pipeline;
    pipeline = nullptr;
    // после перехода в NULL потоки GStreamer остановлены, колбэк appsink больше не вызывается
    gst_element_set_state(p, GST_STATE_NULL);
    gst_object_unref(p);
}

void DvrFilePlayer::pollBus()
{
    if (!pipeline)
        return;

    GstBus *bus = gst_element_get_bus(pipeline);
    if (!bus)
        return;

    bool done = false;
    bool ok = true;
    GstMessage *msg;
    while (!done && (msg = gst_bus_pop_filtered(bus, GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS |
                                                                     GST_MESSAGE_ASYNC_DONE))) != nullptr)
    {
        const GstMessageType t = GST_MESSAGE_TYPE(msg);
        if (t == GST_MESSAGE_ASYNC_DONE)
        {// preroll или завершение перемотки
            if (!prerolled)
            {
                prerolled = true;
                if (pendingSeekNs >= 0)
                {
                    const qint64 pos = pendingSeekNs;
                    pendingSeekNs = -1;
                    doSeek(pos, currentRate);
                }
            }
        }
        else if (t == GST_MESSAGE_ERROR)
        {
            GError *e = nullptr;
            gchar *dbg = nullptr;
            gst_message_parse_error(msg, &e, &dbg);
            qWarning() << "DVR player: ошибка" << (e ? e->message : "?") << QFileInfo(currentFile).fileName();
            if (e)
                g_error_free(e);
            g_free(dbg);
            ok = false;
            done = true;
        }
        else
            done = true;// EOS
        gst_message_unref(msg);
    }
    gst_object_unref(bus);

    if (done)
    {// последний кадр остается на экране
        stop();
        emit finished(ok);
    }
}

GstFlowReturn DvrFilePlayer::onNewSample(GstAppSink *sink, gpointer user_data)
{// поток GStreamer: копируем плоскости I420 плотно (как в GstPlayer) и отдаем в GUI
    DvrFilePlayer *self = static_cast<DvrFilePlayer*>(user_data);

    GstSample *sample = gst_app_sink_pull_sample(sink);
    if (!sample)
        return GST_FLOW_OK;

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);
    GstVideoInfo info;
    if (!buffer || !caps || !gst_video_info_from_caps(&info, caps) ||
        GST_VIDEO_INFO_FORMAT(&info) != GST_VIDEO_FORMAT_I420)
    {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    GstVideoFrame frame;
    if (!gst_video_frame_map(&frame, &info, buffer, GST_MAP_READ))
    {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    auto copyPlane = [&frame](int plane, int cw, int ch) {
        QByteArray out(cw * ch, Qt::Uninitialized);
        const guint8 *src = static_cast<const guint8*>(GST_VIDEO_FRAME_PLANE_DATA(&frame, plane));
        const int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, plane);
        char *dst = out.data();
        for (int r = 0; r < ch; ++r)
            memcpy(dst + r * cw, src + r * stride, cw);
        return out;
    };

    const int w = GST_VIDEO_FRAME_WIDTH(&frame);
    const int h = GST_VIDEO_FRAME_HEIGHT(&frame);
    const int cw = GST_VIDEO_FRAME_COMP_WIDTH(&frame, 1);
    const int ch = GST_VIDEO_FRAME_COMP_HEIGHT(&frame, 1);
    QByteArray yData = copyPlane(0, w, h);
    QByteArray uData = copyPlane(1, cw, ch);
    QByteArray vData = copyPlane(2, cw, ch);

    gst_video_frame_unmap(&frame);
    gst_sample_unref(sample);

    emit self->frameReady(yData, uData, vData, w, h);
    return GST_FLOW_OK;
}
