/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
*/

#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <string_view>

#include "Utility.hpp"

namespace Utility
{
    void Arguments::ParseArguments(const std::vector<std::string_view>& argumentVector_)
    {
        Reset();

        if(argumentVector_.size() >= 2) {
            for(std::vector<std::string_view>::const_iterator iter(std::next(argumentVector_.cbegin(), 1)); iter != argumentVector_.cend(); ++iter) {
                if(*iter == "-h" or *iter == "--help") {
                    Exit("binarytext [ARGUMENTS]\n\n"
                         "The following are the only command-line arguments that can be passed to this application:\n"
                         "  -h / --help\n"
                         "  --encode-text\n"
                         "  --encode-binary\n"
                         "  --decode-text\n"
                         "  --decode-binary\n"
                         "  --input-string=OPTION\n"
                         "  --input-file=OPTION\n"
                         "  --output-file=OPTION\n"
                         "  --algorithm=OPTION (base16, base32, base32hex, base64, base64url, ascii85)\n\n"
                         "Base16 only:\n"
                         "  --case=OPTION (lowercase, mixed, uppercase)\n\n"
                         "Base32, Base32Hex, Base64 and Base64Url only (--encode-text and --encode-binary only):\n"
                         "  --without-padding\n\n"
                         "Ascii85 only:\n"
                         "  --fold-spaces\n"
                         "  --adobe-mode",
                         0);
                } else if((*iter == "--encode-text")) {
                    switch(_task) {
                        case Task::NONE: _task = Task::ENCODE_TEXT; break;
                        case Task::ENCODE_TEXT: throw Error("Conflicting arguments: \"--encode-text\"");
                        case Task::ENCODE_BINARY: throw Error("Conflicting arguments: \"--encode-text\" and \"--encode-binary\"");
                        case Task::DECODE_TEXT: throw Error("Conflicting arguments: \"--encode-text\" and \"--decode-text\"");
                        case Task::DECODE_BINARY: throw Error("Conflicting arguments: \"--encode-text\" and \"--decode-binary\"");
                    }
                } else if(*iter == "--encode-binary") {
                    switch(_task) {
                        case Task::NONE: _task = Task::ENCODE_BINARY; break;
                        case Task::ENCODE_TEXT: throw Error("Conflicting arguments: \"--encode-binary\" and \"--encode-text\"");
                        case Task::ENCODE_BINARY: throw Error("Conflicting arguments: \"--encode-binary\"");
                        case Task::DECODE_TEXT: throw Error("Conflicting arguments: \"--encode-binary\" and \"--decode-text\"");
                        case Task::DECODE_BINARY: throw Error("Conflicting arguments: \"--encode-binary\" and \"--decode-binary\"");
                    }
                } else if(*iter == "--decode-text") {
                    switch(_task) {
                        case Task::NONE: _task = Task::DECODE_TEXT; break;
                        case Task::ENCODE_TEXT: throw Error("Conflicting arguments: \"--decode-text\" and \"--encode-text\"");
                        case Task::ENCODE_BINARY: throw Error("Conflicting arguments: \"--decode-text\" and \"--encode-binary\"");
                        case Task::DECODE_TEXT: throw Error("Conflicting arguments: \"--decode-text\"");
                        case Task::DECODE_BINARY: throw Error("Conflicting arguments: \"--decode-text\" and \"--decode-binary\"");
                    }
                } else if(*iter == "--decode-binary") {
                    switch(_task) {
                        case Task::NONE: _task = Task::DECODE_BINARY; break;
                        case Task::ENCODE_TEXT: throw Error("Conflicting arguments: \"--decode-binary\" and \"--encode-text\"");
                        case Task::ENCODE_BINARY: throw Error("Conflicting arguments: \"--decode-binary\" and \"--encode-binary\"");
                        case Task::DECODE_TEXT: throw Error("Conflicting arguments: \"--decode-binary\" and \"--decode-text\"");
                        case Task::DECODE_BINARY: throw Error("Conflicting arguments: \"--decode-binary\"");
                    }
                } else if(*iter == "--without-padding") {
                    if(_padding == Padding::NONE) {
                        _padding = Padding::DISABLE_PADDING;
                    } else {
                        throw Error("Conflicting arguments: \"--without-padding\"");
                    }
                } else if(*iter == "--fold-spaces") {
                    if(_spaceFolding == SpaceFolding::NONE) {
                        _spaceFolding = SpaceFolding::ENABLE_SPACE_FOLDING;
                    } else {
                        throw Error("Conflicting arguments: \"--fold-spaces\"");
                    }
                } else if(*iter == "--adobe-mode") {
                    if(_adobeMode == AdobeMode::NONE) {
                        _adobeMode = AdobeMode::ENABLE_ADOBE_MODE;
                    } else {
                        throw Error("Conflicting arguments: \"--adobe-mode\"");
                    }
                } else if(std::string_view argument("--input-string="); iter->find(argument) == 0) {
                    if(_inputString.empty()) {
                        _inputString = iter->substr(argument.size(), iter->size());

                        if(_inputString.empty()) {
                            throw Error("Empty input string");
                        }
                    } else {
                        throw Error("Conflicting arguments: \"--input-string=OPTION\"");
                    }
                } else if(argument = "--input-file="; iter->find(argument) == 0) {
                    if(_inputFilePath.empty()) {
                        _inputFilePath = iter->substr(argument.size(), iter->size());

                        if(_inputFilePath.empty()) {
                            throw Error("Empty input file path");
                        }
                    } else {
                        throw Error("Conflicting arguments: \"--input-file=OPTION\"");
                    }
                } else if(argument = "--output-file="; iter->find(argument) == 0) {
                    if(_outputFilePath.empty()) {
                        _outputFilePath = iter->substr(argument.size(), iter->size());

                        if(_outputFilePath.empty()) {
                            throw Error("Empty output file path");
                        }
                    } else {
                        throw Error("Conflicting arguments: \"--output-file=OPTION\"");
                    }
                } else if(argument = "--algorithm="; iter->find(argument) == 0) {
                    if(_algorithm == Algorithm::NONE) {
                        const std::string_view algorithmOption(std::next(iter->cbegin(), argument.size()), iter->cend());

                        if(algorithmOption == "base16") {
                            _algorithm = Algorithm::BASE_16;
                        } else if(algorithmOption == "base32") {
                            _algorithm = Algorithm::BASE_32;
                        } else if(algorithmOption == "base32hex") {
                            _algorithm = Algorithm::BASE_32_HEX;
                        } else if(algorithmOption == "base64") {
                            _algorithm = Algorithm::BASE_64;
                        } else if(algorithmOption == "base64url") {
                            _algorithm = Algorithm::BASE_64_URL;
                        } else if(algorithmOption == "ascii85") {
                            _algorithm = Algorithm::ASCII_85;
                        } else {
                            throw Error(std::format("Invalid algorithm : \"{}\"", algorithmOption));
                        }
                    } else {
                        throw Error("Conflicting arguments: \"--algorithm=OPTION\"");
                    }
                } else if(argument = "--case="; iter->find(argument) == 0) {
                    if(_case == Case::NONE) {
                        const std::string_view caseOption(std::next(iter->cbegin(), argument.size()), iter->cend());

                        if(caseOption == "lowercase") {
                            _case = Case::LOWERCASE;
                        } else if(caseOption == "mixed") {
                            _case = Case::MIXED;
                        } else if(caseOption == "uppercase") {
                            _case = Case::UPPERCASE;
                        } else {
                            throw Error(std::format("Invalid case: \"{}\"", caseOption));
                        }
                    } else {
                        throw Error("Conflicting arguments: \"--case=OPTION\"");
                    }
                } else {
                    throw Error(std::format("Invalid argument: \"{}\"", *iter));
                }
            }

            if(_task == Task::NONE) {
                throw Error("No \"--encode-text\", \"--encode-binary\", \"--decode-text\" or \"--decode-binary\" argument provided");
            } else if(_task == Task::DECODE_BINARY) {
                if(_outputFilePath.empty()) {
                    throw Error("No \"--output-file=OPTION\" argument provided");
                }
            }

            if(not _inputString.empty() and not _inputFilePath.empty()) {
                throw Error("Conflicting arguments: \"--input-string=OPTION\" and \"--input-file=OPTION\"");
            }

            if(_inputString.empty() and _inputFilePath.empty()) {
                throw Error("No \"--input-string=OPTION\" or \"--input-file=OPTION\" argument provided");
            }

            switch(_algorithm) {
                case Algorithm::NONE: _algorithm = Algorithm::BASE_16; [[fallthrough]];
                case Algorithm::BASE_16: {
                    switch(_task) {
                        case Task::ENCODE_TEXT: {
                            if(_case == Case::MIXED) {
                                throw Error("Conflicting arguments: \"--case=mixed\" and \"--encode-text\"");
                            } else if(_case == Case::NONE) {
                                _case = Case::UPPERCASE;
                            }

                            break;
                        }
                        case Task::ENCODE_BINARY: {
                            if(_case == Case::MIXED) {
                                throw Error("Conflicting arguments: \"--case=mixed\" and \"--encode-binary\"");
                            } else if(_case == Case::NONE) {
                                _case = Case::UPPERCASE;
                            }

                            break;
                        }
                        case Task::DECODE_TEXT: _case = (_case == Case::NONE) ? Case::MIXED : _case; break;
                        case Task::DECODE_BINARY: _case = (_case == Case::NONE) ? Case::MIXED : _case; break;
                        default: UnreachableTerminate();
                    }

                    if(_padding != Padding::NONE) throw Error("Conflicting arguments: \"--without-padding\" and \"--algorithm=base16\"");
                    if(_spaceFolding != SpaceFolding::NONE) throw Error("Conflicting arguments: \"--fold-spaces\" and \"--algorithm=base16\"");
                    if(_adobeMode != AdobeMode::NONE) throw Error("Conflicting arguments: \"--adobe-mode\" and \"--algorithm=base16\"");

                    break;
                }
                case Algorithm::BASE_32: {
                    switch(_task) {
                        case Task::ENCODE_TEXT: _padding = (_padding == Padding::NONE) ? Padding::ENABLE_PADDING : _padding; break;
                        case Task::ENCODE_BINARY: _padding = (_padding == Padding::NONE) ? Padding::ENABLE_PADDING : _padding; break;
                        case Task::DECODE_TEXT: {
                            if(_padding != Padding::NONE) throw Error("Conflicting arguments: \"--without-padding\" and \"--decode-text\"");

                            break;
                        }
                        case Task::DECODE_BINARY: {
                            if(_padding != Padding::NONE) throw Error("Conflicting arguments: \"--without-padding\" and \"--decode-binary\"");

                            break;
                        }
                        default: UnreachableTerminate();
                    }

                    if(_case != Case::NONE) throw Error("Conflicting arguments: \"--case=OPTION\" and \"--algorithm=base32\"");
                    if(_spaceFolding != SpaceFolding::NONE) throw Error("Conflicting arguments: \"--fold-spaces\" and \"--algorithm=base32\"");
                    if(_adobeMode != AdobeMode::NONE) throw Error("Conflicting arguments: \"--adobe-mode\" and \"--algorithm=base32\"");

                    break;
                }
                case Algorithm::BASE_32_HEX: {
                    switch(_task) {
                        case Task::ENCODE_TEXT: _padding = (_padding == Padding::NONE) ? Padding::ENABLE_PADDING : _padding; break;
                        case Task::ENCODE_BINARY: _padding = (_padding == Padding::NONE) ? Padding::ENABLE_PADDING : _padding; break;
                        case Task::DECODE_TEXT: {
                            if(_padding != Padding::NONE) throw Error("Conflicting arguments: \"--without-padding\" and \"--decode-text\"");

                            break;
                        }
                        case Task::DECODE_BINARY: {
                            if(_padding != Padding::NONE) throw Error("Conflicting arguments: \"--without-padding\" and \"--decode-binary\"");

                            break;
                        }
                        default: UnreachableTerminate();
                    }

                    if(_case != Case::NONE) throw Error("Conflicting arguments: \"--case=OPTION\" and \"--algorithm=base32hex\"");
                    if(_spaceFolding != SpaceFolding::NONE) throw Error("Conflicting arguments: \"--fold-spaces\" and \"--algorithm=base32hex\"");
                    if(_adobeMode != AdobeMode::NONE) throw Error("Conflicting arguments: \"--adobe-mode\" and \"--algorithm=base32hex\"");

                    break;
                }
                case Algorithm::BASE_64: {
                    switch(_task) {
                        case Task::ENCODE_TEXT: _padding = (_padding == Padding::NONE) ? Padding::ENABLE_PADDING : _padding; break;
                        case Task::ENCODE_BINARY: _padding = (_padding == Padding::NONE) ? Padding::ENABLE_PADDING : _padding; break;
                        case Task::DECODE_TEXT: {
                            if(_padding != Padding::NONE) throw Error("Conflicting arguments: \"--without-padding\" and \"--decode-text\"");

                            break;
                        }
                        case Task::DECODE_BINARY: {
                            if(_padding != Padding::NONE) throw Error("Conflicting arguments: \"--without-padding\" and \"--decode-binary\"");

                            break;
                        }
                        default: UnreachableTerminate();
                    }

                    if(_case != Case::NONE) throw Error("Conflicting arguments: \"--case=OPTION\" and \"--algorithm=base64\"");
                    if(_spaceFolding != SpaceFolding::NONE) throw Error("Conflicting arguments: \"--fold-spaces\" and \"--algorithm=base64\"");
                    if(_adobeMode != AdobeMode::NONE) throw Error("Conflicting arguments: \"--adobe-mode\" and \"--algorithm=base64\"");

                    break;
                }
                case Algorithm::BASE_64_URL: {
                    switch(_task) {
                        case Task::ENCODE_TEXT: _padding = (_padding == Padding::NONE) ? Padding::ENABLE_PADDING : _padding; break;
                        case Task::ENCODE_BINARY: _padding = (_padding == Padding::NONE) ? Padding::ENABLE_PADDING : _padding; break;
                        case Task::DECODE_TEXT: {
                            if(_padding != Padding::NONE) throw Error("Conflicting arguments: \"--without-padding\" and \"--decode-text\"");

                            break;
                        }
                        case Task::DECODE_BINARY: {
                            if(_padding != Padding::NONE) throw Error("Conflicting arguments: \"--without-padding\" and \"--decode-binary\"");

                            break;
                        }
                        default: UnreachableTerminate();
                    }

                    if(_case != Case::NONE) throw Error("Conflicting arguments: \"--case=OPTION\" and \"--algorithm=base64url\"");
                    if(_spaceFolding != SpaceFolding::NONE) throw Error("Conflicting arguments: \"--fold-spaces\" and \"--algorithm=base64url\"");
                    if(_adobeMode != AdobeMode::NONE) throw Error("Conflicting arguments: \"--adobe-mode\" and \"--algorithm=base64url\"");

                    break;
                }
                case Algorithm::ASCII_85: {
                    _spaceFolding = (_spaceFolding == SpaceFolding::NONE) ? SpaceFolding::DISABLE_SPACE_FOLDING : _spaceFolding;
                    _adobeMode = (_adobeMode == AdobeMode::NONE) ? AdobeMode::DISABLE_ADOBE_MODE : _adobeMode;

                    if(_case != Case::NONE) throw Error("Conflicting arguments: \"--case=OPTION\" and \"--algorithm=ascii85\"");
                    if(_padding != Padding::NONE) throw Error("Conflicting arguments: \"--without-padding\" and \"--algorithm=ascii85\"");

                    break;
                }
            }
        } else {
            throw Error("Not enough arguments");
        }
    }

    void Arguments::Reset()
    {
        _task = Task::NONE;
        _algorithm = Algorithm::NONE;
        _case = Case::NONE;
        _padding = Padding::NONE;
        _spaceFolding = SpaceFolding::NONE;
        _adobeMode = AdobeMode::NONE;

        _inputString.clear();
        _inputFilePath.clear();
        _outputFilePath.clear();
    }

    [[noreturn]] void UnreachableTerminate(const std::source_location sourceLocation_) noexcept
    {
        std::cerr << std::format("Something that should not have gone wrong went wrong and this function was called to terminate the program. This function "
                                 "was called inside \"{}\" at function \"{}\", line {}, column {}",
                                 sourceLocation_.file_name(), sourceLocation_.function_name(), sourceLocation_.line(), sourceLocation_.column())
                  << std::endl;
        std::terminate();
    }

    [[noreturn]] void Exit(const std::string& exitMessage_, const int exitCode_) noexcept
    {
        if(exitCode_ == 0) {
            std::cout << exitMessage_ << std::endl;
        } else {
            std::cerr << exitMessage_ << std::endl;
        }

        std::exit(exitCode_);
    }

    void WriteStringToFile(const std::string& string_, const std::filesystem::path& filePath_)
    {
        std::ofstream fileStream(filePath_, std::ofstream::out | std::ofstream::trunc);

        if(not fileStream.is_open()) {
            throw Error("Failed to open file");
        }

        fileStream << string_;

        if(fileStream.fail()) {
            throw Error("Failed to write to file");
        }
    }

    std::string ReadStringFromFile(const std::filesystem::path& filePath_)
    {
        std::ifstream fileStream(filePath_, std::ifstream::in);
        std::string fileString;

        if(not fileStream.is_open()) {
            throw Error("Failed to open file");
        }

        fileStream >> fileString;

        if(fileStream.fail() and not fileStream.eof()) {
            throw Error("Failed to read from file");
        }

        return fileString;
    }
}