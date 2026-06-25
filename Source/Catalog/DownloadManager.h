#pragma once

#include <JuceHeader.h>
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

/** Async HTTP/file downloads and zip archive handling. */
class DownloadManager final : public juce::URL::DownloadTaskListener
{
public:
    using ProgressCallback = std::function<void(const DownloadProgress&)>;

    DownloadManager();
    ~DownloadManager() override;

    void downloadFileAsync (const juce::String& url,
                            const juce::File& destination,
                            ProgressCallback onProgress);

    /** Extract a .zip / .kitforgepack archive into destination (overwrites). */
    bool extractArchive (const juce::File& archive, const juce::File& destination, juce::String& error);

    /** Zip a folder to archive path (.kitforgepack or .zip). */
    bool createZipFromFolder (const juce::File& sourceFolder, const juce::File& archiveFile, juce::String& error);

    void cancelAll();

private:
    juce::CriticalSection lock;
    std::unique_ptr<juce::URL::DownloadTask> activeTask;
    ProgressCallback progressCallback;
    DownloadProgress currentProgress;

    void finished (juce::URL::DownloadTask* task, bool success) override;
    void progress (juce::URL::DownloadTask* task, int64 bytesDownloaded, int64 totalLength) override;

    void notifyProgress (const DownloadProgress& update);
    static juce::File normaliseUrlToFile (const juce::String& url);
};
