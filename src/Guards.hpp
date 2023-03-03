#ifndef GSTREAMER_HELPERS_GUARDS_HPP
#define GSTREAMER_HELPERS_GUARDS_HPP

#include <base/samples/Frame.hpp>
#include <gst/gst.h>
#include <gst/gstcaps.h>
#include <gst/video/video-info.h>

#define ROCK_GSTREAMER_UNREF(GstStruct, gst_unref)                                       \
    inline void unref(GstStruct* obj)                                                    \
    {                                                                                    \
        gst_unref(obj);                                                                  \
    }

namespace gstreamer_helpers {
    namespace guards {
        ROCK_GSTREAMER_UNREF(GstElement, gst_object_unref);
        ROCK_GSTREAMER_UNREF(GstCaps, gst_caps_unref);
        ROCK_GSTREAMER_UNREF(GstSample, gst_sample_unref);
        ROCK_GSTREAMER_UNREF(GstBuffer, gst_buffer_unref);
        ROCK_GSTREAMER_UNREF(GstMemory, gst_memory_unref);
        ROCK_GSTREAMER_UNREF(GstVideoFrame, gst_video_frame_unmap);

        template <typename T> struct GstUnrefGuard {
            T* object;

            explicit GstUnrefGuard(T* object)
                : object(object)
            {
            }

            ~GstUnrefGuard()
            {
                if (object) {
                    unref(object);
                }
            }

            T* release()
            {
                T* ret = object;
                object = nullptr;
                return ret;
            }
        };

        struct GstMemoryUnmapGuard {
            GstMemory* memory;
            GstMapInfo& mapInfo;
            GstMemoryUnmapGuard(GstMemory* memory, GstMapInfo& mapInfo)
                : memory(memory)
                , mapInfo(mapInfo)
            {
            }
            ~GstMemoryUnmapGuard()
            {
                gst_memory_unmap(memory, &mapInfo);
            }
        };
    }
}

#endif