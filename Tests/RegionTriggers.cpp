#include "../JuceLibraryCode/JuceHeader.h"
#include "catch2/catch.hpp"
#include "../Source/SfzRegion.h"
using namespace Catch::literals;

TEST_CASE("Basic triggers", "Region triggers")
{
    SfzFilePool openFiles { File::getCurrentWorkingDirectory() };
    SfzRegion region { File::getCurrentWorkingDirectory(), openFiles };
    region.parseOpcode({ "sample", "*sine" });
    SECTION("key")
    {
        region.parseOpcode({ "key", "40" });
        region.prepare();
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOff(1, 40, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 41, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::controllerEvent(1, 63, 64), 0.5f) );
    }
    SECTION("lokey and hikey")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "42" });
        region.prepare();
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 39, (uint8)64), 0.5f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOff(1, 40, (uint8)64), 0.5f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 41, (uint8)64), 0.5f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 42, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 43, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOff(1, 42, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOff(1, 42, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::controllerEvent(1, 63, 64), 0.5f) );
    }
    SECTION("key and release trigger")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "trigger", "release" });
        region.prepare();
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.5f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOff(1, 40, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 41, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOff(1, 41, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::controllerEvent(1, 63, 64), 0.5f) );
    }
    SECTION("key and release_key trigger")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "trigger", "release_key" });
        region.prepare();
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.5f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOff(1, 40, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 41, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOff(1, 41, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::controllerEvent(1, 63, 64), 0.5f) );
    }
    // TODO: first and legato triggers
    SECTION("lovel and hivel")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lovel", "60" });
        region.parseOpcode({ "hivel", "70" });
        region.prepare();
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.5f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)60), 0.5f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)70), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 41, (uint8)71), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 41, (uint8)59), 0.5f) );
    }
    SECTION("lochan and hichan")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lochan", "2" });
        region.parseOpcode({ "hichan", "4" });
        region.prepare();
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.5f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(2, 40, (uint8)64), 0.5f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(3, 40, (uint8)64), 0.5f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(4, 40, (uint8)64), 0.5f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(5, 40, (uint8)64), 0.5f) );
    }
    SECTION("lorand and hirand")
    {
        region.parseOpcode({ "key", "40" });
        region.parseOpcode({ "lorand", "0.35" });
        region.parseOpcode({ "hirand", "0.40" });
        region.prepare();
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.34f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.35f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.36f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.37f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.38f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.39f) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.40f) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.41f) );
    }
}

TEST_CASE("Legato triggers", "Region triggers")
{
    SfzFilePool openFiles { File::getCurrentWorkingDirectory() };
    SfzRegion region { File::getCurrentWorkingDirectory(), openFiles };
    region.parseOpcode({ "sample", "*sine" });
    SECTION("First note playing")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "50" });
        region.parseOpcode({ "trigger", "first" });
        region.prepare();
        region.updateSwitches( MidiMessage::noteOn(1, 40, (uint8)64) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.5f) );
        region.updateSwitches( MidiMessage::noteOn(1, 41, (uint8)64) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 41, (uint8)64), 0.5f) );
        region.updateSwitches( MidiMessage::noteOff(1, 40) );
        region.updateSwitches( MidiMessage::noteOff(1, 41) );
        region.updateSwitches( MidiMessage::noteOn(1, 42, (uint8)64) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 42, (uint8)64), 0.5f) );
    }

    SECTION("Second note playing")
    {
        region.parseOpcode({ "lokey", "40" });
        region.parseOpcode({ "hikey", "50" });
        region.parseOpcode({ "trigger", "legato" });
        region.prepare();
        region.updateSwitches( MidiMessage::noteOn(1, 40, (uint8)64) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 40, (uint8)64), 0.5f) );
        region.updateSwitches( MidiMessage::noteOn(1, 41, (uint8)64) );
        REQUIRE( region.appliesTo(MidiMessage::noteOn(1, 41, (uint8)64), 0.5f) );
        region.updateSwitches( MidiMessage::noteOff(1, 40) );
        region.updateSwitches( MidiMessage::noteOff(1, 41) );
        region.updateSwitches( MidiMessage::noteOn(1, 42, (uint8)64) );
        REQUIRE( !region.appliesTo(MidiMessage::noteOn(1, 42, (uint8)64), 0.5f) );
    }
}