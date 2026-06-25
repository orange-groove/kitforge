#pragma once

#include <JuceHeader.h>
#include "KitForgePaths.h"

/** Creates a local demo .kitforgepack for offline catalog testing. */
namespace DemoKitFactory
{
    /** Ensures ~/Documents/KitForge/Catalog/packs/demo-rock-kit.kitforgepack exists. */
    juce::File ensureDemoPackExists();

    /** Returns file:// URL for the demo pack, or empty if unavailable. */
    juce::String getDemoPackDownloadUrl();

    /** Re-extracts the demo pack if an installed copy is missing WAV samples. Returns true if repaired. */
    bool repairInstalledDemoKitIfNeeded();
}
