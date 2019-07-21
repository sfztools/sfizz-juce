#include "../JuceLibraryCode/JuceHeader.h"
#include "catch2/catch.hpp"
#include "../Source/SfzBlockEnvelope.h"

TEST_CASE("BlockEnvelope Tests", "BlockEnvelope Tests")
{
    SECTION("Default value")
    {
        constexpr int numElements { 4 };
        SfzBlockEnvelope envelope { numElements };
        std::array<float, numElements> output;
        std::array<float, numElements> expected { 0.0f, 0.0f, 0.0f, 0.0f };
        envelope.getEnvelope(output.data(), numElements);
        REQUIRE( output == expected );
    }

    SECTION("Default value not zero")
    {
        constexpr int numElements { 4 };
        SfzBlockEnvelope<float> envelope { numElements, 1.0f };
        std::array<float, numElements> output;
        std::array<float, numElements> expected { 1.0f, 1.0f, 1.0f, 1.0f };
        envelope.getEnvelope(output.data(), numElements);
        REQUIRE( output == expected );
    }

    SECTION("Basic ramp")
    {
        constexpr int numElements { 8 };
        SfzBlockEnvelope<float> envelope { numElements, 0.0f };
        std::array<float, numElements> output;
        envelope.addEvent(1, 2);
        envelope.getEnvelope(output.data(), numElements);
        std::array<float, numElements> expected { 0.0f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
        REQUIRE( output == expected );
    }

    SECTION("Less basic ramp")
    {
        constexpr int numElements { 8 };
        SfzBlockEnvelope<float> envelope { numElements, 0.0f };
        std::array<float, numElements> output;
        envelope.addEvent(1, 2);
        envelope.addEvent(2, 4);
        envelope.getEnvelope(output.data(), numElements);
        std::array<float, numElements> expected { 0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 2.0f, 2.0f, 2.0f };
        REQUIRE( output == expected );
    }

    SECTION("No new event after a ramp")
    {
        constexpr int numElements { 4 };
        SfzBlockEnvelope<float> envelope { numElements, 0.0f };
        std::array<float, numElements> output;
        envelope.addEvent(1, 2);
        envelope.getEnvelope(output.data(), numElements);
        std::array<float, numElements> expected { 0.0f, 0.5f, 1.0f, 1.0f };
        REQUIRE( output == expected );
        envelope.getEnvelope(output.data(), numElements);
        std::array<float, numElements> expected2 { 1.0f, 1.0f, 1.0f, 1.0f };
        REQUIRE( output == expected2 );
    }

    SECTION("Too many events")
    {
        constexpr int numElements { 4 };
        SfzBlockEnvelope<float> envelope { numElements, 0.0f };
        std::array<float, numElements> output;
        envelope.addEvent(1, 1);
        envelope.addEvent(2, 2);
        envelope.addEvent(3, 3);
        envelope.addEvent(4, 4);
        envelope.getEnvelope(output.data(), numElements);
        std::array<float, numElements> expected { 0.0f, 1.0f, 2.0f, 3.0f };
        REQUIRE( output == expected );
        envelope.getEnvelope(output.data(), numElements);
        std::array<float, numElements> expected2 { 3.0f, 3.0f, 3.0f, 3.0f };
        REQUIRE( output == expected2 );
    }

    SECTION("Transform")
    {
        constexpr int numElements { 4 };
        SfzBlockEnvelope<float> envelope { numElements, 0.0f };
        std::array<float, numElements> output;
        envelope.addEvent(1, 2);
        envelope.setFunction([](auto in){ return 2.0f * in; });
        envelope.getEnvelope(output.data(), numElements);
        std::array<float, numElements> expected { 0.0f, 1.0f, 2.0f, 2.0f };
        REQUIRE( output == expected );
    }
}