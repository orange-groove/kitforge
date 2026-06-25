#include "SFZScanner.h"
#include <algorithm>

namespace
{
    bool isSfzFile (const juce::File& file)
    {
        return file.hasFileExtension ("sfz");
    }
}

int SFZScanner::scoreFileName (const juce::String& fileName)
{
    const auto lower = fileName.toLowerCase();
    int score = 0;

    if (lower.contains ("drum"))   score += 40;
    if (lower.contains ("kit"))    score += 35;
    if (lower.contains ("gm"))     score += 25;
    if (lower.contains ("general")) score += 20;
    if (lower.contains ("full"))   score += 10;
    if (lower.contains ("main"))   score += 10;

    if (lower.contains ("demo") || lower.contains ("test")) score -= 15;

    return score;
}

juce::Array<SFZCandidate> SFZScanner::scanFolder (const juce::File& root)
{
    juce::Array<SFZCandidate> candidates;

    if (! root.isDirectory())
        return candidates;

    for (const auto& file : root.findChildFiles (juce::File::findFiles, true, "*.sfz"))
    {
        SFZCandidate candidate;
        candidate.file = file;
        candidate.relativePath = file.getRelativePathFrom (root);
        candidate.score = scoreFileName (file.getFileNameWithoutExtension());
        candidates.add (std::move (candidate));
    }

    std::sort (candidates.begin(), candidates.end(),
               [] (const SFZCandidate& a, const SFZCandidate& b)
               {
                   if (a.score != b.score)
                       return a.score > b.score;

                   return a.relativePath.compareIgnoreCase (b.relativePath) < 0;
               });

    return candidates;
}

juce::File SFZScanner::pickBest (const juce::Array<SFZCandidate>& candidates)
{
    return candidates.isEmpty() ? juce::File() : candidates.getReference (0).file;
}
