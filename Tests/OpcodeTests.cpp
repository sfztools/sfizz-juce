#include "../JuceLibraryCode/JuceHeader.h"
#include "catch2/catch.hpp"
#include "../Source/SfzRegion.h"
using namespace Catch::literals;

TEST_CASE("Opcode Construction", "Opcode class tests")
{
    SECTION("Normal construction")
    {
        SfzOpcode opcode { "sample", "dummy"};
        REQUIRE( opcode.opcode == "sample" );
        REQUIRE( opcode.value == "dummy" );
        REQUIRE( !opcode.parameter );
    }

    SECTION("Normal construction with underscore")
    {
        SfzOpcode opcode { "sample_underscore", "dummy"};
        REQUIRE( opcode.opcode == "sample_underscore" );
        REQUIRE( opcode.value == "dummy" );
        REQUIRE( !opcode.parameter );
    }

    SECTION("Parameterized opcode")
    {
        SfzOpcode opcode { "sample123", "dummy"};
        REQUIRE( opcode.opcode == "sample" );
        REQUIRE( opcode.value == "dummy" );
        REQUIRE( opcode.parameter );
        REQUIRE( *opcode.parameter == 123 );
    }

    SECTION("Parameterized opcode with underscor")
    {
        SfzOpcode opcode { "sample_underscore123", "dummy"};
        REQUIRE( opcode.opcode == "sample_underscore" );
        REQUIRE( opcode.value == "dummy" );
        REQUIRE( opcode.parameter );
        REQUIRE( *opcode.parameter == 123 );
    }

    // TODO: I would say these are out of spec
    // SECTION("Badly parameterized opcode")
    // {
    //     SfzOpcode opcode { "sample12.3", "dummy"};
    //     REQUIRE( opcode.opcode == "sample12.3" );
    //     REQUIRE( opcode.value == "dummy" );
    //     REQUIRE( !opcode.parameter );
    // }

    // SECTION("Badly parameterized opcode with underscors")
    // {
    //     SfzOpcode opcode { "sample_underscore12.3", "dummy"};
    //     REQUIRE( opcode.opcode == "sample_underscore12.3" );
    //     REQUIRE( opcode.value == "dummy" );
    //     REQUIRE( !opcode.parameter );
    // }
}

TEST_CASE("Opcodes helpers", "Opcode tests")
{
    SECTION("Note values")
    {
        auto noteValue = readNoteValue("c-1");
        REQUIRE( noteValue );
        REQUIRE( *noteValue == 0);
        noteValue = readNoteValue("C-1");
        REQUIRE( noteValue );
        REQUIRE( *noteValue == 0);
        noteValue = readNoteValue("g9");
        REQUIRE( noteValue );
        REQUIRE( *noteValue == 127);
        noteValue = readNoteValue("G9");
        REQUIRE( noteValue );
        REQUIRE( *noteValue == 127);
        noteValue = readNoteValue("c#4");
        REQUIRE( noteValue );
        REQUIRE( *noteValue == 61);
        noteValue = readNoteValue("C#4");
        REQUIRE( noteValue );
        REQUIRE( *noteValue == 61);
    }
}
