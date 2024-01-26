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

#pragma once

#include <exception>
#include <filesystem>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

/// @brief A namespace that has utilities used by the test program.
namespace Utility
{
    /// @brief A class that holds data about the command-line arguments passed to the test program.
    class Arguments
    {
    public:
        /// @brief What to do.
        enum class Task
        {
            NONE,          ///< Empty placeholder.
            ENCODE_TEXT,   ///< Encodes in text mode (`--encode-text`).
            ENCODE_BINARY, ///< Encodes in binary mode (`--encode-binary`).
            DECODE_TEXT,   ///< Decodes in text mode (`--decode-text`).
            DECODE_BINARY  ///< Decodes in binary mode (`--decode-binary`).
        };

        /// @brief Algorithm to use.
        enum class Algorithm
        {
            NONE,        ///< Empty placeholder.
            BASE_16,     ///< Base16 RFC 4648 §8 (`--algorithm=base16`).
            BASE_32,     ///< Base32 RFC 4648 §6 (`--algorithm=base32`).
            BASE_32_HEX, ///< Base32Hex RFC 4648 §7 (`--algorithm=base32hex`).
            BASE_64,     ///< Base64 RFC 4648 §4 (`--algorithm=base64`).
            BASE_64_URL, ///< Base64Url RFC 4648 §5 (`--algorithm=base64url`).
            ASCII_85     ///< Ascii85 (`--algorithm=ascii85`).
        };

        /// @brief Case to use (Base16 only).
        enum class Case
        {
            NONE,      ///< Empty placeholder.
            LOWERCASE, ///< Lowercase (`--case=lowercase`).
            MIXED,     ///< Mixed (`--case=mixed`).
            UPPERCASE  ///< Uppercase (`--case=uppercase`).
        };

        /// @brief Whether or not to have padding in the form of = (Base32, Base32Hex, Base64 and Base64Url only).
        enum class Padding
        {
            NONE,           ///< Empty placeholder.
            ENABLE_PADDING, ///< Enable padding.
            DISABLE_PADDING ///< Disable padding (`--without-padding`).
        };

        /// @brief Whether or not to convert 4 consecutive spaces into y (Ascii85 only).
        enum class SpaceFolding
        {
            NONE,                 ///< Empty placeholder.
            ENABLE_SPACE_FOLDING, ///< Enable space folding (`--fold-spaces`).
            DISABLE_SPACE_FOLDING ///< Disable space folding.
        };

        /// @brief Whether or not to use have <~ and ~> delimiters (Ascii85 only).
        enum class AdobeMode
        {
            NONE,              ///< Empty placeholder.
            ENABLE_ADOBE_MODE, ///< Enable Adobe mode (`--adobe-mode`).
            DISABLE_ADOBE_MODE ///< Disable Adobe mode.
        };

        /// @brief A simple error class for the Arguments class.
        class Error : public std::exception
        {
        public:
            /// @brief Creates a generic Error.
            Error() :
                _what("Utility::Arguments::Error")
            {}
            /**
             * @brief Creates an Error with a given reason.
             * @param[in] what_ Reason for the Error.
            */
            explicit Error(const std::string& what_) :
                _what(what_)
            {}

            /**
             * @brief Gets reason for the Error.
             * @returns Reason for the Error.
            */
            std::string What() const { return _what; }

            // For C++ compatibility purposes

            const char* what() const noexcept override { return _what.c_str(); }

        private:
            std::string _what;
        };

        /// @brief Creates an empty Arguments object.
        Arguments() :
            _task(Task::NONE),
            _algorithm(Algorithm::NONE),
            _case(Case::NONE),
            _padding(Padding::NONE),
            _spaceFolding(SpaceFolding::NONE),
            _adobeMode(AdobeMode::NONE)
        {}
        /**
         * @brief Creates an Arguments object with data from given command-line arguments.
         * @param[in] argumentVector_ Vector of command-line arguments.
         * @throws Utility::Arguments::Error
        */
        explicit Arguments(const std::vector<std::string_view>& argumentVector_) :
            _task(Task::NONE),
            _algorithm(Algorithm::NONE),
            _case(Case::NONE),
            _padding(Padding::NONE),
            _spaceFolding(SpaceFolding::NONE),
            _adobeMode(AdobeMode::NONE)
        {
            ParseArguments(argumentVector_);
        }

        /**
         * @brief Gets Task if it was passed. If it was not an Error is thrown.
         * @returns Task that was passed.
         * @throws Utility::Arguments::Error
        */
        Task GetTask() const { return (_task != Task::NONE) ? _task : throw Error(); }
        /**
         * @brief Gets Algorithm if it was passed. If it was not an Error is thrown.
         * @returns Algorith that was passed.
         * @throws Utility::Arguments::Error
        */
        Algorithm GetAlgorithm() const { return (_algorithm != Algorithm::NONE) ? _algorithm : throw Error(); }
        /**
         * @brief Gets Case if it was passed. If it was not an Error is thrown.
         * @returns Case that was passed.
         * @throws Utility::Arguments::Error
        */
        Case GetCase() const { return (_case != Case::NONE) ? _case : throw Error(); }
        /**
         * @brief Gets Padding if it was passed. If it was not an Error is thrown.
         * @returns Padding that was passed.
         * @throws Utility::Arguments::Error
        */
        Padding GetPadding() const { return (_padding != Padding::NONE) ? _padding : throw Error(); }
        /**
         * @brief Gets SpaceFolding if it was passed. If it was not an Error is thrown.
         * @returns SpaceFolding that was passed.
         * @throws Utility::Arguments::Error
        */
        SpaceFolding GetSpaceFolding() const { return (_spaceFolding != SpaceFolding::NONE) ? _spaceFolding : throw Error(); }
        /**
         * @brief Gets AdobeMode if it was passed. If it was not an Error is thrown.
         * @returns AdobeMode that was passed.
         * @throws Utility::Arguments::Error
        */
        AdobeMode GetAdobeMode() const { return (_adobeMode != AdobeMode::NONE) ? _adobeMode : throw Error(); }
        /**
         * @brief Gets constant reference to input string if it was passed. If it was not an Error is thrown.
         * @returns Constant reference to input string that was passed.
         * @throws Utility::Arguments::Error
        */
        const std::string& GetInputString() const { return (not _inputString.empty()) ? _inputString : throw Error(); }
        /**
         * @brief Gets constant reference to input file path if it was passed. If it was not an Error is thrown.
         * @returns Constant reference to input file path that was passed.
         * @throws Utility::Arguments::Error
        */
        const std::filesystem::path& GetInputFilePath() const { return (not _inputFilePath.empty()) ? _inputFilePath : throw Error(); }
        /**
         * @brief Gets constant reference to output file path if it was passed. If it was not an Error is thrown.
         * @returns Constant reference to output file path that was passed.
         * @throws Utility::Arguments::Error
        */
        const std::filesystem::path& GetOutputFilePath() const { return (not _outputFilePath.empty()) ? _outputFilePath : throw Error(); }
        /**
         * @brief Checks if the input string was passed.
         * @returns Whether or not the input string was passed.
        */
        bool HasInputString() const noexcept { return not _inputString.empty(); }
        /**
         * @brief Checks if the input file path was passed.
         * @returns Whether or not the input file path was passed.
        */
        bool HasInputFilePath() const noexcept { return not _inputFilePath.empty(); }
        /**
         * @brief Checks if the output file path was passed.
         * @returns Whether or not the output file path was passed.
        */
        bool HasOutputFilePath() const noexcept { return not _outputFilePath.empty(); }
        /**
         * @brief Parses data from given command-line arguments.
         * @param[in] argumentVector_ Vector of command-line arguments.
         * @throws Utility::Arguments::Error
        */
        void ParseArguments(const std::vector<std::string_view>& argumentVector_);

    private:
        /// @brief Resets the internal state of the Arguments object.
        void Reset();

        Task _task;
        Algorithm _algorithm;
        Case _case;
        Padding _padding;
        SpaceFolding _spaceFolding;
        AdobeMode _adobeMode;
        std::string _inputString;
        std::filesystem::path _inputFilePath;
        std::filesystem::path _outputFilePath;
    };

    /// @brief A simple error class for the Utility namespace. It is not used in the Arguments class.
    class Error : public std::exception
    {
    public:
        /**
         * @brief Creates an Error with a given reason.
         * @param[in] what_ Reason for the Error.
        */
        explicit Error(const std::string& what_) :
            _what(what_)
        {}

        /**
         * @brief Gets reason for the Error.
         * @returns Reason for the Error.
        */
        std::string What() const { return _what; }

        // For C++ compatibility purposes

        const char* what() const noexcept override { return _what.c_str(); }

    private:
        std::string _what;
    };

    /**
     * This function will print information about where the function was called to stderr and terminate the program abruptly. 
     * It is supposed to be used whenever a theoretically unreachable point was reached.
     * 
     * @param[in] sourceLocation_ The location at which this function was called from, the default argument should be used.
    */
    [[noreturn]] void UnreachableTerminate(const std::source_location sourceLocation_ = std::source_location::current()) noexcept;
    /**
     * @brief Prints a message to stdout(if the exit code is 0) or stderr(if the exit code is anything but 0) and exits the application with an exit code.
     * @param[in] exitMessage_ Message to be printed.
     * @param[in] exitCode_ Exit code to be used.
    */
    [[noreturn]] void Exit(const std::string& exitMessage_, const int exitCode_) noexcept;
    /**
     * @brief Writes a string into a given file.
     * @param[in] string_ String to be written to file.
     * @param[in] filePath_ Path to file.
     * @throws Utility::Error
    */
    void WriteStringToFile(const std::string& string_, const std::filesystem::path& filePath_);
    /**
     * @brief Reads a given file into a string.
     * @param[in] filePath_ Path to file.
     * @returns Read string.
     * @throws Utility::Error
    */
    std::string ReadStringFromFile(const std::filesystem::path& filePath_);
}