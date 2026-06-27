#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <functional>
#include <memory>

struct DownloadProgress
{
    juce::String taskId;
    juce::String label;
    double progress = 0.0; // 0..1
    bool finished = false;
    bool failed = false;
    juce::String errorMessage;
    juce::File outputPath;
};

/** Async HTTP/file downloads and archive extraction. */
class DownloadManager
{
public:
    using ProgressCallback = std::function<void(const DownloadProgress&)>;

    DownloadManager();
    ~DownloadManager();

    void downloadFileAsync (const juce::String& url,
                            const juce::File& destination,
                            ProgressCallback onProgress);

    /** Extract a downloaded archive (.zip, .tar.bz2, .tar.gz) into destination. */
    bool extractArchive (const juce::File& archive, const juce::File& destination, juce::String& error);

    /** Zip a folder to archive path (.kitforgepack or .zip). */
    bool createZipFromFolder (const juce::File& sourceFolder, const juce::File& archiveFile, juce::String& error);

    void cancelAll();

private:
    class DownloadWorker;

    juce::CriticalSection lock;
    std::unique_ptr<DownloadWorker> activeWorker;
    ProgressCallback progressCallback;
    std::atomic<bool> cancelRequested { false };

    void notifyProgress (const DownloadProgress& update);
    static juce::File normaliseUrlToFile (const juce::String& url);
    static bool extractZipArchive (const juce::File& archive, const juce::File& destination, juce::String& error);
    static bool extractTarArchive (const juce::File& archive, const juce::File& destination, juce::String& error, bool gzip);
};
