#include "DownloadManager.h"

namespace
{
    constexpr int kDownloadBufferSize = 0x8000;
    constexpr int kConnectTimeoutMs = 60000;

    juce::String httpErrorMessage (int statusCode)
    {
        return "Download failed (HTTP " + juce::String (statusCode) + ").";
    }
}

class DownloadManager::DownloadWorker final : public juce::Thread
{
public:
    DownloadWorker (DownloadManager& ownerIn,
                    juce::String urlIn,
                    juce::File destinationIn,
                    juce::String taskIdIn)
        : Thread ("KitForge download"),
          owner (ownerIn),
          url (std::move (urlIn)),
          destination (std::move (destinationIn)),
          taskId (std::move (taskIdIn))
    {
    }

    void run() override
    {
        DownloadProgress progress;
        progress.taskId = taskId;
        progress.label = destination.getFileName();
        progress.outputPath = destination;

        destination.deleteFile();
        destination.getParentDirectory().createDirectory();

        juce::URL downloadUrl (url);
        juce::WebInputStream stream (downloadUrl, false);
        stream.withExtraHeaders ("User-Agent: KitForge/1.0\r\n");
        stream.withConnectionTimeout (kConnectTimeoutMs);
        stream.withNumRedirectsToFollow (5);

        if (threadShouldExit() || owner.cancelRequested.load())
            return;

        if (! stream.connect (nullptr))
        {
            progress.failed = true;
            progress.finished = true;
            progress.errorMessage = "Could not connect to download server.";
            owner.notifyProgress (progress);
            return;
        }

        const int statusCode = stream.getStatusCode();

        if (statusCode >= 400)
        {
            progress.failed = true;
            progress.finished = true;
            progress.errorMessage = httpErrorMessage (statusCode);
            owner.notifyProgress (progress);
            return;
        }

        auto output = destination.createOutputStream (kDownloadBufferSize);

        if (output == nullptr)
        {
            progress.failed = true;
            progress.finished = true;
            progress.errorMessage = "Could not create download file.";
            owner.notifyProgress (progress);
            return;
        }

        const int64 totalLength = stream.getTotalLength();
        int64 downloaded = 0;
        juce::HeapBlock<char> buffer (kDownloadBufferSize);

        while (! threadShouldExit() && ! owner.cancelRequested.load() && ! stream.isExhausted())
        {
            const int bytesRead = stream.read (buffer.getData(), kDownloadBufferSize);

            if (bytesRead < 0 || stream.isError())
                break;

            if (bytesRead == 0)
                continue;

            if (! output->write (buffer.getData(), (size_t) bytesRead))
                break;

            downloaded += bytesRead;
            progress.progress = totalLength > 0
                                  ? juce::jlimit (0.0, 1.0, (double) downloaded / (double) totalLength)
                                  : 0.0;
            owner.notifyProgress (progress);
        }

        output.reset();

        if (threadShouldExit() || owner.cancelRequested.load())
            return;

        progress.finished = true;

        if (stream.isError())
        {
            progress.failed = true;
            progress.errorMessage = "Download stream error.";
        }
        else if (! destination.existsAsFile() || destination.getSize() < 64)
        {
            progress.failed = true;
            progress.errorMessage = "Download failed or file is incomplete.";
        }
        else if (statusCode >= 300 && statusCode < 400 && destination.getSize() < 4096)
        {
            progress.failed = true;
            progress.errorMessage = "Download returned an unexpected redirect response.";
        }

        if (! progress.failed)
            progress.progress = 1.0;

        owner.notifyProgress (progress);
    }

private:
    DownloadManager& owner;
    juce::String url;
    juce::File destination;
    juce::String taskId;
};

DownloadManager::DownloadManager() = default;

DownloadManager::~DownloadManager()
{
    cancelAll();
}

juce::File DownloadManager::normaliseUrlToFile (const juce::String& url)
{
    if (url.startsWithIgnoreCase ("file:"))
        return juce::URL (url).getLocalFile();

    return {};
}

void DownloadManager::downloadFileAsync (const juce::String& url,
                                          const juce::File& destination,
                                          ProgressCallback onProgress)
{
    cancelAll();

    const juce::ScopedLock scopedLock (lock);
    progressCallback = std::move (onProgress);
    cancelRequested.store (false);

    DownloadProgress initial;
    initial.taskId = juce::Uuid().toString();
    initial.label = destination.getFileName();
    initial.outputPath = destination;
    notifyProgress (initial);

    destination.getParentDirectory().createDirectory();

    const auto localFile = normaliseUrlToFile (url);

    if (localFile.existsAsFile())
    {
        if (destination.existsAsFile())
            destination.deleteFile();

        DownloadProgress progress = initial;

        if (! localFile.copyFileTo (destination))
        {
            progress.failed = true;
            progress.finished = true;
            progress.errorMessage = "Failed to copy local pack file.";
            notifyProgress (progress);
            return;
        }

        progress.progress = 1.0;
        progress.finished = true;
        notifyProgress (progress);
        return;
    }

    if (url.trim().isEmpty())
    {
        DownloadProgress progress = initial;
        progress.failed = true;
        progress.finished = true;
        progress.errorMessage = "Download URL is empty.";
        notifyProgress (progress);
        return;
    }

    activeWorker = std::make_unique<DownloadWorker> (*this, url, destination, initial.taskId);
    activeWorker->startThread();
}

void DownloadManager::notifyProgress (const DownloadProgress& update)
{
    ProgressCallback callback;

    {
        const juce::ScopedLock scopedLock (lock);
        callback = progressCallback;
    }

    if (callback)
    {
        juce::MessageManager::callAsync ([callback, update]
        {
            callback (update);
        });
    }
}

bool DownloadManager::extractZipArchive (const juce::File& archive, const juce::File& destination, juce::String& error)
{
    juce::ZipFile zip (archive);
    const auto result = zip.uncompressTo (destination, true);

    if (result.failed())
    {
        error = result.getErrorMessage();
        return false;
    }

    return true;
}

bool DownloadManager::extractTarArchive (const juce::File& archive,
                                          const juce::File& destination,
                                          juce::String& error,
                                          bool gzip)
{
    if (! archive.existsAsFile())
    {
        error = "Archive not found: " + archive.getFullPathName();
        return false;
    }

    if (destination.exists())
        destination.deleteRecursively();

    destination.createDirectory();

    juce::StringArray args;
    args.add ("tar");
    args.add (gzip ? "-xzf" : "-xjf");
    args.add (archive.getFullPathName());
    args.add ("-C");
    args.add (destination.getFullPathName());

    juce::ChildProcess process;

    if (! process.start (args))
    {
        error = "Could not run tar to extract archive.";
        return false;
    }

    if (! process.waitForProcessToFinish (60 * 60 * 1000))
    {
        process.kill();
        error = "Archive extraction timed out.";
        return false;
    }

    if (process.getExitCode() != 0)
    {
        error = process.readAllProcessOutput().trim();

        if (error.isEmpty())
            error = "tar failed with exit code " + juce::String (process.getExitCode());

        return false;
    }

    return true;
}

bool DownloadManager::extractArchive (const juce::File& archive, const juce::File& destination, juce::String& error)
{
    if (! archive.existsAsFile())
    {
        error = "Archive not found: " + archive.getFullPathName();
        return false;
    }

    const auto name = archive.getFileName().toLowerCase();

    if (name.endsWith (".zip") || name.endsWith (".kitforgepack"))
        return extractZipArchive (archive, destination, error);

    if (name.endsWith (".tar.bz2") || name.endsWith (".tbz2"))
        return extractTarArchive (archive, destination, error, false);

    if (name.endsWith (".tar.gz") || name.endsWith (".tgz"))
        return extractTarArchive (archive, destination, error, true);

    error = "Unsupported archive format: " + archive.getFileName();
    return false;
}

bool DownloadManager::createZipFromFolder (const juce::File& sourceFolder, const juce::File& archiveFile, juce::String& error)
{
    if (! sourceFolder.isDirectory())
    {
        error = "Source folder does not exist.";
        return false;
    }

    juce::ZipFile::Builder builder;

    for (const auto& file : sourceFolder.findChildFiles (juce::File::findFiles, true))
    {
        const auto relativePath = file.getRelativePathFrom (sourceFolder);
        builder.addFile (file, 9, relativePath);
    }

    if (archiveFile.existsAsFile())
        archiveFile.deleteFile();

    archiveFile.getParentDirectory().createDirectory();

    juce::FileOutputStream stream (archiveFile);

    if (stream.failedToOpen())
    {
        error = "Could not create archive file.";
        return false;
    }

    if (! builder.writeToStream (stream, nullptr))
    {
        error = "Failed to write zip archive.";
        return false;
    }

    return true;
}

void DownloadManager::cancelAll()
{
    cancelRequested.store (true);

    std::unique_ptr<DownloadWorker> worker;

    {
        const juce::ScopedLock scopedLock (lock);
        worker = std::move (activeWorker);
        progressCallback = nullptr;
    }

    if (worker != nullptr)
    {
        worker->signalThreadShouldExit();
        worker->waitForThreadToExit (10000);
    }
}
