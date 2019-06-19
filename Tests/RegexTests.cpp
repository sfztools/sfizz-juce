#include "../JuceLibraryCode/JuceHeader.h"
#include "catch2/catch.hpp"
#include "../Source/SfzGlobals.h"
using namespace Catch::literals;

TEST_CASE("Include Regex", "Regex Tests")
{
    auto filenameTest = [](const std::string& fileName){
        SECTION(fileName)
        {
            std::smatch includeMatch;
            std::string testString { "#include \"" + fileName + "\"" };
            auto found = std::regex_search(testString, includeMatch, SfzRegexes::includes);
            REQUIRE(found);
            REQUIRE(includeMatch[1] == fileName);
        }
    };

    filenameTest("file.sfz");
    filenameTest("../Programs/file.sfz");
    filenameTest("..\\Programs\\file.sfz");
    filenameTest("file-1.sfz");
    filenameTest("file~1.sfz");
    filenameTest("file_1.sfz");
    filenameTest("file$1.sfz");
    filenameTest("file,1.sfz");

    SECTION("Whitespace and rubbish")
    {
        std::smatch includeMatch;
        std::string fileName { "file.sfz" };
        std::string testString { "#include \"" + fileName + "\" blabldaljf///df" };
        auto found = std::regex_search(testString, includeMatch, SfzRegexes::includes);
        REQUIRE(found);
        REQUIRE(includeMatch[1] == fileName);
    }

    SECTION("Check for lazy matching")
    {
        std::smatch includeMatch;
        std::string fileName { "file.sfz" };
        std::string testString { "#include \"" + fileName + "\" b\"" };
        auto found = std::regex_search(testString, includeMatch, SfzRegexes::includes);
        REQUIRE(found);
        REQUIRE(includeMatch[1] == fileName);
    }
}