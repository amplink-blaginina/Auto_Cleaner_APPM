#include "cameraplayer.h"
#include "cameraview.h"

#include <gst/app/app.h>
#include <QDebug>

GstPlayer::GstPlayer(CameraView* widget, QObject* parent)
    : QObject(parent),
      cameraWidget(widget)
{
    // gst_init may be called multiple times safely; but ensure called before using GStreamer
    gst_init(nullptr, nullptr);

    // Connect our frameReady signal to the widget slot using queued connection so the slot runs in GUI thread
    if (cameraWidget) {
        connect(this, &GstPlayer::frameReady,
                cameraWidget, &CameraView::setFrameData,
                Qt::QueuedConnection);
    }
}

GstPlayer::~GstPlayer() {
    stop();
}

void GstPlayer::play(const QString& url) {
    stop();

    QString pipelineStr =
        QString(
            "rtspsrc location=%1 latency=200 ! "
            "rtph264depay ! h264parse ! v4l2h264dec ! "
            "video/x-raw,format=I420 ! "
            "appsink name=sink sync=false max-buffers=1 drop=true"
        ).arg(url);

    GError* err = nullptr;
    pipeline = gst_parse_launch(pipelineStr.toUtf8().constData(), &err);
    if (!pipeline) {
        qCritical() << "GstPlayer: pipeline error:" << (err ? err->message : "unknown");
        if (err) g_error_free(err);
        pipeline = nullptr;
        return;
    }

    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    if (!sink) {
        qCritical() << "GstPlayer: appsink not found in pipeline";
        gst_object_unref(pipeline);
        pipeline = nullptr;
        return;
    }

    // Ensure appsink is configured (some properties already set in pipeline string)
    gst_app_sink_set_emit_signals(GST_APP_SINK(sink), true);
    gst_app_sink_set_max_buffers(GST_APP_SINK(sink), 1);
    gst_app_sink_set_drop(GST_APP_SINK(sink), true);

    // Connect new-sample signal; pass 'this' as user_data so callback can emit signal
    signalHandlerId = g_signal_connect(sink, "new-sample", G_CALLBACK(onNewSample), this);

    // Unref local sink reference (pipeline still holds it)
    gst_object_unref(sink);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void GstPlayer::stop() {
    if (!pipeline) return;

    // Disconnect signal from appsink before shutting down pipeline
    if (signalHandlerId != 0) {
        GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
        if (sink) {
            g_signal_handler_disconnect(sink, signalHandlerId);
            gst_object_unref(sink);
        }
        signalHandlerId = 0;
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    pipeline = nullptr;
}

GstFlowReturn GstPlayer::onNewSample(GstElement* sink, gpointer user_data) {
    // user_data is GstPlayer*
    GstPlayer* self = static_cast<GstPlayer*>(user_data);
    if (!self) return GST_FLOW_ERROR;

    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (!sample) return GST_FLOW_OK;

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstCaps* caps = gst_sample_get_caps(sample);

    GstVideoInfo info;
    if (!gst_video_info_from_caps(&info, caps)) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    int w = GST_VIDEO_INFO_WIDTH(&info);
    int h = GST_VIDEO_INFO_HEIGHT(&info);

    // Map buffer and copy planes. Assume I420 layout (Y, U, V contiguous)
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    int ySize = w * h;
    int uvSize = ySize / 4;

    QByteArray yData;
    QByteArray uData;
    QByteArray vData;

    if ((gsize)map.size >= (gsize)(ySize + uvSize * 2)) {
        yData = QByteArray(reinterpret_cast<const char*>(map.data), ySize);
        uData = QByteArray(reinterpret_cast<const char*>(map.data + ySize), uvSize);
        vData = QByteArray(reinterpret_cast<const char*>(map.data + ySize + uvSize), uvSize);
    } else {
        // Fallback: unexpected buffer size
        qWarning() << "GstPlayer: unexpected buffer size, skipping frame";
        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    gst_buffer_unmap(buffer, &map);

    // Emit Qt signal — connection to widget is queued, so slot runs in GUI thread
    emit self->frameReady(yData, uData, vData, w, h);

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}
