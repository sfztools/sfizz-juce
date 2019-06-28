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

TEST_CASE("Header Regex", "Regex Tests")
{
    SECTION("Basic header match")
    {
        std::smatch headerMatch;
        std::string line{"<header>param1=value1 param2=value2<next>"};
        auto found = std::regex_search(line, headerMatch, SfzRegexes::headers);
        REQUIRE(found);
        REQUIRE(headerMatch[1] == "header");
        REQUIRE(headerMatch[2] == "param1=value1 param2=value2");
    }
    SECTION("EOL header match")
    {
        std::smatch headerMatch;
        std::string line{"<header>param1=value1 param2=value2"};
        auto found = std::regex_search(line, headerMatch, SfzRegexes::headers);
        REQUIRE(found);
        REQUIRE(headerMatch[1] == "header");
        REQUIRE(headerMatch[2] == "param1=value1 param2=value2");
    }
}

void memberTest(const std::string &line, const std::string &variable, const std::string &value)
{
    std::smatch memberMatch;
    auto found = std::regex_search(line, memberMatch, SfzRegexes::members);
    REQUIRE(found);
    REQUIRE(memberMatch[1] == variable);
    REQUIRE(memberMatch[2] == value);
}

TEST_CASE("Member Regex", "Regex Tests")
{
    memberTest("param=value", "param", "value");
    memberTest("param=113", "param", "113");
    memberTest("param1=value", "param1", "value");
    memberTest("param_1=value", "param_1", "value");
    memberTest("param_1=value", "param_1", "value");
    memberTest("ampeg_sustain_oncc74=-100", "ampeg_sustain_oncc74", "-100");
    memberTest("lorand=0.750", "lorand", "0.750");
    memberTest("sample=value", "sample", "value");
    memberTest("sample=value-()*", "sample", "value-()*");
    memberTest("sample=../sample.wav", "sample", "../sample.wav");
    memberTest("sample=..\\sample.wav", "sample", "..\\sample.wav");
    memberTest("sample=subdir\\subdir\\sample.wav", "sample", "subdir\\subdir\\sample.wav");
    memberTest("sample=subdir/subdir/sample.wav", "sample", "subdir/subdir/sample.wav");
    memberTest("sample=subdir_underscore\\sample.wav", "sample", "subdir_underscore\\sample.wav");
    memberTest("sample=subdir space\\sample.wav", "sample", "subdir space\\sample.wav");
    memberTest("sample=subdir space\\sample.wav next_member=value", "sample", "subdir space\\sample.wav");
    memberTest("sample=..\\Samples\\pizz\\a0_vl3_rr3.wav", "sample", "..\\Samples\\pizz\\a0_vl3_rr3.wav");
    memberTest("sample=..\\Samples\\SMD Cymbals Stereo (Samples)\\Hi-Hat (Samples)\\01 Hat Tight 1\\RR1\\09_Hat_Tight_Cnt_RR1.wav", "sample", "..\\Samples\\SMD Cymbals Stereo (Samples)\\Hi-Hat (Samples)\\01 Hat Tight 1\\RR1\\09_Hat_Tight_Cnt_RR1.wav");
}

void parameterTest(const std::string& line, const std::string& opcode, const std::string& parameter)
{
    std::smatch parameterMatch;
    auto found = std::regex_search(line, parameterMatch, SfzRegexes::opcodeParameters);
    REQUIRE(found);
    REQUIRE(parameterMatch[1] == opcode);
    REQUIRE(parameterMatch[2] == parameter);
}

void parameterFail(const std::string &line)
{
    std::smatch parameterMatch;
    auto found = std::regex_search(line, parameterMatch, SfzRegexes::opcodeParameters);
    REQUIRE(!found);
}

TEST_CASE("Opcode parameter Regex", "Regex Tests")
{
    parameterTest("opcode_123", "opcode_", "123");
    parameterTest("xfin_locc1", "xfin_locc", "1");
    parameterTest("ampeg_hold_oncc24", "ampeg_hold_oncc", "24");
    parameterTest("lfo02_phase_oncc135", "lfo02_phase_oncc", "135");
    parameterFail("lfo01_freq");
    parameterFail("ampeg_sustain");
}