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

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "BinaryText.hpp"
#include "Utility.hpp"

int main(int argumentCount_, char** argumentArray_)
{
    Utility::Arguments arguments;

    {
        std::vector<std::string_view> argumentVector;

        for(int i(0); i < argumentCount_; ++i) {
            argumentVector.emplace_back(argumentArray_[i]);
        }

        try {
            arguments.ParseArguments(argumentVector);
        } catch(const Utility::Arguments::Error& error) {
            Utility::Exit(error.What(), -1);
        }
    }

    try {
        auto convert = [](const auto convertable_) -> auto {
            if constexpr(std::is_same_v<std::remove_const_t<decltype(convertable_)>, Utility::Arguments::Case>) {
                switch(convertable_) {
                    case Utility::Arguments::Case::LOWERCASE: return BinaryText::Base16::Case::LOWERCASE; break;
                    case Utility::Arguments::Case::MIXED: return BinaryText::Base16::Case::MIXED;
                    case Utility::Arguments::Case::UPPERCASE: return BinaryText::Base16::Case::UPPERCASE; break;
                    default: Utility::UnreachableTerminate();
                }
            } else if constexpr(std::is_same_v<std::remove_const_t<decltype(convertable_)>, Utility::Arguments::Padding>) {
                switch(convertable_) {
                    case Utility::Arguments::Padding::ENABLE_PADDING: return true;
                    case Utility::Arguments::Padding::DISABLE_PADDING: return false;
                    default: Utility::UnreachableTerminate();
                }
            } else if constexpr(std::is_same_v<std::remove_const_t<decltype(convertable_)>, Utility::Arguments::SpaceFolding>) {
                switch(convertable_) {
                    case Utility::Arguments::SpaceFolding::ENABLE_SPACE_FOLDING: return true;
                    case Utility::Arguments::SpaceFolding::DISABLE_SPACE_FOLDING: return false;
                    default: Utility::UnreachableTerminate();
                }
            } else if constexpr(std::is_same_v<std::remove_const_t<decltype(convertable_)>, Utility::Arguments::AdobeMode>) {
                switch(convertable_) {
                    case Utility::Arguments::AdobeMode::ENABLE_ADOBE_MODE: return true;
                    case Utility::Arguments::AdobeMode::DISABLE_ADOBE_MODE: return false;
                    default: Utility::UnreachableTerminate();
                }
            } else {
                static_assert(false);
            }
        };
        auto processTextOutput = [&arguments](const std::string& string_) -> void {
            if(arguments.HasOutputFilePath()) {
                Utility::WriteStringToFile(string_, arguments.GetOutputFilePath());
            } else {
                std::cout << string_ << std::endl;
            }
        };
        auto processBinaryOutput = [&arguments](const BinaryText::ByteBuffer<std::byte>& byteBuffer_) -> void {
            byteBuffer_.WriteToFile(arguments.GetOutputFilePath());
        };

        switch(arguments.GetTask()) {
            case Utility::Arguments::Task::ENCODE_TEXT: {
                switch(arguments.GetAlgorithm()) {
                    case Utility::Arguments::Algorithm::BASE_16: {
                        if(arguments.HasInputString()) {
                            processTextOutput(BinaryText::Base16::EncodeStringToString(arguments.GetInputString(), convert(arguments.GetCase())));
                        } else {
                            processTextOutput(BinaryText::Base16::EncodeStringToString(Utility::ReadStringFromFile(arguments.GetInputFilePath()),
                                                                                       convert(arguments.GetCase())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_32: {
                        if(arguments.HasInputString()) {
                            processTextOutput(BinaryText::Base32::EncodeStringToString(arguments.GetInputString(), convert(arguments.GetPadding())));
                        } else {
                            processTextOutput(BinaryText::Base32::EncodeStringToString(Utility::ReadStringFromFile(arguments.GetInputFilePath()),
                                                                                       convert(arguments.GetPadding())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_32_HEX: {
                        if(arguments.HasInputString()) {
                            processTextOutput(BinaryText::Base32Hex::EncodeStringToString(arguments.GetInputString(), convert(arguments.GetPadding())));
                        } else {
                            processTextOutput(BinaryText::Base32Hex::EncodeStringToString(Utility::ReadStringFromFile(arguments.GetInputFilePath()),
                                                                                          convert(arguments.GetPadding())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_64: {
                        if(arguments.HasInputString()) {
                            processTextOutput(BinaryText::Base64::EncodeStringToString(arguments.GetInputString(), convert(arguments.GetPadding())));
                        } else {
                            processTextOutput(BinaryText::Base64::EncodeStringToString(Utility::ReadStringFromFile(arguments.GetInputFilePath()),
                                                                                       convert(arguments.GetPadding())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_64_URL: {
                        if(arguments.HasInputString()) {
                            processTextOutput(BinaryText::Base64Url::EncodeStringToString(arguments.GetInputString(), convert(arguments.GetPadding())));
                        } else {
                            processTextOutput(BinaryText::Base64Url::EncodeStringToString(Utility::ReadStringFromFile(arguments.GetInputFilePath()),
                                                                                          convert(arguments.GetPadding())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::ASCII_85: {
                        if(arguments.HasInputString()) {
                            processTextOutput(BinaryText::Ascii85::EncodeStringToString(arguments.GetInputString(), convert(arguments.GetSpaceFolding()),
                                                                                        convert(arguments.GetAdobeMode())));
                        } else {
                            processTextOutput(BinaryText::Ascii85::EncodeStringToString(Utility::ReadStringFromFile(arguments.GetInputFilePath()),
                                                                                        convert(arguments.GetSpaceFolding()),
                                                                                        convert(arguments.GetAdobeMode())));
                        }

                        break;
                    }
                    default: Utility::UnreachableTerminate();
                }

                break;
            }
            case Utility::Arguments::Task::ENCODE_BINARY: {
                switch(arguments.GetAlgorithm()) {
                    case Utility::Arguments::Algorithm::BASE_16: {
                        processTextOutput(BinaryText::Base16::EncodeByteBufferToString(BinaryText::ByteBuffer<std::byte>(arguments.GetInputFilePath()),
                                                                                       convert(arguments.GetCase())));

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_32: {
                        processTextOutput(BinaryText::Base32::EncodeByteBufferToString(BinaryText::ByteBuffer<std::byte>(arguments.GetInputFilePath()),
                                                                                       convert(arguments.GetPadding())));

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_32_HEX: {
                        processTextOutput(BinaryText::Base32Hex::EncodeByteBufferToString(BinaryText::ByteBuffer<std::byte>(arguments.GetInputFilePath()),
                                                                                          convert(arguments.GetPadding())));

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_64: {
                        processTextOutput(BinaryText::Base64::EncodeByteBufferToString(BinaryText::ByteBuffer<std::byte>(arguments.GetInputFilePath()),
                                                                                       convert(arguments.GetPadding())));

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_64_URL: {
                        processTextOutput(BinaryText::Base64Url::EncodeByteBufferToString(BinaryText::ByteBuffer<std::byte>(arguments.GetInputFilePath()),
                                                                                          convert(arguments.GetPadding())));

                        break;
                    }
                    case Utility::Arguments::Algorithm::ASCII_85: {
                        processTextOutput(BinaryText::Ascii85::EncodeByteBufferToString(BinaryText::ByteBuffer<std::byte>(arguments.GetInputFilePath()),
                                                                                        convert(arguments.GetSpaceFolding()),
                                                                                        convert(arguments.GetAdobeMode())));

                        break;
                    }
                    default: Utility::UnreachableTerminate();
                }

                break;
            }
            case Utility::Arguments::Task::DECODE_TEXT: {
                switch(arguments.GetAlgorithm()) {
                    case Utility::Arguments::Algorithm::BASE_16: {
                        if(arguments.HasInputString()) {
                            processTextOutput(BinaryText::Base16::DecodeStringToString(arguments.GetInputString(), convert(arguments.GetCase())));
                        } else {
                            processTextOutput(BinaryText::Base16::DecodeStringToString(Utility::ReadStringFromFile(arguments.GetInputFilePath()),
                                                                                       convert(arguments.GetCase())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_32: {
                        if(arguments.HasInputString()) {
                            processTextOutput(BinaryText::Base32::DecodeStringToString(arguments.GetInputString()));
                        } else {
                            processTextOutput(BinaryText::Base32::DecodeStringToString(Utility::ReadStringFromFile(arguments.GetInputFilePath())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_32_HEX: {
                        if(arguments.HasInputString()) {
                            processTextOutput(BinaryText::Base32Hex::DecodeStringToString(arguments.GetInputString()));
                        } else {
                            processTextOutput(BinaryText::Base32Hex::DecodeStringToString(Utility::ReadStringFromFile(arguments.GetInputFilePath())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_64: {
                        if(arguments.HasInputString()) {
                            processTextOutput(BinaryText::Base64::DecodeStringToString(arguments.GetInputString()));
                        } else {
                            processTextOutput(BinaryText::Base64::DecodeStringToString(Utility::ReadStringFromFile(arguments.GetInputFilePath())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_64_URL: {
                        if(arguments.HasInputString()) {
                            processTextOutput(BinaryText::Base64Url::DecodeStringToString(arguments.GetInputString()));
                        } else {
                            processTextOutput(BinaryText::Base64Url::DecodeStringToString(Utility::ReadStringFromFile(arguments.GetInputFilePath())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::ASCII_85: {
                        if(arguments.HasInputString()) {
                            processTextOutput(BinaryText::Ascii85::DecodeStringToString(arguments.GetInputString(), convert(arguments.GetSpaceFolding()),
                                                                                        convert(arguments.GetAdobeMode())));
                        } else {
                            processTextOutput(BinaryText::Ascii85::DecodeStringToString(Utility::ReadStringFromFile(arguments.GetInputFilePath()),
                                                                                        convert(arguments.GetSpaceFolding()),
                                                                                        convert(arguments.GetAdobeMode())));
                        }

                        break;
                    }
                    default: Utility::UnreachableTerminate();
                }

                break;
            }
            case Utility::Arguments::Task::DECODE_BINARY: {
                switch(arguments.GetAlgorithm()) {
                    case Utility::Arguments::Algorithm::BASE_16: {
                        if(arguments.HasInputString()) {
                            processBinaryOutput(
                                BinaryText::Base16::DecodeStringToByteBuffer<std::byte>(arguments.GetInputString(), convert(arguments.GetCase())));
                        } else {
                            processBinaryOutput(BinaryText::Base16::DecodeStringToByteBuffer<std::byte>(
                                Utility::ReadStringFromFile(arguments.GetInputFilePath()), convert(arguments.GetCase())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_32: {
                        if(arguments.HasInputString()) {
                            processBinaryOutput(BinaryText::Base32::DecodeStringToByteBuffer<std::byte>(arguments.GetInputString()));
                        } else {
                            processBinaryOutput(
                                BinaryText::Base32::DecodeStringToByteBuffer<std::byte>(Utility::ReadStringFromFile(arguments.GetInputFilePath())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_32_HEX: {
                        if(arguments.HasInputString()) {
                            processBinaryOutput(BinaryText::Base32Hex::DecodeStringToByteBuffer<std::byte>(arguments.GetInputString()));
                        } else {
                            processBinaryOutput(
                                BinaryText::Base32Hex::DecodeStringToByteBuffer<std::byte>(Utility::ReadStringFromFile(arguments.GetInputFilePath())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_64: {
                        if(arguments.HasInputString()) {
                            processBinaryOutput(BinaryText::Base64::DecodeStringToByteBuffer<std::byte>(arguments.GetInputString()));
                        } else {
                            processBinaryOutput(
                                BinaryText::Base64::DecodeStringToByteBuffer<std::byte>(Utility::ReadStringFromFile(arguments.GetInputFilePath())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::BASE_64_URL: {
                        if(arguments.HasInputString()) {
                            processBinaryOutput(BinaryText::Base64Url::DecodeStringToByteBuffer<std::byte>(arguments.GetInputString()));
                        } else {
                            processBinaryOutput(
                                BinaryText::Base64Url::DecodeStringToByteBuffer<std::byte>(Utility::ReadStringFromFile(arguments.GetInputFilePath())));
                        }

                        break;
                    }
                    case Utility::Arguments::Algorithm::ASCII_85: {
                        if(arguments.HasInputString()) {
                            processBinaryOutput(BinaryText::Ascii85::DecodeStringToByteBuffer<std::byte>(
                                arguments.GetInputString(), convert(arguments.GetSpaceFolding()), convert(arguments.GetAdobeMode())));
                        } else {
                            processBinaryOutput(BinaryText::Ascii85::DecodeStringToByteBuffer<std::byte>(
                                Utility::ReadStringFromFile(arguments.GetInputFilePath()), convert(arguments.GetSpaceFolding()),
                                convert(arguments.GetAdobeMode())));
                        }

                        break;
                    }
                    default: Utility::UnreachableTerminate();
                }

                break;
            }
            default: Utility::UnreachableTerminate();
        }
    } catch(const BinaryText::Base16::Error& error) {
        Utility::Exit(error.What(), -1);
    } catch(const BinaryText::Base32::Error& error) {
        Utility::Exit(error.What(), -1);
    } catch(const BinaryText::Base32Hex::Error& error) {
        Utility::Exit(error.What(), -1);
    } catch(const BinaryText::Base64::Error& error) {
        Utility::Exit(error.What(), -1);
    } catch(const BinaryText::Base64Url::Error& error) {
        Utility::Exit(error.What(), -1);
    } catch(const BinaryText::Ascii85::Error& error) {
        Utility::Exit(error.What(), -1);
    } catch(const BinaryText::ByteBuffer<std::byte>::Error& error) {
        Utility::Exit(error.What(), -1);
    } catch(const Utility::Error& error) {
        Utility::Exit(error.What(), -1);
    }

    return 0;
}