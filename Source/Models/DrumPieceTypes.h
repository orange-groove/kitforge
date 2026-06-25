#pragma once

#include <JuceHeader.h>

enum class DrumPieceType
{
    kick,
    snare,
    rackTom,
    floorTom,
    hiHat,
    crash,
    ride,
    china,
    splash,
    accessory
};

enum class ShapeType
{
    circle,
    rectangle,
    oval,
    cymbal,
    polygon
};

inline juce::String drumPieceTypeToString (DrumPieceType type)
{
    switch (type)
    {
        case DrumPieceType::kick:      return "kick";
        case DrumPieceType::snare:     return "snare";
        case DrumPieceType::rackTom:   return "rackTom";
        case DrumPieceType::floorTom:  return "floorTom";
        case DrumPieceType::hiHat:     return "hiHat";
        case DrumPieceType::crash:     return "crash";
        case DrumPieceType::ride:      return "ride";
        case DrumPieceType::china:     return "china";
        case DrumPieceType::splash:    return "splash";
        default:                       return "accessory";
    }
}

inline DrumPieceType drumPieceTypeFromString (const juce::String& s)
{
    if (s == "kick")      return DrumPieceType::kick;
    if (s == "snare")     return DrumPieceType::snare;
    if (s == "rackTom")   return DrumPieceType::rackTom;
    if (s == "floorTom")  return DrumPieceType::floorTom;
    if (s == "hiHat")     return DrumPieceType::hiHat;
    if (s == "crash")     return DrumPieceType::crash;
    if (s == "ride")      return DrumPieceType::ride;
    if (s == "china")     return DrumPieceType::china;
    if (s == "splash")    return DrumPieceType::splash;
    return DrumPieceType::accessory;
}

inline juce::String shapeTypeToString (ShapeType shape)
{
    switch (shape)
    {
        case ShapeType::circle:     return "circle";
        case ShapeType::rectangle:  return "rectangle";
        case ShapeType::oval:       return "oval";
        case ShapeType::cymbal:     return "cymbal";
        case ShapeType::polygon:    return "polygon";
        default:                    return "circle";
    }
}

inline ShapeType shapeTypeFromString (const juce::String& s)
{
    if (s == "rectangle") return ShapeType::rectangle;
    if (s == "oval")      return ShapeType::oval;
    if (s == "cymbal")    return ShapeType::cymbal;
    if (s == "polygon")   return ShapeType::polygon;
    return ShapeType::circle;
}

inline bool isCymbalPieceType (DrumPieceType type)
{
    return type == DrumPieceType::crash
        || type == DrumPieceType::ride
        || type == DrumPieceType::china
        || type == DrumPieceType::splash
        || type == DrumPieceType::hiHat;
}

inline ShapeType defaultShapeForType (DrumPieceType type)
{
    if (type == DrumPieceType::kick)
        return ShapeType::rectangle;

    return ShapeType::circle;
}

/** Resolves the top-down shape for a piece (kick = square, cymbals/drums = circle). */
inline ShapeType effectiveShapeForPiece (DrumPieceType type, ShapeType stored)
{
    if (type == DrumPieceType::kick)
        return ShapeType::rectangle;

    if (type == DrumPieceType::accessory && stored == ShapeType::polygon)
        return ShapeType::polygon;

    return ShapeType::circle;
}

/** Top-down kit view diameter (width == height) in canvas pixels. */
inline float defaultPieceSize (DrumPieceType type)
{
    switch (type)
    {
        case DrumPieceType::kick:      return 120.0f;
        case DrumPieceType::floorTom:  return 96.0f;
        case DrumPieceType::snare:     return 84.0f;
        case DrumPieceType::rackTom:   return 72.0f;
        case DrumPieceType::hiHat:     return 68.0f;
        case DrumPieceType::crash:     return 104.0f;
        case DrumPieceType::ride:      return 110.0f;
        case DrumPieceType::china:     return 100.0f;
        case DrumPieceType::splash:    return 80.0f;
        default:                       return 70.0f;
    }
}

inline juce::String defaultPieceDisplayName (DrumPieceType type)
{
    switch (type)
    {
        case DrumPieceType::kick:      return "Kick";
        case DrumPieceType::snare:     return "Snare";
        case DrumPieceType::rackTom:   return "Rack Tom";
        case DrumPieceType::floorTom:  return "Floor Tom";
        case DrumPieceType::hiHat:     return "Hi-Hat";
        case DrumPieceType::crash:     return "Crash";
        case DrumPieceType::ride:      return "Ride";
        case DrumPieceType::china:     return "China";
        case DrumPieceType::splash:    return "Splash";
        default:                       return "Accessory";
    }
}

inline juce::Colour drumShellFillColour()     { return juce::Colours::white; }
inline juce::Colour drumShellRimColour()      { return juce::Colours::black; }
inline juce::Colour cymbalFillColour()        { return juce::Colour (0xff4a4a4a); }
inline juce::Colour cymbalRimColour()         { return juce::Colours::black; }

/** Paint order for top-down kit view: kick back, toms mid, cymbals front. Lower = further back. */
inline int displayLayerOrder (DrumPieceType type)
{
    switch (type)
    {
        case DrumPieceType::kick:     return 0;
        case DrumPieceType::snare:
        case DrumPieceType::rackTom:
        case DrumPieceType::floorTom: return 10;
        case DrumPieceType::accessory: return 15;
        case DrumPieceType::hiHat:
        case DrumPieceType::crash:
        case DrumPieceType::ride:
        case DrumPieceType::china:
        case DrumPieceType::splash:   return 20;
        default:                      return 10;
    }
}
