#include "../JuceLibraryCode/JuceHeader.h"
#include "catch2/catch.hpp"
#include "../Source/SfzCCEnvelope.h"

TEST_CASE("CCEnvelope Tests", "CCEnvelope Tests")
{
    SECTION("Normal construction")
    {
        SfzCCEnvelope envelope { 128 };
        REQUIRE( envelope.getNextValue() == 0.0 );
        REQUIRE( envelope.getNextValue() == 0.0 );
    }

    SECTION("Standard targets")
    {
        SfzCCEnvelope envelope { 128 };
        envelope.addEvent(2, 127);
        envelope.addEvent(4, 0);
        REQUIRE( envelope.getNextValue() == 0.5 );
        REQUIRE( envelope.getNextValue() == 1.0 );
        REQUIRE( envelope.getNextValue() == 0.5 );
        REQUIRE( envelope.getNextValue() == 0.0 );
    }

    // SECTION("Blank value in between")
    // {
    //     SfzCCEnvelope envelope { 128 };
    //     envelope.addEvent(2, 63);
    //     REQUIRE( envelope.getNextValue() == Approx(0.25).epsilon(0.01) );
    //     REQUIRE( envelope.getNextValue() == Approx(0.5).epsilon(0.01) );
    //     REQUIRE( envelope.getNextValue() == Approx(0.5).epsilon(0.01) );
    //     envelope.addEvent(2, 127);
    //     REQUIRE( envelope.getNextValue() == Approx(0.75).epsilon(0.01) );
    //     REQUIRE( envelope.getNextValue() == 1.0 );
    // }

    SECTION("Different transformation")
    {
        SfzCCEnvelope envelope { 128 };
        envelope.setTransform( [] (float ccValue ) { return 2*ccValue + 1; } );
        envelope.addEvent(2, 127);
        envelope.addEvent(4, 0);
        REQUIRE( envelope.getNextValue() == 2.0 );
        REQUIRE( envelope.getNextValue() == 3.0 );
        REQUIRE( envelope.getNextValue() == 2.0 );
        REQUIRE( envelope.getNextValue() == 1.0 );
    }

    // TODO: update this one to make sense with the new behavior
    // SECTION("Reset to a different value")
    // {
    //     SfzCCEnvelope envelope { 128 };
    //     envelope.addEvent(2, 127);
    //     envelope.addEvent(4, 0);
    //     REQUIRE( envelope.getNextValue() == 0.5 );
    //     REQUIRE( envelope.getNextValue() == 1.0 );
    //     REQUIRE( envelope.getNextValue() == 0.5 );
    //     REQUIRE( envelope.getNextValue() == 0.0 );
    //     envelope.reset( 63 );
    //     envelope.addEvent(2, 127);
    //     envelope.addEvent(4, 0);
    //     REQUIRE( envelope.getNextValue() == Approx(0.75).epsilon(0.01) );
    //     REQUIRE( envelope.getNextValue() == 1.0 );
    //     REQUIRE( envelope.getNextValue() == 0.5 );
    //     REQUIRE( envelope.getNextValue() == 0.0 );
    // }

    // SECTION("Fill the CC buffer")
    // {
    //     SfzCCEnvelope envelope{ 4 };
    //     envelope.reset(0);
    //     envelope.addEvent(1, 127);
    //     envelope.addEvent(2, 0);
    //     envelope.addEvent(3, 127);
    //     REQUIRE(envelope.getNextValue() == 1.0);
    //     REQUIRE(envelope.getNextValue() == 0.0);
    //     REQUIRE(envelope.getNextValue() == 1.0);
    //     REQUIRE(envelope.getNextValue() == 1.0);
    //     envelope.addEvent(1, 127);
    //     envelope.addEvent(2, 0);
    //     envelope.addEvent(3, 127);
    //     REQUIRE(envelope.getNextValue() == 1.0);
    //     REQUIRE(envelope.getNextValue() == 0.0);
    //     REQUIRE(envelope.getNextValue() == 1.0);
    //     REQUIRE(envelope.getNextValue() == 1.0);
    // }
}