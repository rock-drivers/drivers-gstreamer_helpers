#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gstreamer_helpers/App.hpp>
#include <gstreamer_helpers/Guards.hpp>

#include <cstring>

using namespace gstreamer_helpers;
using namespace gstreamer_helpers::guards;
using namespace std;
using namespace base::samples::frame;

base::samples::frame::frame_mode_t app::gstFormatToFrameMode(GstVideoFormat format)
{
    switch (format) {
        case GST_VIDEO_FORMAT_RGB:
            return base::samples::frame::MODE_RGB32;
        case GST_VIDEO_FORMAT_BGR:
            return base::samples::frame::MODE_BGR;
        case GST_VIDEO_FORMAT_GRAY8:
            return base::samples::frame::MODE_GRAYSCALE;
        default:
            // Should not happen, the component validates the frame mode
            // against the accepted modes
            throw std::runtime_error("unsupported GST frame format received");
    }
}

GstVideoFormat app::frameModeToGSTFormat(base::samples::frame::frame_mode_t format)
{
    switch (format) {
        case base::samples::frame::MODE_RGB:
            return GST_VIDEO_FORMAT_RGB;
        case base::samples::frame::MODE_BGR:
            return GST_VIDEO_FORMAT_BGR;
        case base::samples::frame::MODE_RGB32:
            return GST_VIDEO_FORMAT_RGBx;
        case base::samples::frame::MODE_GRAYSCALE:
            return GST_VIDEO_FORMAT_GRAY8;
        default:
            // Should not happen, the component validates the frame mode
            // against the accepted modes
            throw std::runtime_error("unsupported frame format received");
    }
}

/** Set the caps for the appsrc element according to the received frame
 * parameters
 *
 * Do it once before pushing data to the appsrc
 */
GstVideoInfo app::initAppSrcCaps(Frame const& frame, GstElement* appsrc)
{
    auto frameMode = frame.frame_mode;
    auto width = frame.size.width;
    auto height = frame.size.height;

    auto format = frameModeToGSTFormat(frameMode);
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
        "format",
        G_TYPE_STRING,
        gst_video_format_to_string(format),
        "width",
        G_TYPE_INT,
        width,
        "height",
        G_TYPE_INT,
        height,
        NULL);
    if (!caps) {
        throw std::runtime_error("failed to generate caps");
    }
    GstUnrefGuard<GstCaps> caps_unref_guard(caps);
    g_object_set(appsrc, "caps", caps, NULL);

    GstVideoInfo info;
    gst_video_info_from_caps(&info, caps);
    return info;
}

/** Push a rock frame into a GStreamer pipeline */
void app::pushRockFrame(Frame const& frame, GstVideoInfo& info, GstElement* appsrc)
{
    /* Create a buffer to wrap the last received image */
    GstBuffer* buffer = gst_buffer_new_and_alloc(info.size);
    GstUnrefGuard<GstBuffer> unref_guard(buffer);

    int sourceStride = frame.getRowSize();
    int targetStride = GST_VIDEO_INFO_PLANE_STRIDE(&info, 0);
    if (targetStride != sourceStride) {
        GstVideoFrame vframe;
        gst_video_frame_map(&vframe, &info, buffer, GST_MAP_WRITE);
        GstUnrefGuard<GstVideoFrame> memory_unmap_guard(&vframe);
        guint8* pixels = static_cast<uint8_t*>(GST_VIDEO_FRAME_PLANE_DATA(&vframe, 0));
        int targetStride = GST_VIDEO_FRAME_PLANE_STRIDE(&vframe, 0);
        int rowSize = frame.getRowSize();
        for (int i = 0; i < info.height; ++i) {
            memcpy(pixels + i * targetStride,
                frame.image.data() + i * sourceStride,
                rowSize);
        }
    }
    else {
        gst_buffer_fill(buffer, 0, frame.image.data(), frame.image.size());
    }

    GstFlowReturn ret;
    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
}

/** Pull a rock frame out of a GStreamer pipeline */
void app::pullRockFrame(GstElement* appsink, Frame& frame)
{
    /* Retrieve the buffer */
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
    GstUnrefGuard<GstSample> sample_unref_guard(sample);

    /* If we have a new sample we have to send it to our Rock frame */
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (buffer == NULL) {
        throw std::logic_error("could not allocate buffer in pullRockFrame");
    }

    GstVideoInfo videoInfo;
    {
        GstCaps* caps = gst_sample_get_caps(sample);
        gst_video_info_from_caps(&videoInfo, caps);
    }

    int width = videoInfo.width;
    int height = videoInfo.height;

    GstMemory* memory = gst_buffer_get_memory(buffer, 0);
    GstUnrefGuard<GstMemory> memory_unref_guard(memory);

    GstMapInfo mapInfo;
    if (!gst_memory_map(memory, &mapInfo, GST_MAP_READ)) {
        throw std::logic_error("could not map buffer in pullRockFrame");
    }
    GstMemoryUnmapGuard memory_unmap_guard(memory, mapInfo);

    auto frame_mode = gstFormatToFrameMode(videoInfo.finfo->format);
    frame.init(width, height, 8, frame_mode);
    frame.time = base::Time::now();
    frame.frame_status = STATUS_VALID;

    uint8_t* pixels = &(frame.image[0]);
    if (frame.getNumberOfBytes() > mapInfo.size) {
        throw std::logic_error(
            "Inconsistent number of bytes. Rock's image type calculated " +
            to_string(frame.getNumberOfBytes()) + " while GStreamer only has " +
            to_string(mapInfo.size));
    }

    int sourceStride = videoInfo.stride[0];
    int targetStride = frame.getRowSize();
    if (sourceStride != targetStride) {
        for (int i = 0; i < height; ++i) {
            std::memcpy(pixels + targetStride * i,
                mapInfo.data + sourceStride * i,
                frame.getRowSize());
        }
    }
    else {
        std::memcpy(pixels, mapInfo.data, frame.getNumberOfBytes());
    }
}