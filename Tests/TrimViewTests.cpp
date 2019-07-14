#include "../JuceLibraryCode/JuceHeader.h"
#include "catch2/catch.hpp"
#include "../Source/SfzGlobals.h"
#include <string_view>
using namespace Catch::literals;
using namespace  std::literals::string_view_literals;

TEST_CASE("Trim View test", "Trim view test")
{
    SECTION("Trim spaces")
    {
        auto input { "   view  "sv };
        trimView(input);
        REQUIRE( input == "view"sv );
    }
    SECTION("Empty view")
    {
        auto input { "     "sv };
        trimView(input);
        REQUIRE( input.empty() );
    }
}