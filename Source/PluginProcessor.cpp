#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Serialization/KitSerializer.h"
#include "Core/KitForgePaths.h"
#include "Core/DemoKitSampleBindings.h"

KitForgeAudioProcessor::KitForgeAudioProcessor()
     : AudioProcessor (BusesProperties()
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    services.initialize();

    if (services.consumeDemoKitRepairFlag())
    {
        const auto installPath = KitForgePaths::getLibraryInstallPath ("demo-rock-kit");
        const auto importResult = services.getPackImporter().importFolder (installPath);

        if (importResult.success)
        {
            auto kit = importResult.kit;
            kit.resolveSamplePaths (installPath);
            DemoKitSampleBindings::bindSamplePathsFromFolder (kit, installPath);
            kit.resolveSamplePaths (installPath);
            kitModel.importContents (kit);
            rebuildEngine();
        }
    }
    else
    {
        rebuildEngine();
    }
}

KitForgeAudioProcessor::~KitForgeAudioProcessor() = default;

const juce::String KitForgeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool KitForgeAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool KitForgeAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool KitForgeAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double KitForgeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int KitForgeAudioProcessor::getNumPrograms() { return 1; }
int KitForgeAudioProcessor::getCurrentProgram() { return 0; }
void KitForgeAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String KitForgeAudioProcessor::getProgramName (int index) { juce::ignoreUnused (index); return {}; }
void KitForgeAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

void KitForgeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    samplerEngine.prepare (sampleRate, samplesPerBlock);
}

void KitForgeAudioProcessor::releaseResources() {}

bool KitForgeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void KitForgeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    juce::MidiBuffer playbackMidi;

    {
        const juce::ScopedLock lock (modelLock);

        for (const auto metadata : midiMessages)
        {
            const auto message = metadata.getMessage();
            bool skipNote = false;

            if (message.isNoteOn())
                skipNote = midiLearnManager.processMidiNote (message.getNoteNumber(), kitModel);

            if (! skipNote)
                playbackMidi.addEvent (message, metadata.samplePosition);
        }

        samplerEngine.processBlock (buffer, playbackMidi, kitModel, 0, buffer.getNumSamples());
    }
}

void KitForgeAudioProcessor::triggerPiece (const juce::String& pieceId, float velocity)
{
    samplerEngine.queueTriggerPiece (pieceId, velocity);
}


void KitForgeAudioProcessor::rebuildEngine()
{
    const juce::ScopedLock lock (modelLock);
    samplerEngine.rebuildFromModel (kitModel);
}

bool KitForgeAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* KitForgeAudioProcessor::createEditor()
{
    return new KitForgeAudioProcessorEditor (*this);
}

void KitForgeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const juce::ScopedLock lock (modelLock);
    juce::MemoryOutputStream stream (destData, false);
    stream.writeString (juce::JSON::toString (KitSerializer::kitToVar (kitModel)));
}

void KitForgeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, (size_t) sizeInBytes, false);
    juce::var parsed;

    if (juce::JSON::parse (stream.readString(), parsed).failed())
        return;

    {
        const juce::ScopedLock lock (modelLock);
        KitSerializer::kitFromVar (kitModel, parsed);

        if (! DemoKitSampleBindings::kitHasAssignedSamples (kitModel))
        {
            const auto demoInstall = KitForgePaths::getLibraryInstallPath ("demo-rock-kit");
            DemoKitSampleBindings::bindSamplePathsFromFolder (kitModel, demoInstall);
            kitModel.resolveSamplePaths (demoInstall);
        }
    }

    rebuildEngine();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KitForgeAudioProcessor();
}
