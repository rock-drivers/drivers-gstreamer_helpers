#ifndef GSTREAMER_HELPERS_APP_HPP
#define GSTREAMER_HELPERS_APP_HPP

#include <base/samples/Frame.hpp>
#include <gst/gst.h>
#include <gst/gstcaps.h>
#include <gst/video/video-info.h>

namespace gstreamer_helpers {
    /** Helpers to interact with the app elements (appsrc/appsink)*/
    namespace app {
        /** Set the caps for an appsrc element according to the received frame
         * parameters
         *
         * Do it once before pushing data to the appsrc. Keep the return value
         * to pass to pushRockFrame
         */
        GstVideoInfo initAppSrcCaps(base::samples::frame::Frame const& frame,
            GstElement* appsrc);

        /** Push a rock frame into a GStreamer pipeline via an appsink element
         *
         * The info struct is the one returned by initAppSrcCaps
         */
        void pushRockFrame(base::samples::frame::Frame const& frame,
            GstVideoInfo& info,
            GstElement* appsrc);

        /** Pull a rock frame out of a GStreamer pipeline */
        void pullRockFrame(GstElement* appsink, base::samples::frame::Frame& frame);

        /** Convert the GStreamer video format to Rock's frame mode */
        base::samples::frame::frame_mode_t gstFormatToFrameMode(GstVideoFormat format);

        /** Convert Rock's frame mode into GStreamer's video format */
        GstVideoFormat frameModeToGSTFormat(base::samples::frame::frame_mode_t format);

    };
}

#endif
