#include "DownloadManager.h"

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

    currentProgress = {};
    currentProgress.taskId = juce::Uuid().toString();
    currentProgress.label = destination.getFileName();
    currentProgress.outputPath = destination;
    notifyProgress (currentProgress);

    destination.getParentDirectory().createDirectory();

    // Local file:// URL — copy immediately without HTTP.
    const auto localFile = normaliseUrlToFile (url);

    if (localFile.existsAsFile())
    {
        if (destination.existsAsFile())
            destination.deleteFile();

        if (! localFile.copyFileTo (destination))
        {
            currentProgress.failed = true;
            currentProgress.finished = true;
            currentProgress.errorMessage = "Failed to copy local pack file.";
            notifyProgress (currentProgress);
            return;
        }

        currentProgress.progress = 1.0;
        currentProgress.finished = true;
        notifyProgress (currentProgress);
        return;
    }

    if (url.trim().isEmpty())
    {
        currentProgress.failed = true;
        currentProgress.finished = true;
        currentProgress.errorMessage = "Download URL is empty.";
        notifyProgress (currentProgress);
        return;
    }

    juce::URL downloadUrl (url);
    activeTask = downloadUrl.downloadToFile (destination, juce::URL::DownloadTaskOptions().withListener (this));

    if (activeTask == nullptr)
    {
        currentProgress.failed = true;
        currentProgress.finished = true;
        currentProgress.errorMessage = "Could not start download task.";
        notifyProgress (currentProgress);
    }
}

void DownloadManager::finished (juce::URL::DownloadTask* task, bool success)
{
    juce::ignoreUnused (task);

    const juce::ScopedLock scopedLock (lock);

    if (success && currentProgress.outputPath.existsAsFile())
    {
        currentProgress.progress = 1.0;
        currentProgress.finished = true;
    }
    else
    {
        currentProgress.failed = true;
        currentProgress.finished = true;
        currentProgress.errorMessage = "Download failed (HTTP "
            + juce::String (task != nullptr ? task->statusCode() : 0) + ").";
    }

    activeTask.reset();
    notifyProgress (currentProgress);
}

void DownloadManager::progress (juce::URL::DownloadTask* task, int64 bytesDownloaded, int64 totalLength)
{
    juce::ignoreUnused (task);

    const juce::ScopedLock scopedLock (lock);

    if (totalLength > 0)
        currentProgress.progress = juce::jlimit (0.0, 1.0, (double) bytesDownloaded / (double) totalLength);
    else
        currentProgress.progress = 0.0;

    notifyProgress (currentProgress);
}

void DownloadManager::notifyProgress (const DownloadProgress& update)
{
    if (progressCallback)
    {
        juce::MessageManager::callAsync ([cb = progressCallback, update]
        {
            cb (update);
        });
    }
}

bool DownloadManager::extractArchive (const juce::File& archive, const juce::File& destination, juce::String& error)
{
    if (! archive.existsAsFile())
    {
        error = "Archive not found: " + archive.getFullPathName();
        return false;
    }

    if (destination.exists())
        destination.deleteRecursively();

    destination.createDirectory();

    juce::ZipFile zip (archive);
    const auto result = zip.uncompressTo (destination, true);

    if (result.failed())
    {
        error = result.getErrorMessage();
        return false;
    }

    return true;
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
    const juce::ScopedLock scopedLock (lock);
    activeTask.reset();
    progressCallback = nullptr;
}
