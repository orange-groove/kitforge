#include "DemoSampleAssets.h"
#include "DemoSampleBinaryData.h"

namespace
{
    struct BundledSampleEntry
    {
        const char* fileName;
        const char* data;
        int dataSize;
    };

    const BundledSampleEntry kBundledSamples[] =
    {
        { "kick_center.wav",     DemoSampleBinary::kick_center_wav,     DemoSampleBinary::kick_center_wavSize },
        { "snare_center.wav",    DemoSampleBinary::snare_center_wav,    DemoSampleBinary::snare_center_wavSize },
        { "snare_rimshot.wav",   DemoSampleBinary::snare_rimshot_wav,   DemoSampleBinary::snare_rimshot_wavSize },
        { "rack_tom_hit.wav",    DemoSampleBinary::rack_tom_hit_wav,    DemoSampleBinary::rack_tom_hit_wavSize },
        { "floor_tom_hit.wav",   DemoSampleBinary::floor_tom_hit_wav,   DemoSampleBinary::floor_tom_hit_wavSize },
        { "hihat_closed.wav",    DemoSampleBinary::hihat_closed_wav,    DemoSampleBinary::hihat_closed_wavSize },
        { "hihat_open.wav",      DemoSampleBinary::hihat_open_wav,      DemoSampleBinary::hihat_open_wavSize },
        { "crash_hit.wav",       DemoSampleBinary::crash_hit_wav,       DemoSampleBinary::crash_hit_wavSize },
        { "ride_bow.wav",        DemoSampleBinary::ride_bow_wav,        DemoSampleBinary::ride_bow_wavSize },
    };
}

bool DemoSampleAssets::writeBundledSample (const juce::File& destination, const juce::String& fileName)
{
    for (const auto& entry : kBundledSamples)
    {
        if (fileName != entry.fileName)
            continue;

        destination.getParentDirectory().createDirectory();

        if (destination.existsAsFile())
            destination.deleteFile();

        juce::FileOutputStream stream (destination);

        if (stream.failedToOpen())
            return false;

        return stream.write (entry.data, (size_t) entry.dataSize) == (size_t) entry.dataSize;
    }

    return false;
}
