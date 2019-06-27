#include "../JuceLibraryCode/JuceHeader.h"
#include "catch2/catch.hpp"
#include "../Source/SfzGlobals.h"
using namespace Catch::literals;

void includeTest(const std::string& line, const std::string& fileName)
{
    std::smatch includeMatch;
    auto found = std::regex_search(line, includeMatch, SfzRegexes::includes);
    REQUIRE(found);
    REQUIRE(includeMatch[1] == fileName);
}

TEST_CASE("#include Regex", "Regex Tests")
{
    includeTest("#include \"file.sfz\"", "file.sfz");
    includeTest("#include \"../Programs/file.sfz\"", "../Programs/file.sfz");
    includeTest("#include \"..\\Programs\\file.sfz\"", "..\\Programs\\file.sfz");
    includeTest("#include \"file-1.sfz\"", "file-1.sfz");
    includeTest("#include \"file~1.sfz\"", "file~1.sfz");
    includeTest("#include \"file_1.sfz\"", "file_1.sfz");
    includeTest("#include \"file$1.sfz\"", "file$1.sfz");
    includeTest("#include \"file,1.sfz\"", "file,1.sfz");
    includeTest("#include \"rubbishCharactersAfter.sfz\" blabldaljf///df", "rubbishCharactersAfter.sfz");
    includeTest("#include \"lazyMatching.sfz\" b\"", "lazyMatching.sfz");
}

void defineTest(const std::string& line, const std::string& variable, const std::string& value)
{
    std::smatch defineMatch;
    auto found = std::regex_search(line, defineMatch, SfzRegexes::defines);
    REQUIRE(found);
    REQUIRE(defineMatch[1] == variable);
    REQUIRE(defineMatch[2] == value);
}

void defineFails(const std::string& line)
{
    std::smatch defineMatch;
    auto found = std::regex_search(line, defineMatch, SfzRegexes::defines);
    std::cout << line << '\n';
    REQUIRE(!found);
}

TEST_CASE("#define Regex", "Regex Tests")
{
    defineTest("#define $number 1", "$number", "1");
    defineTest("#define $letters QWERasdf", "$letters", "QWERasdf");
    defineTest("#define $alphanum asr1t44", "$alphanum", "asr1t44");
    defineTest("#define  $whitespace   asr1t44   ", "$whitespace", "asr1t44");
    defineTest("#define $lazyMatching  matched  bfasd ", "$lazyMatching", "matched");
    defineFails("#define $symbols# 1");
    defineFails("#define $symbolsAgain $1");
    defineFails("#define $trailingSymbols 1$");
}