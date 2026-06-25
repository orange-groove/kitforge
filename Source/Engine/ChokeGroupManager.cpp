#include "ChokeGroupManager.h"
#include "DrumVoice.h"

void ChokeGroupManager::registerVoice (DrumVoice* voice, const juce::String& chokeGroupId)
{
    if (voice == nullptr || chokeGroupId.isEmpty())
        return;

    activeVoices.push_back ({ voice, chokeGroupId });
}

void ChokeGroupManager::unregisterVoice (DrumVoice* voice)
{
    activeVoices.erase (std::remove_if (activeVoices.begin(), activeVoices.end(),
                                        [voice] (const Entry& e) { return e.voice == voice; }),
                        activeVoices.end());
}

void ChokeGroupManager::chokeGroup (const juce::String& chokeGroupId)
{
    if (chokeGroupId.isEmpty())
        return;

    for (auto& entry : activeVoices)
    {
        if (entry.chokeGroupId == chokeGroupId && entry.voice != nullptr)
            entry.voice->forceStop();
    }
}

void ChokeGroupManager::clear()
{
    activeVoices.clear();
}
