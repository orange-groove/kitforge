#include "LooseSampleFolderImporter.h"
#include "KitModelBuilder.h"
#include "SampleNameParser.h"
#include "LlmSampleClassifier.h"

ImportResult LooseSampleFolderImporter::scanFolder (const juce::File& folder) const
{
    ImportResult result;
    result.importFormat = "loose_wav";
    result.sourceRoot = folder;

    if (! folder.isDirectory())
    {
        result.errorMessage = "Folder not found: " + folder.getFullPathName();
        return result;
    }

    result.kitName = folder.getFileName();

    SampleNameParser parser;
    std::vector<SampleMetadata> parsedSamples;
    int wavCount = 0;

    for (const auto& file : folder.findChildFiles (juce::File::findFiles, true))
    {
        if (! file.hasFileExtension ("wav"))
        {
            if (file.hasFileExtension ("mp3") || file.hasFileExtension ("aiff")
                || file.hasFileExtension ("flac"))
            {
                result.warnings.push_back (ImportWarning::make (ImportWarningType::unsupportedFileType,
                                                                "Unsupported file type (WAV only): "
                                                                    + file.getFileName(),
                                                                file.getFullPathName()));
            }

            continue;
        }

        ++wavCount;
        auto meta = parser.parseFile (file);

        if (meta.confidence <= 0.0f)
        {
            result.warnings.push_back (ImportWarning::make (ImportWarningType::unknownSample,
                                                              "Unknown sample: " + file.getFileName(),
                                                              file.getFullPathName()));
        }

        parsedSamples.push_back (std::move (meta));
    }

    if (parsedSamples.empty())
    {
        result.errorMessage = "No WAV files found in folder.";
        return result;
    }

    LlmSampleClassifier::classify (result.kitName, parsedSamples);

    result.kit = KitModelBuilder::buildFromMetadata (result.kitName, parsedSamples, result.warnings);
    result.stats = ImportResult::computeStats (result.kit, wavCount);
    result.success = true;
    return result;
}
