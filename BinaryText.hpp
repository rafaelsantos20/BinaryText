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

#include <algorithm>       // std::fill / std::copy
#include <array>           // std::array
#include <bitset>          // std::bitset
#include <cmath>           // std::ceil / std::floor
#include <cstddef>         // std::size_t / std::ptrdiff_t
#include <exception>       // std::terminate / std::exception
#include <filesystem>      // std::filesystem::path
#include <format>          // std::format
#include <fstream>         // std::ifstream / std::ofstream
#include <iostream>        // std::cerr / std::endl
#include <iterator>        // std::contiguous_iterator_tag / std::contiguous_iterator / std::next / std::advance / std::prev
#include <limits>          // std::numeric_limits::digits / std::numeric_limits::max
#include <memory>          // std::unique_ptr / std::make_unique
#include <new>             // std::bad_alloc
#include <source_location> // std::source_location
#include <stdexcept>       // std::length_error
#include <string>          // std::string
#include <type_traits>     // std::remove_const_t / std::disjunction_v / std::is_same_v / std::is_same
#include <utility>         // std::swap / std::move
#include <vector>          // std::vector

/// @brief BinaryText namespace.
namespace BinaryText
{
    /**
     * This function will print information about where the function was called to stderr and terminate the program abruptly. 
     * It is supposed to be used whenever a theoretically unreachable point was reached.
     * 
     * @param[in] sourceLocation_ The location at which this function was called from, the default argument should be used.
    */
    [[noreturn]] void UnreachableTerminate(const std::source_location sourceLocation_ = std::source_location::current()) noexcept
    {
        std::cerr << std::format("Something that should not have gone wrong went wrong and this function was called to terminate the program. This function "
                                 "was called inside \"{}\" at function \"{}\", line {}, column {}",
                                 sourceLocation_.file_name(), sourceLocation_.function_name(), sourceLocation_.line(), sourceLocation_.column())
                  << std::endl;
        std::terminate();
    }

    /**
     * @brief A concept that only accepts types supported by ByteBuffer.
     * @tparam ByteType Either char, signed char, unsigned char or std::byte.
    */
    template<typename ByteType>
    concept ByteBufferCompatible = std::disjunction_v<std::is_same<ByteType, char>, std::is_same<ByteType, signed char>, std::is_same<ByteType, unsigned char>,
                                                      std::is_same<ByteType, std::byte>>;

    /**
     * @brief A class that is able to hold a buffer made up of bytes.
     * @tparam ByteType Type that satisfies the ByteBufferCompatible concept (char, signed char, unsigned char and std::byte).
    */
    template<ByteBufferCompatible ByteType>
    class ByteBuffer
    {
    public:
        /// @brief A simple error class for the ByteBuffer class.
        class Error : public std::exception
        {
        public:
            /// @brief The type of Error.
            enum class Type
            {
                EMPTY_BUFFER_ERROR,       ///< Internal buffer is empty.
                INVALID_ARGUMENTS_ERROR,  ///< Invalid arguments.
                OPEN_FILE_ERROR,          ///< Failed to open the file.
                READ_FROM_FILE_ERROR,     ///< Failed to read from file.
                WRITE_TO_FILE_ERROR,      ///< Failed to write to file.
                OUT_OF_RANGE_ERROR,       ///< Failed to access byte at given position.
                MAXIMUM_SIZE_LIMIT_ERROR, ///< Maximum size limit reached.
                ALLOCATION_ERROR          ///< Failed to allocate memory.
            };

            /**
             * @brief Creates an Error of given Type.
             * @param[in] type_ Type of Error.
             * @param[in] sourceLocation_ Source location of Error.
            */
            explicit Error(const Type type_, const std::source_location sourceLocation_ = std::source_location::current()) :
                _type(type_),
                _sourceLocation(sourceLocation_)
            {
                switch(_type) {
                    case Type::EMPTY_BUFFER_ERROR: _what = "Internal buffer is empty"; break;
                    case Type::INVALID_ARGUMENTS_ERROR: _what = "Invalid arguments"; break;
                    case Type::OPEN_FILE_ERROR: _what = "Failed to open file"; break;
                    case Type::READ_FROM_FILE_ERROR: _what = "Failed to read from file"; break;
                    case Type::WRITE_TO_FILE_ERROR: _what = "Failed to write to file"; break;
                    case Type::OUT_OF_RANGE_ERROR: _what = "Failed to access byte at given position"; break;
                    case Type::MAXIMUM_SIZE_LIMIT_ERROR: _what = "Maximum size limit reached"; break;
                    case Type::ALLOCATION_ERROR: _what = "Failed to allocate memory"; break;
                    default: _what = "Invalid error type"; break;
                }
            }

            /**
             * @brief Gets the Type of the Error.
             * @returns Type of Error.
            */
            Type GetType() const noexcept { return _type; }
            /**
             * @brief Gets the location at which the Error was thrown.
             * @returns Source location of Error.
            */
            std::source_location GetSourceLocation() const { return _sourceLocation; }
            /**
             * @brief Gets reason for the Error.
             * @returns Reason for the Error.
            */
            std::string What() const { return _what; }

            // For C++ compatibility purposes

            const char* what() const noexcept override { return _what.c_str(); }

        private:
            Type _type;
            std::source_location _sourceLocation;
            std::string _what;
        };

        /**
         * @brief A base iterator class that can be used as a non-constant iterator or a constant one.
         * @tparam ValueType Same as ByteType but can be constant.
        */
        template<typename ValueType>
            requires std::is_same_v<std::remove_const_t<ValueType>, std::remove_const_t<ByteType>>
        class BaseIterator
        {
        public:
            using IteratorCategory = std::contiguous_iterator_tag;
            using DifferenceType = std::ptrdiff_t;
            using Pointer = ValueType*;
            using Reference = ValueType&;

            using iterator_category = IteratorCategory;
            using difference_type = DifferenceType;
            using value_type = ValueType;
            using pointer = Pointer;
            using reference = Reference;

            // clang-format off

            BaseIterator() noexcept : _address(nullptr) {}
            explicit BaseIterator(const Pointer address_) : _address(address_) {}

            Reference operator[](const DifferenceType position_) const { return _address[position_]; }
            Reference operator*() const { return *_address; }
            Pointer operator->() const { return _address; }
            BaseIterator& operator=(const Pointer address_) { _address = address_; return *this; }
            BaseIterator& operator+=(const DifferenceType difference_) { _address += difference_; return *this; }
            BaseIterator& operator-=(const DifferenceType difference_) { _address -= difference_; return *this; }
            BaseIterator& operator++() { _address++; return *this; }
            BaseIterator& operator--() { _address--; return *this; }
            BaseIterator operator++(int) { BaseIterator iterator(*this); ++_address; return iterator; }
            BaseIterator operator--(int) { BaseIterator iterator(*this); --_address; return iterator; }
            BaseIterator operator+(const DifferenceType difference_) const { BaseIterator iterator(*this); return iterator += difference_; }
            BaseIterator operator-(const DifferenceType difference_) const { BaseIterator iterator(*this); return iterator -= difference_; }
            friend BaseIterator operator+(const DifferenceType difference_, const BaseIterator& iterator_) { return iterator_ + difference_; }
            friend BaseIterator operator-(const DifferenceType difference_, const BaseIterator& iterator_) { return iterator_ - difference_; }
            DifferenceType operator-(const BaseIterator& iterator_) const { return std::distance(iterator_._address, _address); }
            operator BaseIterator<const ValueType>() const requires std::is_same_v<ValueType, std::remove_const_t<ValueType>> { return BaseIterator<const ValueType>(_address); }
            bool operator<(const BaseIterator& iterator_) const { return iterator_._address < _address; }
            bool operator<=(const BaseIterator& iterator_) const { return iterator_._address <= _address; }
            bool operator>(const BaseIterator& iterator_) const { return iterator_._address > _address; }
            bool operator>=(const BaseIterator& iterator_) const { return iterator_._address >= _address; }
            bool operator==(const BaseIterator& iterator_) const { return iterator_._address == _address; }
            bool operator!=(const BaseIterator& iterator_) const { return iterator_._address != _address; }

            // clang-format on

        private:
            Pointer _address;
        };

        static_assert(std::contiguous_iterator<BaseIterator<ByteType>>);
        static_assert(std::contiguous_iterator<BaseIterator<const ByteType>>);

        /**
         * @brief A base reverse iterator class that can be used as a non-constant rerverse iterator or a constant reverse one.
         * @tparam ValueType Same as ByteType but can be constant.
        */
        template<typename ValueType>
            requires std::is_same_v<std::remove_const_t<ValueType>, std::remove_const_t<ByteType>>
        class BaseReverseIterator
        {
        public:
            using IteratorCategory = std::contiguous_iterator_tag;
            using DifferenceType = std::ptrdiff_t;
            using Pointer = ValueType*;
            using Reference = ValueType&;

            using iterator_category = IteratorCategory;
            using difference_type = DifferenceType;
            using value_type = ValueType;
            using pointer = Pointer;
            using reference = Reference;

            // clang-format off

            BaseReverseIterator() noexcept : _address(nullptr) {}
            explicit BaseReverseIterator(const Pointer address_) : _address(address_) {}

            BaseIterator<ValueType> Base() const { BaseIterator<ValueType> iterator(_address); return ++iterator; }
            BaseIterator<ValueType> base() const { BaseIterator<ValueType> iterator(_address); return ++iterator; }

            Reference operator[](const DifferenceType position_) const { return _address[position_]; }
            Reference operator*() const { return *_address; }
            Pointer operator->() const { return _address; }
            BaseReverseIterator& operator=(const Pointer address_) { _address = address_; return *this; }
            BaseReverseIterator& operator+=(const DifferenceType difference_) { _address -= difference_; return *this; }
            BaseReverseIterator& operator-=(const DifferenceType difference_) { _address += difference_; return *this; }
            BaseReverseIterator& operator++() { _address--; return *this; }
            BaseReverseIterator& operator--() { _address++; return *this; }
            BaseReverseIterator operator++(int) { BaseReverseIterator iterator(*this); --_address; return iterator; }
            BaseReverseIterator operator--(int) { BaseReverseIterator iterator(*this); ++_address; return iterator; }
            BaseReverseIterator operator+(const DifferenceType difference_) const { BaseReverseIterator iterator(*this); return iterator -= difference_; }
            BaseReverseIterator operator-(const DifferenceType difference_) const { BaseReverseIterator iterator(*this); return iterator += difference_; }
            friend BaseReverseIterator operator+(const DifferenceType difference_, const BaseReverseIterator& iterator_) { return iterator_ - difference_; }
            friend BaseReverseIterator operator-(const DifferenceType difference_, const BaseReverseIterator& iterator_) { return iterator_ + difference_; }
            DifferenceType operator-(const BaseReverseIterator& iterator_) const { return std::distance(_address, iterator_._address); }
            operator BaseReverseIterator<const ValueType>() const requires std::is_same_v<ValueType, std::remove_const_t<ValueType>> { return BaseReverseIterator<const ValueType>(_address); }
            bool operator<(const BaseReverseIterator& iterator_) const { return iterator_._address < _address; }
            bool operator<=(const BaseReverseIterator& iterator_) const { return iterator_._address <= _address; }
            bool operator>(const BaseReverseIterator& iterator_) const { return iterator_._address > _address; }
            bool operator>=(const BaseReverseIterator& iterator_) const { return iterator_._address >= _address; }
            bool operator==(const BaseReverseIterator& iterator_) const { return iterator_._address == _address; }
            bool operator!=(const BaseReverseIterator& iterator_) const { return iterator_._address != _address; }

            // clang-format on

        private:
            Pointer _address;
        };

        static_assert(std::contiguous_iterator<BaseReverseIterator<ByteType>>);
        static_assert(std::contiguous_iterator<BaseReverseIterator<const ByteType>>);

        using ValueType = ByteType;
        using Reference = ValueType&;
        using ConstantReference = const ValueType&;
        using Iterator = BaseIterator<ValueType>;
        using ConstantIterator = BaseIterator<const ValueType>;
        using ReverseIterator = BaseReverseIterator<ValueType>;
        using ConstantReverseIterator = BaseReverseIterator<const ValueType>;
        using DifferenceType = std::ptrdiff_t;
        using SizeType = std::size_t;

        /// @brief Creates an empty ByteBuffer.
        ByteBuffer() noexcept :
            _size(0),
            _buffer(nullptr)
        {}
        /**
         * @brief Creates a ByteBuffer of given size.
         * @param[in] size_ Size of ByteBuffer to be created.
         * @throws BinaryText::ByteBuffer::Error
        */
        explicit ByteBuffer(const SizeType size_) :
            _size(0),
            _buffer(nullptr)
        {
            if(size_ > GetMaximumSize()) {
                throw Error(Error::Type::MAXIMUM_SIZE_LIMIT_ERROR);
            } else if(size_ > 0) {
                _size = size_;

                try {
                    _buffer = std::make_unique<ValueType[]>(_size);

                    std::fill(_buffer.get(), _buffer.get() + _size, static_cast<ValueType>(0));
                } catch(const std::bad_alloc& error) {
                    _size = 0;
                    _buffer = nullptr;

                    throw Error(Error::Type::ALLOCATION_ERROR);
                }
            }
        }
        /**
         * @brief Creates a ByteBuffer from a pointer to a ByteType buffer.
         * @param[in] buffer_ Pointer to buffer to be copied.
         * @param[in] size_ Size of the buffer to be copied.
         * @throws BinaryText::ByteBuffer::Error
        */
        ByteBuffer(const ValueType* buffer_, const SizeType size_) :
            _size(0),
            _buffer(nullptr)
        {
            if(size_ > GetMaximumSize()) {
                throw Error(Error::Type::MAXIMUM_SIZE_LIMIT_ERROR);
            } else if(size_ > 0 and buffer_ == nullptr) {
                throw Error(Error::Type::INVALID_ARGUMENTS_ERROR);
            } else if(size_ < 1 and buffer_ != nullptr) {
                throw Error(Error::Type::INVALID_ARGUMENTS_ERROR);
            } else if(size_ < 1 and buffer_ == nullptr) {
                _size = 0;
                _buffer = nullptr;
            } else {
                _size = size_;

                try {
                    _buffer = std::make_unique<ValueType[]>(_size);

                    std::copy(buffer_, buffer_ + _size, _buffer.get());
                } catch(const std::bad_alloc& error) {
                    _size = 0;
                    _buffer = nullptr;

                    throw Error(Error::Type::ALLOCATION_ERROR);
                }
            }
        }
        /**
         * @brief Creates a ByteBuffer from an std::vector.
         * @param[in] vector_ Vector to be copied.
         * @throws BinaryText::ByteBuffer::Error
        */
        explicit ByteBuffer(const std::vector<ValueType>& vector_) :
            _size(0),
            _buffer(nullptr)
        {
            if(vector_.size() > GetMaximumSize()) {
                throw Error(Error::Type::MAXIMUM_SIZE_LIMIT_ERROR);
            } else if(vector_.size() > 0) {
                _size = vector_.size();

                try {
                    _buffer = std::make_unique<ValueType[]>(_size);

                    std::copy(vector_.cbegin(), vector_.cend(), _buffer.get());
                } catch(const std::bad_alloc& error) {
                    _size = 0;
                    _buffer = nullptr;

                    throw Error(Error::Type::ALLOCATION_ERROR);
                }
            }
        }
        /**
         * @brief Creates a ByteBuffer from a given file in the filesystem
         * @param[in] filePath_ Path to file
         * @throws BinaryText::ByteBuffer::Error
        */
        explicit ByteBuffer(const std::filesystem::path& filePath_) { ReadFromFile(filePath_); }
        /**
         * @brief Copy constructor of ByteBuffer.
         * @param[in] byteBuffer_ ByteBuffer to be copied.
        */
        ByteBuffer(const ByteBuffer& byteBuffer_) :
            _size(0),
            _buffer(nullptr)
        {
            if(byteBuffer_._buffer != nullptr) {
                _size = byteBuffer_._size;

                try {
                    _buffer = std::make_unique<ValueType[]>(_size);

                    std::copy(byteBuffer_._buffer.get(), byteBuffer_._buffer.get() + _size, _buffer.get());
                } catch(const std::bad_alloc& error) {
                    _size = 0;
                    _buffer = nullptr;

                    throw Error(Error::Type::ALLOCATION_ERROR);
                }
            }
        }
        /**
         * @brief Move constructor of ByteBuffer.
         * @param[in] byteBuffer_ ByteBuffer to be moved.
        */
        ByteBuffer(ByteBuffer&& byteBuffer_) noexcept :
            _size(byteBuffer_._size),
            _buffer(std::move(byteBuffer_._buffer))
        {
            byteBuffer_._size = 0;
            byteBuffer_._buffer = nullptr;
        }

        /**
         * @brief Fills an entire ByteBuffer with given byte.
         * @param[in] byte_ Byte to fill the entire ByteBuffer with.
         * @throws BinaryText::ByteBuffer::Error
        */
        void SetAllBytes(const ValueType byte_)
        {
            if(_buffer != nullptr) {
                try {
                    std::fill(_buffer.get(), _buffer.get() + _size, byte_);
                } catch(const std::bad_alloc& error) {
                    _size = 0;
                    _buffer = nullptr;

                    throw Error(Error::Type::ALLOCATION_ERROR);
                }
            }
        }
        /**
         * @brief Gets non-constant pointer to internal buffer.
         * @returns Non-constant pointer to the internal buffer.
        */
        ValueType* GetBuffer() noexcept { return _buffer.get(); }
        /**
         * @brief Gets constant pointer to internal buffer. 
         * @returns Constant pointer to the internal buffer.
        */
        const ValueType* GetBuffer() const noexcept { return _buffer.get(); }
        /**
         * @brief Gets size of the ByteBuffer.
         * @returns Size of the ByteBuffer.
        */
        SizeType GetSize() const noexcept { return _size; }
        /**
         * @brief Gets maximum size that a ByteBuffer can have.
         * @returns Maximum size that a ByteBuffer can have.
        */
        static consteval SizeType GetMaximumSize() noexcept { return static_cast<SizeType>(std::numeric_limits<DifferenceType>::max()); }
        /**
         * @brief Gets non-constant reference to a position in the ByteBuffer.
         * @param[in] position_ Position to be accessed.
         * @returns Non-constant reference to a position in the ByteBuffer.
         * @throws BinaryText::ByteBuffer::Error
        */
        Reference At(const SizeType position_)
        {
            return (position_ < _size and _buffer != nullptr) ? _buffer[position_] : throw Error(Error::Type::OUT_OF_RANGE_ERROR);
        }
        /**
         * @brief Gets constant reference to a position in the ByteBuffer.
         * @param[in] position_ Position to be accessed.
         * @returns Constant reference to a position in the ByteBuffer.
         * @throws BinaryText::ByteBuffer::Error
        */
        ConstantReference At(const SizeType position_) const
        {
            return (position_ < _size and _buffer != nullptr) ? _buffer[position_] : throw Error(Error::Type::OUT_OF_RANGE_ERROR);
        }
        /**
         * @brief Gets non-constant reference to a position in the ByteBuffer. It does not check if the position is valid.
         * @param[in] position_ Position to be accessed.
         * @returns Non-constant reference to a position in the ByteBuffer.
        */
        Reference UncheckedAt(const SizeType position_) { return _buffer[position_]; }
        /**
         * @brief Gets constant reference to a position in the ByteBuffer. It does not check if the position is valid.
         * @param[in] position_ Position to be accessed.
         * @returns Constant reference to a position in the ByteBuffer.
        */
        ConstantReference UncheckedAt(const SizeType position_) const { return _buffer[position_]; }
        /**
         * @brief Checks if the ByteBuffer is empty.
         * @returns Whether ByteBuffer is empty or not.
        */
        bool IsEmpty() const noexcept { return _buffer == nullptr; }
        /**
         * @brief Resizes the ByteBuffer to given size.
         * @param[in] size_ Size to resize it to.
         * @throws BinaryText::ByteBuffer::Error
        */
        void Resize(const SizeType size_)
        {
            if(size_ > GetMaximumSize()) {
                throw Error(Error::Type::MAXIMUM_SIZE_LIMIT_ERROR);
            } else if(size_ == _size) {
                return;
            } else if(size_ < 1) {
                _size = 0;
                _buffer = nullptr;
            } else {
                const SizeType previousSize(_size);
                _size = size_;

                try {
                    std::unique_ptr<ValueType[]> previousBuffer(std::make_unique<ValueType[]>(previousSize));

                    std::copy(_buffer.get(), _buffer.get() + previousSize, previousBuffer.get());

                    _buffer = std::make_unique<ValueType[]>(_size);

                    if(_size < previousSize) {
                        std::copy(previousBuffer.get(), previousBuffer.get() + _size, _buffer.get());
                    } else {
                        std::copy(previousBuffer.get(), previousBuffer.get() + previousSize, _buffer.get());

                        previousBuffer = nullptr;

                        std::fill(_buffer.get() + previousSize, _buffer.get() + _size, static_cast<ValueType>(0));
                    }
                } catch(const std::bad_alloc& error) {
                    _size = 0;
                    _buffer = nullptr;

                    throw Error(Error::Type::ALLOCATION_ERROR);
                }
            }
        }
        /**
         * @brief Resizes the ByteBuffer to given size.
         * @param[in] size_ Size to resize it to.
         * @param[in] byte_ Byte to append in case the provided size is bigger than current size.
         * @throws BinaryText::ByteBuffer::Error
        */
        void Resize(const SizeType size_, const ValueType byte_)
        {
            if(size_ > GetMaximumSize()) {
                throw Error(Error::Type::MAXIMUM_SIZE_LIMIT_ERROR);
            } else if(size_ == _size) {
                return;
            } else if(size_ < 1) {
                _size = 0;
                _buffer = nullptr;
            } else {
                const SizeType previousSize(_size);
                _size = size_;

                try {
                    std::unique_ptr<ValueType[]> previousBuffer(std::make_unique<ValueType[]>(previousSize));

                    std::copy(_buffer.get(), _buffer.get() + previousSize, previousBuffer.get());

                    _buffer = std::make_unique<ValueType[]>(_size);

                    if(_size < previousSize) {
                        std::copy(previousBuffer.get(), previousBuffer.get() + _size, _buffer.get());
                    } else {
                        std::copy(previousBuffer.get(), previousBuffer.get() + previousSize, _buffer.get());

                        previousBuffer = nullptr;

                        std::fill(_buffer.get() + previousSize, _buffer.get() + _size, byte_);
                    }
                } catch(const std::bad_alloc& error) {
                    _size = 0;
                    _buffer = nullptr;

                    throw Error(Error::Type::ALLOCATION_ERROR);
                }
            }
        }
        /**
         * @brief Swaps two ByteBuffers with each other.
         * @param[in,out] byteBuffer_ ByteBuffer to swap with.
        */
        void Swap(ByteBuffer& byteBuffer_) noexcept
        {
            std::swap(_size, byteBuffer_._size);
            std::swap(_buffer, byteBuffer_._buffer);
        }
        /// @brief Clears the ByteBuffer
        void Clear() noexcept
        {
            _size = 0;
            _buffer = nullptr;
        }
        /**
         * @brief Reads the contents of a file into the ByteBuffer. If an error is thrown the ByteBuffer is reset.
         * @param[in] filePath_ Path to file.
         * @throws BinaryText::ByteBuffer::Error
        */
        void ReadFromFile(const std::filesystem::path& filePath_)
        {
            std::ifstream fileStream(filePath_, std::ifstream::in | std::ifstream::binary);
            _size = 0;
            _buffer = nullptr;

            if(fileStream.is_open()) {
                while(fileStream) {
                    ByteBuffer byteBuffer(8192);

                    if constexpr(std::is_same_v<ValueType, char>) {
                        fileStream.read(byteBuffer._buffer.get(), 8192);
                    } else {
                        fileStream.read(reinterpret_cast<char*>(byteBuffer._buffer.get()), 8192);
                    }

                    if(not fileStream.fail() or fileStream.eof()) {
                        const std::streamsize readCount(fileStream.gcount());

                        if(readCount > 0) {
                            byteBuffer.Resize(static_cast<SizeType>(readCount));
                            operator+=(byteBuffer);
                        }
                    } else {
                        _size = 0;
                        _buffer = nullptr;

                        throw Error(Error::Type::READ_FROM_FILE_ERROR);
                    }
                }
            } else {
                _size = 0;
                _buffer = nullptr;

                throw Error(Error::Type::OPEN_FILE_ERROR);
            }
        }
        /**
         * @brief Writes the entire ByteBuffer into a file.
         * @param[in] filePath_ Path to file.
         * @throws BinaryText::ByteBuffer::Error
        */
        void WriteToFile(const std::filesystem::path& filePath_) const
        {
            if(_buffer != nullptr) {
                std::ofstream fileStream(filePath_, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);

                if(fileStream.is_open()) {
                    if(_size > 8192) {
                        const SizeType chunkAmount(static_cast<SizeType>(static_cast<double>(_size) / 8192.0));
                        const SizeType lastChunkSize(_size - (8192 * chunkAmount));

                        for(SizeType i(0); i < chunkAmount; ++i) {
                            std::unique_ptr<ValueType[]> chunk(nullptr);

                            try {
                                chunk = std::make_unique<ValueType[]>(8192);

                                std::copy(_buffer.get() + (8192 * i), _buffer.get() + (8192 * i) + 8192, chunk.get());
                            } catch(const std::bad_alloc& error) {
                                throw Error(Error::Type::ALLOCATION_ERROR);
                            }

                            if constexpr(std::is_same_v<ValueType, char>) {
                                fileStream.write(chunk.get(), 8192);
                            } else {
                                fileStream.write(reinterpret_cast<char*>(chunk.get()), 8192);
                            }

                            if(fileStream.fail()) {
                                throw Error(Error::Type::WRITE_TO_FILE_ERROR);
                            }
                        }

                        if(lastChunkSize > 0) {
                            std::unique_ptr<ValueType[]> chunk(nullptr);

                            try {
                                chunk = std::make_unique<ValueType[]>(lastChunkSize);

                                std::copy(_buffer.get() + (8192 * chunkAmount), _buffer.get() + ((8192 * chunkAmount) + lastChunkSize), chunk.get());
                            } catch(const std::bad_alloc& error) {
                                throw Error(Error::Type::ALLOCATION_ERROR);
                            }

                            if constexpr(std::is_same_v<ValueType, char>) {
                                fileStream.write(chunk.get(), lastChunkSize);
                            } else {
                                fileStream.write(reinterpret_cast<char*>(chunk.get()), lastChunkSize);
                            }

                            if(fileStream.fail()) {
                                throw Error(Error::Type::WRITE_TO_FILE_ERROR);
                            }
                        }
                    } else {
                        std::unique_ptr<ValueType[]> chunk(nullptr);

                        try {
                            chunk = std::make_unique<ValueType[]>(_size);

                            std::copy(_buffer.get(), _buffer.get() + _size, chunk.get());
                        } catch(const std::bad_alloc& error) {
                            throw Error(Error::Type::ALLOCATION_ERROR);
                        }

                        if constexpr(std::is_same_v<ValueType, char>) {
                            fileStream.write(chunk.get(), _size);
                        } else {
                            fileStream.write(reinterpret_cast<char*>(chunk.get()), _size);
                        }

                        if(fileStream.fail()) {
                            throw Error(Error::Type::WRITE_TO_FILE_ERROR);
                        }
                    }
                } else {
                    throw Error(Error::Type::OPEN_FILE_ERROR);
                }
            } else {
                throw Error(Error::Type::EMPTY_BUFFER_ERROR);
            }
        }
        /**
         * @brief Converts a ByteBuffer into an std::vector.
         * @returns The converted std::vector.
        */
        std::vector<ValueType> ToVector() const
        {
            if(_buffer != nullptr) {
                return std::vector<ValueType>(_buffer.get(), _buffer.get() + _size);
            } else {
                return std::vector<ValueType>();
            }
        }
        /**
         * @brief Gets an iterator that points to the beginning of the ByteBuffer.
         * @returns Iterator that points to the beginning of the ByteBuffer.
        */
        Iterator Begin() noexcept { return Iterator(&_buffer[0]); }
        /**
         * @brief Gets an iterator that points to the end of the ByteBuffer.
         * @returns Iterator that points to the end of the ByteBuffer.
        */
        Iterator End() noexcept { return Iterator(&_buffer[_size]); }
        /**
         * @brief Gets a constant iterator that points to the beginning of the ByteBuffer.
         * @returns Constant iterator that points to the beginning of the ByteBuffer.
        */
        ConstantIterator Begin() const noexcept { return ConstantIterator(&_buffer[0]); }
        /**
         * @brief Gets a constant iterator that points to the end of the ByteBuffer.
         * @returns Constant iterator that points to the end of the ByteBuffer.
        */
        ConstantIterator End() const noexcept { return ConstantIterator(&_buffer[_size]); }
        /**
         * @brief Gets a constant iterator that points to the beginning of the ByteBuffer.
         * @returns Constant iterator that points to the beginning of the ByteBuffer.
        */
        ConstantIterator ConstantBegin() const noexcept { return ConstantIterator(&_buffer[0]); }
        /**
         * @brief Gets a constant iterator that points to the end of the ByteBuffer.
         * @returns Constant iterator that points to the end of the ByteBuffer.
        */
        ConstantIterator ConstantEnd() const noexcept { return ConstantIterator(&_buffer[_size]); }
        /**
         * @brief Gets a reverse iterator that points to the beginning of the ByteBuffer.
         * @returns Reverse iterator that points to the beginning of the ByteBuffer.
        */
        ReverseIterator ReverseBegin() noexcept { return ReverseIterator(&_buffer[(_size > 0) ? (_size - 1) : -1]); }
        /**
         * @brief Gets a reverse iterator that points to the end of the ByteBuffer.
         * @returns Reverse iterator that points to the end of the ByteBuffer.
        */
        ReverseIterator ReverseEnd() noexcept { return ReverseIterator(&_buffer[-1]); }
        /**
         * @brief Gets a constant reverse iterator that points to the beginning of the ByteBuffer.
         * @returns Constant reverse iterator that points to the beginning of the ByteBuffer.
        */
        ConstantReverseIterator ReverseBegin() const noexcept { return ConstantReverseIterator(&_buffer[(_size > 0) ? (_size - 1) : -1]); }
        /**
         * @brief Gets a constant reverse iterator that points to the end of the ByteBuffer.
         * @returns Constant reverse iterator that points to the end of the ByteBuffer.
        */
        ConstantReverseIterator ReverseEnd() const noexcept { return ConstantReverseIterator(&_buffer[-1]); }
        /**
         * @brief Gets a constant reverse iterator that points to the beginning of the ByteBuffer.
         * @returns Constant reverse iterator that points to the beginning of the ByteBuffer.
        */
        ConstantReverseIterator ConstantReverseBegin() const noexcept { return ConstantReverseIterator(&_buffer[(_size > 0) ? (_size - 1) : -1]); }
        /**
         * @brief Gets a constant reverse iterator that points to the end of the ByteBuffer.
         * @returns Constant reverse iterator that points to the end of the ByteBuffer.
        */
        ConstantReverseIterator ConstantReverseEnd() const noexcept { return ConstantReverseIterator(&_buffer[-1]); }

        /**
         * @brief Copy assignment operator of ByteBuffer.
         * @param[in] byteBuffer_ ByteBuffer to be copied.
         * @throws BinaryText::ByteBuffer::Error
        */
        ByteBuffer& operator=(const ByteBuffer& byteBuffer_)
        {
            if(&byteBuffer_ == this) {
                return *this;
            } else {
                if(byteBuffer_._buffer != nullptr) {
                    _size = byteBuffer_._size;

                    try {
                        _buffer = std::make_unique<ValueType[]>(_size);

                        std::copy(byteBuffer_._buffer.get(), byteBuffer_._buffer.get() + _size, _buffer.get());
                    } catch(const std::bad_alloc& error) {
                        _size = 0;
                        _buffer = nullptr;

                        throw Error(Error::Type::ALLOCATION_ERROR);
                    }

                    return *this;
                } else {
                    _size = 0;
                    _buffer = nullptr;

                    return *this;
                }
            }
        }
        /**
         * @brief Move assignment operator of ByteBuffer.
         * @param[in] byteBuffer_ ByteBuffer to be moved.
        */
        ByteBuffer& operator=(ByteBuffer&& byteBuffer_) noexcept
        {
            if(&byteBuffer_ == this) {
                return *this;
            } else {
                _size = byteBuffer_._size;
                _buffer = std::move(byteBuffer_._buffer);
                byteBuffer_._size = 0;
                byteBuffer_._buffer = nullptr;

                return *this;
            }
        }
        /**
         * @brief Addition compound assignment operator of ByteBuffer.
         * @param[in] byteBuffer_ ByteBuffer to add.
         * @throws BinaryText::ByteBuffer::Error
        */
        ByteBuffer& operator+=(const ByteBuffer& byteBuffer_)
        {
            if(_buffer != nullptr) {
                if(byteBuffer_._buffer != nullptr) {
                    const SizeType nextSize(_size + byteBuffer_._size);

                    if(nextSize > GetMaximumSize()) {
                        throw Error(Error::Type::MAXIMUM_SIZE_LIMIT_ERROR);
                    } else if(nextSize < _size) {
                        throw Error(Error::Type::MAXIMUM_SIZE_LIMIT_ERROR);
                    } else {
                        std::unique_ptr<ValueType[]> nextBuffer(nullptr);

                        try {
                            nextBuffer = std::make_unique<ValueType[]>(nextSize);

                            std::copy(_buffer.get(), _buffer.get() + _size, nextBuffer.get());
                            std::copy(byteBuffer_._buffer.get(), byteBuffer_._buffer.get() + byteBuffer_._size, nextBuffer.get() + _size);
                        } catch(const std::bad_alloc& error) {
                            _size = 0;
                            _buffer = nullptr;

                            throw Error(Error::Type::ALLOCATION_ERROR);
                        }

                        _size = nextSize;
                        _buffer = std::move(nextBuffer);

                        return *this;
                    }
                } else {
                    return *this;
                }
            } else {
                if(byteBuffer_._buffer != nullptr) {
                    _size = byteBuffer_._size;
                    _buffer = std::make_unique<ValueType[]>(_size);

                    std::copy(byteBuffer_._buffer.get(), byteBuffer_._buffer.get() + _size, _buffer.get());

                    return *this;
                } else {
                    return *this;
                }
            }
        }
        /**
         * @brief Addition operator of ByteBuffer
         * @param[in] byteBuffer_ ByteBuffer to add.
         * @throws BinaryText::ByteBuffer::Error
        */
        ByteBuffer operator+(const ByteBuffer& byteBuffer_) const
        {
            if(_buffer != nullptr) {
                if(byteBuffer_._buffer != nullptr) {
                    const SizeType nextSize(_size + byteBuffer_._size);

                    if(nextSize > GetMaximumSize()) {
                        throw Error(Error::Type::MAXIMUM_SIZE_LIMIT_ERROR);
                    } else if(nextSize < _size) {
                        throw Error(Error::Type::MAXIMUM_SIZE_LIMIT_ERROR);
                    } else {
                        std::unique_ptr<ValueType[]> nextBuffer(nullptr);

                        try {
                            nextBuffer = std::make_unique<ValueType[]>(nextSize);

                            std::copy(_buffer.get(), _buffer.get() + _size, nextBuffer.get());
                            std::copy(byteBuffer_._buffer.get(), byteBuffer_._buffer.get() + byteBuffer_._size, nextBuffer.get() + _size);
                        } catch(const std::bad_alloc& error) {
                            _size = 0;
                            _buffer = nullptr;

                            throw Error(Error::Type::ALLOCATION_ERROR);
                        }

                        ByteBuffer byteBuffer;
                        byteBuffer._size = nextSize;
                        byteBuffer._buffer = std::move(nextBuffer);

                        return byteBuffer;
                    }
                } else {
                    return *this;
                }
            } else {
                if(byteBuffer_._buffer != nullptr) {
                    return byteBuffer_;
                } else {
                    return *this;
                }
            }
        }
        /**
         * @brief Equality operator of ByteBuffer.
         * @param[in] byteBuffer_ ByteBuffer to compare with.
        */
        bool operator==(const ByteBuffer& byteBuffer_) const noexcept
        {
            if(_size == byteBuffer_._size) {
                if(_buffer == nullptr) {
                    if(byteBuffer_._buffer == nullptr) {
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    if(byteBuffer_._buffer == nullptr) {
                        return false;
                    } else {
                        for(SizeType i(0); i < _size; ++i) {
                            if(_buffer[i] != byteBuffer_._buffer[i]) {
                                return false;
                            }
                        }

                        return true;
                    }
                }
            } else {
                return false;
            }
        }
        /**
         * @brief Inequality operator of ByteBuffer.
         * @param[in] byteBuffer_ ByteBuffer to compare with.
        */
        bool operator!=(const ByteBuffer& byteBuffer_) const noexcept { return not(operator==(byteBuffer_)); }
        /**
         * @brief Gets non-constant reference to a position in the ByteBuffer.
         * @param[in] position_ Position to be accessed.
         * @throws BinaryText::ByteBuffer::Error
        */
        ValueType& operator[](const SizeType position_)
        {
            return (position_ < _size and _buffer != nullptr) ? _buffer[position_] : throw Error(Error::Type::OUT_OF_RANGE_ERROR);
        }
        /**
         * @brief Gets constant reference to a position in the ByteBuffer.
         * @param[in] position_ Position to be accessed.
         * @throws BinaryText::ByteBuffer::Error
        */
        const ValueType& operator[](const SizeType position_) const
        {
            return (position_ < _size and _buffer != nullptr) ? _buffer[position_] : throw Error(Error::Type::OUT_OF_RANGE_ERROR);
        }

        // For C++ compatibility purposes
        // clang-format off

        using value_type = ValueType;
        using reference = Reference;
        using const_reference = ConstantReference;
        using iterator = Iterator;
        using const_iterator = ConstantIterator;
        using reverse_iterator = ReverseIterator;
        using const_reverse_iterator = ConstantReverseIterator;
        using difference_type = DifferenceType;
        using size_type = SizeType;

        bool empty() const noexcept { return _buffer == nullptr; }
        size_type size() const noexcept { return _size; }
        static consteval size_type max_size() noexcept { return static_cast<size_type>(std::numeric_limits<difference_type>::max()); }
        void resize(const size_type size_) { Resize(size_); }
        void resize(const size_type size_, const value_type byte_) { Resize(size_, byte_); }
        reference at(const size_type position_) { return (position_ < _size and _buffer != nullptr) ? _buffer[position_] : throw Error(Error::Type::OUT_OF_RANGE_ERROR); }
        const_reference at(const size_type position_) const { return (position_ < _size and _buffer != nullptr) ? _buffer[position_] : throw Error(Error::Type::OUT_OF_RANGE_ERROR); }
        reference unchecked_at(const size_type position_) { return _buffer[position_]; }
        const_reference unchecked_at(const size_type position_) const { return _buffer[position_]; }
        value_type* data() noexcept { return _buffer.get(); }
        const value_type* data() const noexcept { return _buffer.get(); }
        void swap(ByteBuffer& byteBuffer_) noexcept { std::swap(_size, byteBuffer_._size); std::swap(_buffer, byteBuffer_._buffer); }
        friend void swap(ByteBuffer& first_, ByteBuffer& second_) noexcept { std::swap(first_._size, second_._size); std::swap(first_._buffer, second_._buffer); }
        iterator begin() noexcept { return iterator(&_buffer[0]); }
        iterator end() noexcept { return iterator(&_buffer[_size]); }
        const_iterator begin() const noexcept { return const_iterator(&_buffer[0]); }
        const_iterator end() const noexcept { return const_iterator(&_buffer[_size]); }
        const_iterator cbegin() const noexcept { return const_iterator(&_buffer[0]); }
        const_iterator cend() const noexcept { return const_iterator(&_buffer[_size]); }
        reverse_iterator rbegin() noexcept { return reverse_iterator(&_buffer[(_size > 0) ? (_size - 1) : -1]); }
        reverse_iterator rend() noexcept { return reverse_iterator(&_buffer[-1]); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(&_buffer[(_size > 0) ? (_size - 1) : -1]); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(&_buffer[-1]); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(&_buffer[(_size > 0) ? (_size - 1) : -1]); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(&_buffer[-1]); }

        // clang-format on

    private:
        SizeType _size;
        std::unique_ptr<ValueType[]> _buffer;
    };

    /// @brief How many bits there are in a char on this platform
    constexpr int charSize = std::numeric_limits<unsigned char>::digits;

    /// @brief A namespace that has functions that implement Base16 encoding and decoding in accordance to RFC 4648 §8.
    namespace Base16
    {
        /// @brief A simple error class for the Base16 namespace.
        class Error : public std::exception
        {
        public:
            /// @brief The type of Error.
            enum class Type
            {
                INTERNAL_STRING_RESERVE_ERROR, ///< Failed to reserve size to internal string.
                INVALID_CASE_ERROR,            ///< Invalid case.
                STRING_PARSE_ERROR             ///< Failed to parse string.
            };

            /**
             * @brief Creates an Error of given Type.
             * @param[in] type_ Type of Error.
             * @param[in] sourceLocation_ Source location of Error.
            */
            explicit Error(const Type type_, const std::source_location sourceLocation_ = std::source_location::current()) :
                _type(type_),
                _sourceLocation(sourceLocation_)
            {
                switch(_type) {
                    case Type::INTERNAL_STRING_RESERVE_ERROR: _what = "Faild to reserve size to internal string"; break;
                    case Type::INVALID_CASE_ERROR: _what = "Invalid case"; break;
                    case Type::STRING_PARSE_ERROR: _what = "Failed to parse string"; break;
                    default: _what = "Invalid error type"; break;
                }
            }

            /**
             * @brief Gets the Type of the Error.
             * @returns Type of Error.
            */
            Type GetType() const noexcept { return _type; }
            /**
             * @brief Gets the location at which the Error was thrown.
             * @returns Source location of Error.
            */
            std::source_location GetSourceLocation() const { return _sourceLocation; }
            /**
             * @brief Gets reason for the Error.
             * @returns Reason for the Error.
            */
            std::string What() const { return _what; }

            // For C++ compatibility purposes

            const char* what() const noexcept override { return _what.c_str(); }

        private:
            Type _type;
            std::source_location _sourceLocation;
            std::string _what;
        };

        /// @brief Case setting.
        enum class Case
        {
            LOWERCASE, ///< Lowercase (example: 7a7a).
            MIXED,     ///< Mixed (example: 7a7A).
            UPPERCASE  ///< Uppercase (example: 7A7A).
        };

        static_assert(charSize == 8, "These Base16 functions only works if a char is 8 bits big");

        /**
         * @brief Encodes a not-encoded string into a Base16 encoded string.
         * @param[in] string_ String to be encoded.
         * @param[in] case_ Case to be used (mixed case not supported).
         * @throws BinaryText::Base16::Error
        */
        std::string EncodeStringToString(const std::string& string_, const Case case_ = Case::UPPERCASE)
        {
            std::string encodedString;

            try {
                if(string_.size() > 0) {
                    encodedString.reserve(string_.size() * 2);
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(std::string::const_iterator iter(string_.cbegin()); iter != string_.cend(); ++iter) {
                std::bitset<charSize> bitset(*iter);

                for(unsigned int i(0U); i < 2U; ++i) {
                    std::bitset<charSize / 2> partialBitset;

                    if(i == 0U) {
                        partialBitset = std::bitset<charSize / 2>((bitset >> 4).to_ullong());
                    } else {
                        partialBitset = std::bitset<charSize / 2>(((bitset << 4) >> 4).to_ullong());
                    }

                    switch(case_) {
                        case Case::UPPERCASE: {
                            switch(partialBitset.to_ullong()) {
                                case 0B0000ULL: encodedString.append(1, '0'); break;
                                case 0B0001ULL: encodedString.append(1, '1'); break;
                                case 0B0010ULL: encodedString.append(1, '2'); break;
                                case 0B0011ULL: encodedString.append(1, '3'); break;
                                case 0B0100ULL: encodedString.append(1, '4'); break;
                                case 0B0101ULL: encodedString.append(1, '5'); break;
                                case 0B0110ULL: encodedString.append(1, '6'); break;
                                case 0B0111ULL: encodedString.append(1, '7'); break;
                                case 0B1000ULL: encodedString.append(1, '8'); break;
                                case 0B1001ULL: encodedString.append(1, '9'); break;
                                case 0B1010ULL: encodedString.append(1, 'A'); break;
                                case 0B1011ULL: encodedString.append(1, 'B'); break;
                                case 0B1100ULL: encodedString.append(1, 'C'); break;
                                case 0B1101ULL: encodedString.append(1, 'D'); break;
                                case 0B1110ULL: encodedString.append(1, 'E'); break;
                                case 0B1111ULL: encodedString.append(1, 'F'); break;
                                default: UnreachableTerminate();
                            }

                            break;
                        }
                        case Case::LOWERCASE: {
                            switch(partialBitset.to_ullong()) {
                                case 0B0000ULL: encodedString.append(1, '0'); break;
                                case 0B0001ULL: encodedString.append(1, '1'); break;
                                case 0B0010ULL: encodedString.append(1, '2'); break;
                                case 0B0011ULL: encodedString.append(1, '3'); break;
                                case 0B0100ULL: encodedString.append(1, '4'); break;
                                case 0B0101ULL: encodedString.append(1, '5'); break;
                                case 0B0110ULL: encodedString.append(1, '6'); break;
                                case 0B0111ULL: encodedString.append(1, '7'); break;
                                case 0B1000ULL: encodedString.append(1, '8'); break;
                                case 0B1001ULL: encodedString.append(1, '9'); break;
                                case 0B1010ULL: encodedString.append(1, 'a'); break;
                                case 0B1011ULL: encodedString.append(1, 'b'); break;
                                case 0B1100ULL: encodedString.append(1, 'c'); break;
                                case 0B1101ULL: encodedString.append(1, 'd'); break;
                                case 0B1110ULL: encodedString.append(1, 'e'); break;
                                case 0B1111ULL: encodedString.append(1, 'f'); break;
                                default: UnreachableTerminate();
                            }

                            break;
                        }
                        case Case::MIXED: throw Error(Error::Type::INVALID_CASE_ERROR);
                        default: throw Error(Error::Type::INVALID_CASE_ERROR);
                    }
                }
            }

            return encodedString;
        }
        /**
         * @brief Encodes a ByteBuffer into a Base16 encoded string.
         * @tparam ByteType Type that satisfies the ByteBufferCompatible concept (char, signed char, unsigned char and std::byte).
         * @param[in] string_ String to be encoded.
         * @param[in] case_ Case to be used (mixed case not supported).
         * @throws BinaryText::Base16::Error
         * @throws BinaryText::ByteBuffer::Error
        */
        template<ByteBufferCompatible ByteType>
        std::string EncodeByteBufferToString(const ByteBuffer<ByteType>& byteBuffer_, const Case case_ = Case::UPPERCASE)
        {
            std::string encodedString;

            try {
                if(byteBuffer_.GetSize() > 0) {
                    encodedString.reserve(byteBuffer_.GetSize() * 2);
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(typename ByteBuffer<ByteType>::ConstantIterator iter(byteBuffer_.ConstantBegin()); iter != byteBuffer_.ConstantEnd(); ++iter) {
                std::bitset<charSize> bitset(static_cast<char>(*iter));

                for(unsigned int i(0U); i < 2U; ++i) {
                    std::bitset<charSize / 2> partialBitset;

                    if(i == 0U) {
                        partialBitset = std::bitset<charSize / 2>((bitset >> 4).to_ullong());
                    } else {
                        partialBitset = std::bitset<charSize / 2>(((bitset << 4) >> 4).to_ullong());
                    }

                    switch(case_) {
                        case Case::UPPERCASE: {
                            switch(partialBitset.to_ullong()) {
                                case 0B0000ULL: encodedString.append(1, '0'); break;
                                case 0B0001ULL: encodedString.append(1, '1'); break;
                                case 0B0010ULL: encodedString.append(1, '2'); break;
                                case 0B0011ULL: encodedString.append(1, '3'); break;
                                case 0B0100ULL: encodedString.append(1, '4'); break;
                                case 0B0101ULL: encodedString.append(1, '5'); break;
                                case 0B0110ULL: encodedString.append(1, '6'); break;
                                case 0B0111ULL: encodedString.append(1, '7'); break;
                                case 0B1000ULL: encodedString.append(1, '8'); break;
                                case 0B1001ULL: encodedString.append(1, '9'); break;
                                case 0B1010ULL: encodedString.append(1, 'A'); break;
                                case 0B1011ULL: encodedString.append(1, 'B'); break;
                                case 0B1100ULL: encodedString.append(1, 'C'); break;
                                case 0B1101ULL: encodedString.append(1, 'D'); break;
                                case 0B1110ULL: encodedString.append(1, 'E'); break;
                                case 0B1111ULL: encodedString.append(1, 'F'); break;
                                default: UnreachableTerminate();
                            }

                            break;
                        }
                        case Case::LOWERCASE: {
                            switch(partialBitset.to_ullong()) {
                                case 0B0000ULL: encodedString.append(1, '0'); break;
                                case 0B0001ULL: encodedString.append(1, '1'); break;
                                case 0B0010ULL: encodedString.append(1, '2'); break;
                                case 0B0011ULL: encodedString.append(1, '3'); break;
                                case 0B0100ULL: encodedString.append(1, '4'); break;
                                case 0B0101ULL: encodedString.append(1, '5'); break;
                                case 0B0110ULL: encodedString.append(1, '6'); break;
                                case 0B0111ULL: encodedString.append(1, '7'); break;
                                case 0B1000ULL: encodedString.append(1, '8'); break;
                                case 0B1001ULL: encodedString.append(1, '9'); break;
                                case 0B1010ULL: encodedString.append(1, 'a'); break;
                                case 0B1011ULL: encodedString.append(1, 'b'); break;
                                case 0B1100ULL: encodedString.append(1, 'c'); break;
                                case 0B1101ULL: encodedString.append(1, 'd'); break;
                                case 0B1110ULL: encodedString.append(1, 'e'); break;
                                case 0B1111ULL: encodedString.append(1, 'f'); break;
                                default: UnreachableTerminate();
                            }

                            break;
                        }
                        case Case::MIXED: throw Error(Error::Type::INVALID_CASE_ERROR);
                        default: throw Error(Error::Type::INVALID_CASE_ERROR);
                    }
                }
            }

            return encodedString;
        }
        /**
         * @brief Decodes a Base16 encoded string into a decoded string. Whitespace and newline characters are ignored.
         * @param[in] encodedString_ String to be decoded.
         * @param[in] case_ Case to be used. The default is mixed case.
         * @throws BinaryText::Base16::Error
        */
        std::string DecodeStringToString(const std::string& encodedString_, const Case case_ = Case::MIXED)
        {
            std::string decodedString;

            try {
                if(encodedString_.size() > 0) {
                    decodedString.reserve(encodedString_.size() / 2);
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(std::string::const_iterator iter(encodedString_.cbegin()); iter != encodedString_.cend(); ++iter) {
                if(*iter == ' ' or *iter == '\n') {
                    continue;
                } else {
                    std::bitset<charSize> bitset;
                    unsigned int counter(0U);

                    for(std::string::const_iterator jter(iter); jter != encodedString_.cend(); ++jter) {
                        if(*jter == ' ' or *jter == '\n') {
                            if(std::next(jter, 1) == encodedString_.cend()) {
                                break;
                            } else {
                                std::advance(iter, 1);

                                continue;
                            }
                        } else {
                            counter += 1U;
                            bitset <<= charSize / 2;

                            switch(case_) {
                                case Case::MIXED: {
                                    switch(*jter) {
                                        case '0': bitset |= std::bitset<charSize>(0B0000ULL); break;
                                        case '1': bitset |= std::bitset<charSize>(0B0001ULL); break;
                                        case '2': bitset |= std::bitset<charSize>(0B0010ULL); break;
                                        case '3': bitset |= std::bitset<charSize>(0B0011ULL); break;
                                        case '4': bitset |= std::bitset<charSize>(0B0100ULL); break;
                                        case '5': bitset |= std::bitset<charSize>(0B0101ULL); break;
                                        case '6': bitset |= std::bitset<charSize>(0B0110ULL); break;
                                        case '7': bitset |= std::bitset<charSize>(0B0111ULL); break;
                                        case '8': bitset |= std::bitset<charSize>(0B1000ULL); break;
                                        case '9': bitset |= std::bitset<charSize>(0B1001ULL); break;
                                        case 'A': bitset |= std::bitset<charSize>(0B1010ULL); break;
                                        case 'B': bitset |= std::bitset<charSize>(0B1011ULL); break;
                                        case 'C': bitset |= std::bitset<charSize>(0B1100ULL); break;
                                        case 'D': bitset |= std::bitset<charSize>(0B1101ULL); break;
                                        case 'E': bitset |= std::bitset<charSize>(0B1110ULL); break;
                                        case 'F': bitset |= std::bitset<charSize>(0B1111ULL); break;
                                        case 'a': bitset |= std::bitset<charSize>(0B1010ULL); break;
                                        case 'b': bitset |= std::bitset<charSize>(0B1011ULL); break;
                                        case 'c': bitset |= std::bitset<charSize>(0B1100ULL); break;
                                        case 'd': bitset |= std::bitset<charSize>(0B1101ULL); break;
                                        case 'e': bitset |= std::bitset<charSize>(0B1110ULL); break;
                                        case 'f': bitset |= std::bitset<charSize>(0B1111ULL); break;
                                        default: throw Error(Error::Type::STRING_PARSE_ERROR);
                                    }

                                    break;
                                }
                                case Case::UPPERCASE: {
                                    switch(*jter) {
                                        case '0': bitset |= std::bitset<charSize>(0B0000ULL); break;
                                        case '1': bitset |= std::bitset<charSize>(0B0001ULL); break;
                                        case '2': bitset |= std::bitset<charSize>(0B0010ULL); break;
                                        case '3': bitset |= std::bitset<charSize>(0B0011ULL); break;
                                        case '4': bitset |= std::bitset<charSize>(0B0100ULL); break;
                                        case '5': bitset |= std::bitset<charSize>(0B0101ULL); break;
                                        case '6': bitset |= std::bitset<charSize>(0B0110ULL); break;
                                        case '7': bitset |= std::bitset<charSize>(0B0111ULL); break;
                                        case '8': bitset |= std::bitset<charSize>(0B1000ULL); break;
                                        case '9': bitset |= std::bitset<charSize>(0B1001ULL); break;
                                        case 'A': bitset |= std::bitset<charSize>(0B1010ULL); break;
                                        case 'B': bitset |= std::bitset<charSize>(0B1011ULL); break;
                                        case 'C': bitset |= std::bitset<charSize>(0B1100ULL); break;
                                        case 'D': bitset |= std::bitset<charSize>(0B1101ULL); break;
                                        case 'E': bitset |= std::bitset<charSize>(0B1110ULL); break;
                                        case 'F': bitset |= std::bitset<charSize>(0B1111ULL); break;
                                        default: throw Error(Error::Type::STRING_PARSE_ERROR);
                                    }

                                    break;
                                }
                                case Case::LOWERCASE: {
                                    switch(*jter) {
                                        case '0': bitset |= std::bitset<charSize>(0B0000ULL); break;
                                        case '1': bitset |= std::bitset<charSize>(0B0001ULL); break;
                                        case '2': bitset |= std::bitset<charSize>(0B0010ULL); break;
                                        case '3': bitset |= std::bitset<charSize>(0B0011ULL); break;
                                        case '4': bitset |= std::bitset<charSize>(0B0100ULL); break;
                                        case '5': bitset |= std::bitset<charSize>(0B0101ULL); break;
                                        case '6': bitset |= std::bitset<charSize>(0B0110ULL); break;
                                        case '7': bitset |= std::bitset<charSize>(0B0111ULL); break;
                                        case '8': bitset |= std::bitset<charSize>(0B1000ULL); break;
                                        case '9': bitset |= std::bitset<charSize>(0B1001ULL); break;
                                        case 'a': bitset |= std::bitset<charSize>(0B1010ULL); break;
                                        case 'b': bitset |= std::bitset<charSize>(0B1011ULL); break;
                                        case 'c': bitset |= std::bitset<charSize>(0B1100ULL); break;
                                        case 'd': bitset |= std::bitset<charSize>(0B1101ULL); break;
                                        case 'e': bitset |= std::bitset<charSize>(0B1110ULL); break;
                                        case 'f': bitset |= std::bitset<charSize>(0B1111ULL); break;
                                        default: throw Error(Error::Type::STRING_PARSE_ERROR);
                                    }

                                    break;
                                }
                                default: throw Error(Error::Type::INVALID_CASE_ERROR);
                            }

                            if(std::next(jter, 1) == encodedString_.cend() or std::next(jter, 1) == std::next(iter, 2)) {
                                iter = jter;

                                break;
                            }
                        }
                    }

                    switch(counter) {
                        case 2U: break;
                        case 1U: bitset <<= charSize / 2; break;
                        default: UnreachableTerminate();
                    }

                    decodedString.append(1, static_cast<char>(static_cast<unsigned char>(bitset.to_ullong())));
                }
            }

            return decodedString;
        }
        /**
         * @brief Decodes a Base16 encoded string into a decoded ByteBuffer. Whitespace and newline characters are ignored.
         * @tparam ByteType Type that satisfies the ByteBufferCompatible concept (char, signed char, unsigned char and std::byte).
         * @param[in] encodedString_ String to be decoded.
         * @param[in] case_ Case to be used. The default is mixed case.
         * @throws BinaryText::Base16::Error
         * @throws BinaryText::ByteBuffer::Error
        */
        template<ByteBufferCompatible ByteType>
        ByteBuffer<ByteType> DecodeStringToByteBuffer(const std::string& encodedString_, const Case case_ = Case::MIXED)
        {
            ByteBuffer<ByteType> decodedByteBuffer;
            ByteBuffer<ByteType> byteBuffer(8192);
            typename ByteBuffer<ByteType>::SizeType bufferIterator(0);

            for(std::string::const_iterator iter(encodedString_.cbegin()); iter != encodedString_.cend(); ++iter) {
                if(*iter == ' ' or *iter == '\n') {
                    continue;
                } else {
                    std::bitset<charSize> bitset;
                    unsigned int counter(0U);

                    for(std::string::const_iterator jter(iter); jter != encodedString_.cend(); ++jter) {
                        if(*jter == ' ' or *jter == '\n') {
                            if(std::next(jter, 1) == encodedString_.cend()) {
                                break;
                            } else {
                                std::advance(iter, 1);

                                continue;
                            }
                        } else {
                            counter += 1U;
                            bitset <<= charSize / 2;

                            switch(case_) {
                                case Case::MIXED: {
                                    switch(*jter) {
                                        case '0': bitset |= std::bitset<charSize>(0B0000ULL); break;
                                        case '1': bitset |= std::bitset<charSize>(0B0001ULL); break;
                                        case '2': bitset |= std::bitset<charSize>(0B0010ULL); break;
                                        case '3': bitset |= std::bitset<charSize>(0B0011ULL); break;
                                        case '4': bitset |= std::bitset<charSize>(0B0100ULL); break;
                                        case '5': bitset |= std::bitset<charSize>(0B0101ULL); break;
                                        case '6': bitset |= std::bitset<charSize>(0B0110ULL); break;
                                        case '7': bitset |= std::bitset<charSize>(0B0111ULL); break;
                                        case '8': bitset |= std::bitset<charSize>(0B1000ULL); break;
                                        case '9': bitset |= std::bitset<charSize>(0B1001ULL); break;
                                        case 'A': bitset |= std::bitset<charSize>(0B1010ULL); break;
                                        case 'B': bitset |= std::bitset<charSize>(0B1011ULL); break;
                                        case 'C': bitset |= std::bitset<charSize>(0B1100ULL); break;
                                        case 'D': bitset |= std::bitset<charSize>(0B1101ULL); break;
                                        case 'E': bitset |= std::bitset<charSize>(0B1110ULL); break;
                                        case 'F': bitset |= std::bitset<charSize>(0B1111ULL); break;
                                        case 'a': bitset |= std::bitset<charSize>(0B1010ULL); break;
                                        case 'b': bitset |= std::bitset<charSize>(0B1011ULL); break;
                                        case 'c': bitset |= std::bitset<charSize>(0B1100ULL); break;
                                        case 'd': bitset |= std::bitset<charSize>(0B1101ULL); break;
                                        case 'e': bitset |= std::bitset<charSize>(0B1110ULL); break;
                                        case 'f': bitset |= std::bitset<charSize>(0B1111ULL); break;
                                        default: throw Error(Error::Type::STRING_PARSE_ERROR);
                                    }

                                    break;
                                }
                                case Case::UPPERCASE: {
                                    switch(*jter) {
                                        case '0': bitset |= std::bitset<charSize>(0B0000ULL); break;
                                        case '1': bitset |= std::bitset<charSize>(0B0001ULL); break;
                                        case '2': bitset |= std::bitset<charSize>(0B0010ULL); break;
                                        case '3': bitset |= std::bitset<charSize>(0B0011ULL); break;
                                        case '4': bitset |= std::bitset<charSize>(0B0100ULL); break;
                                        case '5': bitset |= std::bitset<charSize>(0B0101ULL); break;
                                        case '6': bitset |= std::bitset<charSize>(0B0110ULL); break;
                                        case '7': bitset |= std::bitset<charSize>(0B0111ULL); break;
                                        case '8': bitset |= std::bitset<charSize>(0B1000ULL); break;
                                        case '9': bitset |= std::bitset<charSize>(0B1001ULL); break;
                                        case 'A': bitset |= std::bitset<charSize>(0B1010ULL); break;
                                        case 'B': bitset |= std::bitset<charSize>(0B1011ULL); break;
                                        case 'C': bitset |= std::bitset<charSize>(0B1100ULL); break;
                                        case 'D': bitset |= std::bitset<charSize>(0B1101ULL); break;
                                        case 'E': bitset |= std::bitset<charSize>(0B1110ULL); break;
                                        case 'F': bitset |= std::bitset<charSize>(0B1111ULL); break;
                                        default: throw Error(Error::Type::STRING_PARSE_ERROR);
                                    }

                                    break;
                                }
                                case Case::LOWERCASE: {
                                    switch(*jter) {
                                        case '0': bitset |= std::bitset<charSize>(0B0000ULL); break;
                                        case '1': bitset |= std::bitset<charSize>(0B0001ULL); break;
                                        case '2': bitset |= std::bitset<charSize>(0B0010ULL); break;
                                        case '3': bitset |= std::bitset<charSize>(0B0011ULL); break;
                                        case '4': bitset |= std::bitset<charSize>(0B0100ULL); break;
                                        case '5': bitset |= std::bitset<charSize>(0B0101ULL); break;
                                        case '6': bitset |= std::bitset<charSize>(0B0110ULL); break;
                                        case '7': bitset |= std::bitset<charSize>(0B0111ULL); break;
                                        case '8': bitset |= std::bitset<charSize>(0B1000ULL); break;
                                        case '9': bitset |= std::bitset<charSize>(0B1001ULL); break;
                                        case 'a': bitset |= std::bitset<charSize>(0B1010ULL); break;
                                        case 'b': bitset |= std::bitset<charSize>(0B1011ULL); break;
                                        case 'c': bitset |= std::bitset<charSize>(0B1100ULL); break;
                                        case 'd': bitset |= std::bitset<charSize>(0B1101ULL); break;
                                        case 'e': bitset |= std::bitset<charSize>(0B1110ULL); break;
                                        case 'f': bitset |= std::bitset<charSize>(0B1111ULL); break;
                                        default: throw Error(Error::Type::STRING_PARSE_ERROR);
                                    }

                                    break;
                                }
                                default: throw Error(Error::Type::INVALID_CASE_ERROR);
                            }

                            if(std::next(jter, 1) == encodedString_.cend() or std::next(jter, 1) == std::next(iter, 2)) {
                                iter = jter;

                                break;
                            }
                        }
                    }

                    switch(counter) {
                        case 2U: break;
                        case 1U: bitset <<= charSize / 2; break;
                        default: UnreachableTerminate();
                    }

                    if(bufferIterator < 8192) {
                        if constexpr(std::is_same_v<ByteType, unsigned char>) {
                            byteBuffer.At(bufferIterator) = static_cast<unsigned char>(bitset.to_ullong());
                        } else {
                            byteBuffer.At(bufferIterator) = static_cast<ByteType>(static_cast<unsigned char>(bitset.to_ullong()));
                        }

                        bufferIterator += 1;
                    } else {
                        decodedByteBuffer += byteBuffer;
                        bufferIterator = 0;

                        if constexpr(std::is_same_v<ByteType, unsigned char>) {
                            byteBuffer.At(bufferIterator) = static_cast<unsigned char>(bitset.to_ullong());
                        } else {
                            byteBuffer.At(bufferIterator) = static_cast<ByteType>(static_cast<unsigned char>(bitset.to_ullong()));
                        }

                        bufferIterator += 1;
                    }
                }
            }

            if(bufferIterator > 0) {
                byteBuffer.Resize(bufferIterator);

                decodedByteBuffer += byteBuffer;
            }

            return decodedByteBuffer;
        }
    };

    /// @brief A namespace that has functions that implement Base32 encoding and decoding in accordance to RFC 4648 §6.
    namespace Base32
    {
        /// @brief A simple error class for the Base32 namespace.
        class Error : public std::exception
        {
        public:
            /// @brief The type of Error.
            enum class Type
            {
                INTERNAL_STRING_RESERVE_ERROR, ///< Failed to reserve size to internal string.
                STRING_PARSE_ERROR             ///< Failed to parse string.
            };

            /**
             * @brief Creates an Error of given Type.
             * @param[in] type_ Type of Error.
             * @param[in] sourceLocation_ Source location of Error.
            */
            explicit Error(const Type type_, const std::source_location sourceLocation_ = std::source_location::current()) :
                _type(type_),
                _sourceLocation(sourceLocation_)
            {
                switch(_type) {
                    case Type::INTERNAL_STRING_RESERVE_ERROR: _what = "Faild to reserve size to internal string"; break;
                    case Type::STRING_PARSE_ERROR: _what = "Failed to parse string"; break;
                    default: _what = "Invalid error type"; break;
                }
            }

            /**
             * @brief Gets the Type of the Error.
             * @returns Type of Error.
            */
            Type GetType() const noexcept { return _type; }
            /**
             * @brief Gets the location at which the Error was thrown.
             * @returns Source location of Error.
            */
            std::source_location GetSourceLocation() const { return _sourceLocation; }
            /**
             * @brief Gets reason for the Error.
             * @returns Reason for the Error.
            */
            std::string What() const { return _what; }

            // For C++ compatibility purposes

            const char* what() const noexcept override { return _what.c_str(); }

        private:
            Type _type;
            std::source_location _sourceLocation;
            std::string _what;
        };

        static_assert(charSize == 8, "These Base32 functions only works if a char is 8 bits big");

        /**
         * @brief Encodes a not-encoded string into a Base32 encoded string.
         * @param[in] string_ String to be encoded.
         * @param[in] withPadding_ Whether or not padding (the '=' character) should be included.
         * @throws BinaryText::Base32::Error
        */
        std::string EncodeStringToString(const std::string& string_, const bool withPadding_ = true)
        {
            std::string encodedString;

            try {
                if(string_.size() > 0) {
                    encodedString.reserve(static_cast<std::size_t>(std::ceil(static_cast<double>(string_.size()) / 5.0) * 8.0));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(std::string::const_iterator iter = string_.cbegin(); iter != string_.cend(); ++iter) {
                std::bitset<charSize * 5> bitset;
                unsigned int counter(0U);

                for(std::string::const_iterator jter(iter); jter != string_.cend(); ++jter) {
                    counter += 1U;
                    bitset <<= charSize;
                    bitset |= std::bitset<charSize * 5>(std::bitset<charSize>(*jter).to_ullong());

                    if(std::next(jter, 1) == string_.cend() or std::next(jter, 1) == std::next(iter, 5)) {
                        iter = jter;

                        break;
                    }
                }

                switch(counter) {
                    case 5U: counter = 8U; break;
                    case 4U: {
                        bitset <<= charSize;
                        counter = 7U;

                        break;
                    }
                    case 3U: {
                        bitset <<= (charSize * 2);
                        counter = 5U;

                        break;
                    }
                    case 2U: {
                        bitset <<= (charSize * 3);
                        counter = 4U;

                        break;
                    }
                    case 1U: {
                        bitset <<= (charSize * 4);
                        counter = 2U;

                        break;
                    }
                    default: UnreachableTerminate();
                }

                for(unsigned int i(1U); i < 9U; ++i) {
                    if(i > counter) {
                        if(withPadding_) {
                            encodedString.append(1, '=');
                        } else {
                            break;
                        }
                    } else {
                        std::bitset<(charSize * 5) / 8> partialBitset((bitset >> ((charSize * 5) - (5 * static_cast<std::size_t>(i)))).to_ullong());

                        switch(partialBitset.to_ullong()) {
                            case 0B00000ULL: encodedString.append(1, 'A'); break;
                            case 0B00001ULL: encodedString.append(1, 'B'); break;
                            case 0B00010ULL: encodedString.append(1, 'C'); break;
                            case 0B00011ULL: encodedString.append(1, 'D'); break;
                            case 0B00100ULL: encodedString.append(1, 'E'); break;
                            case 0B00101ULL: encodedString.append(1, 'F'); break;
                            case 0B00110ULL: encodedString.append(1, 'G'); break;
                            case 0B00111ULL: encodedString.append(1, 'H'); break;
                            case 0B01000ULL: encodedString.append(1, 'I'); break;
                            case 0B01001ULL: encodedString.append(1, 'J'); break;
                            case 0B01010ULL: encodedString.append(1, 'K'); break;
                            case 0B01011ULL: encodedString.append(1, 'L'); break;
                            case 0B01100ULL: encodedString.append(1, 'M'); break;
                            case 0B01101ULL: encodedString.append(1, 'N'); break;
                            case 0B01110ULL: encodedString.append(1, 'O'); break;
                            case 0B01111ULL: encodedString.append(1, 'P'); break;
                            case 0B10000ULL: encodedString.append(1, 'Q'); break;
                            case 0B10001ULL: encodedString.append(1, 'R'); break;
                            case 0B10010ULL: encodedString.append(1, 'S'); break;
                            case 0B10011ULL: encodedString.append(1, 'T'); break;
                            case 0B10100ULL: encodedString.append(1, 'U'); break;
                            case 0B10101ULL: encodedString.append(1, 'V'); break;
                            case 0B10110ULL: encodedString.append(1, 'W'); break;
                            case 0B10111ULL: encodedString.append(1, 'X'); break;
                            case 0B11000ULL: encodedString.append(1, 'Y'); break;
                            case 0B11001ULL: encodedString.append(1, 'Z'); break;
                            case 0B11010ULL: encodedString.append(1, '2'); break;
                            case 0B11011ULL: encodedString.append(1, '3'); break;
                            case 0B11100ULL: encodedString.append(1, '4'); break;
                            case 0B11101ULL: encodedString.append(1, '5'); break;
                            case 0B11110ULL: encodedString.append(1, '6'); break;
                            case 0B11111ULL: encodedString.append(1, '7'); break;
                            default: UnreachableTerminate();
                        }
                    }
                }
            }

            return encodedString;
        }
        /**
         * @brief Encodes a ByteBuffer into a Base32 encoded string.
         * @tparam ByteType Type that satisfies the ByteBufferCompatible concept (char, signed char, unsigned char and std::byte).
         * @param[in] string_ String to be encoded.
         * @param[in] withPadding_ Whether or not padding (the '=' character) should be included.
         * @throws BinaryText::Base32::Error
         * @throws BinaryText::ByteBuffer::Error
        */
        template<ByteBufferCompatible ByteType>
        std::string EncodeByteBufferToString(const ByteBuffer<ByteType>& byteBuffer_, const bool withPadding_ = true)
        {
            std::string encodedString;

            try {
                if(byteBuffer_.GetSize() > 0) {
                    encodedString.reserve(static_cast<std::size_t>(std::ceil(static_cast<double>(byteBuffer_.GetSize()) / 5.0) * 8.0));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(typename ByteBuffer<ByteType>::ConstantIterator iter(byteBuffer_.ConstantBegin()); iter != byteBuffer_.ConstantEnd(); ++iter) {
                std::bitset<charSize * 5> bitset;
                unsigned int counter(0U);

                for(typename ByteBuffer<ByteType>::ConstantIterator jter(iter); jter != byteBuffer_.ConstantEnd(); ++jter) {
                    counter += 1U;
                    bitset <<= charSize;
                    bitset |= std::bitset<charSize * 5>(std::bitset<charSize>(static_cast<char>(*jter)).to_ullong());

                    if(std::next(jter, 1) == byteBuffer_.ConstantEnd() or std::next(jter, 1) == std::next(iter, 5)) {
                        iter = jter;

                        break;
                    }
                }

                switch(counter) {
                    case 5U: counter = 8U; break;
                    case 4U: {
                        bitset <<= charSize;
                        counter = 7U;

                        break;
                    }
                    case 3U: {
                        bitset <<= (charSize * 2);
                        counter = 5U;

                        break;
                    }
                    case 2U: {
                        bitset <<= (charSize * 3);
                        counter = 4U;

                        break;
                    }
                    case 1U: {
                        bitset <<= (charSize * 4);
                        counter = 2U;

                        break;
                    }
                    default: UnreachableTerminate();
                }

                for(unsigned int i(1U); i < 9U; ++i) {
                    if(i > counter) {
                        if(withPadding_) {
                            encodedString.append(1, '=');
                        } else {
                            break;
                        }
                    } else {
                        std::bitset<(charSize) * 5 / 8> partialBitset((bitset >> ((charSize * 5) - (5 * i))).to_ullong());

                        switch(partialBitset.to_ullong()) {
                            case 0B00000ULL: encodedString.append(1, 'A'); break;
                            case 0B00001ULL: encodedString.append(1, 'B'); break;
                            case 0B00010ULL: encodedString.append(1, 'C'); break;
                            case 0B00011ULL: encodedString.append(1, 'D'); break;
                            case 0B00100ULL: encodedString.append(1, 'E'); break;
                            case 0B00101ULL: encodedString.append(1, 'F'); break;
                            case 0B00110ULL: encodedString.append(1, 'G'); break;
                            case 0B00111ULL: encodedString.append(1, 'H'); break;
                            case 0B01000ULL: encodedString.append(1, 'I'); break;
                            case 0B01001ULL: encodedString.append(1, 'J'); break;
                            case 0B01010ULL: encodedString.append(1, 'K'); break;
                            case 0B01011ULL: encodedString.append(1, 'L'); break;
                            case 0B01100ULL: encodedString.append(1, 'M'); break;
                            case 0B01101ULL: encodedString.append(1, 'N'); break;
                            case 0B01110ULL: encodedString.append(1, 'O'); break;
                            case 0B01111ULL: encodedString.append(1, 'P'); break;
                            case 0B10000ULL: encodedString.append(1, 'Q'); break;
                            case 0B10001ULL: encodedString.append(1, 'R'); break;
                            case 0B10010ULL: encodedString.append(1, 'S'); break;
                            case 0B10011ULL: encodedString.append(1, 'T'); break;
                            case 0B10100ULL: encodedString.append(1, 'U'); break;
                            case 0B10101ULL: encodedString.append(1, 'V'); break;
                            case 0B10110ULL: encodedString.append(1, 'W'); break;
                            case 0B10111ULL: encodedString.append(1, 'X'); break;
                            case 0B11000ULL: encodedString.append(1, 'Y'); break;
                            case 0B11001ULL: encodedString.append(1, 'Z'); break;
                            case 0B11010ULL: encodedString.append(1, '2'); break;
                            case 0B11011ULL: encodedString.append(1, '3'); break;
                            case 0B11100ULL: encodedString.append(1, '4'); break;
                            case 0B11101ULL: encodedString.append(1, '5'); break;
                            case 0B11110ULL: encodedString.append(1, '6'); break;
                            case 0B11111ULL: encodedString.append(1, '7'); break;
                            default: UnreachableTerminate();
                        }
                    }
                }
            }

            return encodedString;
        }
        /**
         * @brief Decodes a Base32 encoded string into a decoded string. Whitespace and newline characters are not ignored.
         * @param[in] encodedString_ String to be decoded.
         * @throws BinaryText::Base32::Error
        */
        std::string DecodeStringToString(const std::string& encodedString_)
        {
            std::string decodedString;

            try {
                if(encodedString_.size() > 0) {
                    decodedString.reserve(static_cast<std::size_t>(std::ceil(static_cast<double>(encodedString_.size()) / 8.0) * 5.0));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(std::string::const_iterator iter(encodedString_.cbegin()); iter != encodedString_.cend(); ++iter) {
                std::bitset<charSize * 5> bitset;
                unsigned int paddingCounter(0U);
                unsigned int loopCounter(0U);

                for(std::string::const_iterator jter(iter); jter != encodedString_.cend(); ++jter) {
                    loopCounter += 1U;
                    bitset <<= (charSize * 5) / 8;

                    if(paddingCounter > 0U and *jter != '=') {
                        throw Error(Error::Type::STRING_PARSE_ERROR);
                    } else {
                        switch(*jter) {
                            case 'A': bitset |= std::bitset<charSize * 5>(0B00000ULL); break;
                            case 'B': bitset |= std::bitset<charSize * 5>(0B00001ULL); break;
                            case 'C': bitset |= std::bitset<charSize * 5>(0B00010ULL); break;
                            case 'D': bitset |= std::bitset<charSize * 5>(0B00011ULL); break;
                            case 'E': bitset |= std::bitset<charSize * 5>(0B00100ULL); break;
                            case 'F': bitset |= std::bitset<charSize * 5>(0B00101ULL); break;
                            case 'G': bitset |= std::bitset<charSize * 5>(0B00110ULL); break;
                            case 'H': bitset |= std::bitset<charSize * 5>(0B00111ULL); break;
                            case 'I': bitset |= std::bitset<charSize * 5>(0B01000ULL); break;
                            case 'J': bitset |= std::bitset<charSize * 5>(0B01001ULL); break;
                            case 'K': bitset |= std::bitset<charSize * 5>(0B01010ULL); break;
                            case 'L': bitset |= std::bitset<charSize * 5>(0B01011ULL); break;
                            case 'M': bitset |= std::bitset<charSize * 5>(0B01100ULL); break;
                            case 'N': bitset |= std::bitset<charSize * 5>(0B01101ULL); break;
                            case 'O': bitset |= std::bitset<charSize * 5>(0B01110ULL); break;
                            case 'P': bitset |= std::bitset<charSize * 5>(0B01111ULL); break;
                            case 'Q': bitset |= std::bitset<charSize * 5>(0B10000ULL); break;
                            case 'R': bitset |= std::bitset<charSize * 5>(0B10001ULL); break;
                            case 'S': bitset |= std::bitset<charSize * 5>(0B10010ULL); break;
                            case 'T': bitset |= std::bitset<charSize * 5>(0B10011ULL); break;
                            case 'U': bitset |= std::bitset<charSize * 5>(0B10100ULL); break;
                            case 'V': bitset |= std::bitset<charSize * 5>(0B10101ULL); break;
                            case 'W': bitset |= std::bitset<charSize * 5>(0B10110ULL); break;
                            case 'X': bitset |= std::bitset<charSize * 5>(0B10111ULL); break;
                            case 'Y': bitset |= std::bitset<charSize * 5>(0B11000ULL); break;
                            case 'Z': bitset |= std::bitset<charSize * 5>(0B11001ULL); break;
                            case '2': bitset |= std::bitset<charSize * 5>(0B11010ULL); break;
                            case '3': bitset |= std::bitset<charSize * 5>(0B11011ULL); break;
                            case '4': bitset |= std::bitset<charSize * 5>(0B11100ULL); break;
                            case '5': bitset |= std::bitset<charSize * 5>(0B11101ULL); break;
                            case '6': bitset |= std::bitset<charSize * 5>(0B11110ULL); break;
                            case '7': bitset |= std::bitset<charSize * 5>(0B11111ULL); break;
                            case '=': paddingCounter += 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }
                    }

                    if(std::next(jter, 1) == encodedString_.cend() or std::next(jter, 1) == std::next(iter, 8)) {
                        iter = jter;

                        break;
                    }
                }

                switch(loopCounter) {
                    case 8U: break;
                    case 7U: {
                        bitset <<= (charSize * 5) / 8;

                        switch(paddingCounter) {
                            case 5U: paddingCounter = 6U; break;
                            case 3U: paddingCounter = 4U; break;
                            case 2U: paddingCounter = 3U; break;
                            case 0U: paddingCounter = 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 6U: {
                        bitset <<= ((charSize * 5) / 8) * 2;

                        switch(paddingCounter) {
                            case 4U: paddingCounter = 6U; break;
                            case 2U: paddingCounter = 4U; break;
                            case 1U: paddingCounter = 3U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 5U: {
                        bitset <<= ((charSize * 5) / 8) * 3;

                        switch(paddingCounter) {
                            case 3U: paddingCounter = 6U; break;
                            case 1U: paddingCounter = 4U; break;
                            case 0U: paddingCounter = 3U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 4U: {
                        bitset <<= ((charSize * 5) / 8) * 4;

                        switch(paddingCounter) {
                            case 2U: paddingCounter = 6U; break;
                            case 0U: paddingCounter = 4U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 3U: {
                        bitset <<= ((charSize * 5) / 8) * 5;

                        switch(paddingCounter) {
                            case 1U: paddingCounter = 6U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 2U: {
                        bitset <<= ((charSize * 5) / 8) * 6;

                        switch(paddingCounter) {
                            case 0U: paddingCounter = 6U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 1U: throw Error(Error::Type::STRING_PARSE_ERROR);
                    default: UnreachableTerminate();
                }

                switch(paddingCounter) {
                    case 0U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> charSize).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>(bitset.to_ullong())));

                        break;
                    }
                    case 1U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> charSize).to_ullong())));

                        break;
                    }
                    case 3U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())));

                        break;
                    }
                    case 4U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong())));

                        break;
                    }
                    case 6U: decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong()))); break;
                    default: throw Error(Error::Type::STRING_PARSE_ERROR);
                }

                if(paddingCounter > 0U) {
                    break;
                }
            }

            return decodedString;
        }
        /**
         * @brief Decodes a Base32 encoded string into a decoded ByteBuffer. Whitespace and newline characters are not ignored.
         * @tparam ByteType Type that satisfies the ByteBufferCompatible concept (char, signed char, unsigned char and std::byte).
         * @param[in] encodedString_ String to be decoded.
         * @throws BinaryText::Base32::Error
         * @throws BinaryText::ByteBuffer::Error
        */
        template<ByteBufferCompatible ByteType>
        ByteBuffer<ByteType> DecodeStringToByteBuffer(const std::string& encodedString_)
        {
            ByteBuffer<ByteType> decodedByteBuffer;
            ByteBuffer<ByteType> byteBuffer(8192);
            typename ByteBuffer<ByteType>::SizeType bufferIterator(0);
            auto addToByteBuffer = [&decodedByteBuffer, &byteBuffer, &bufferIterator](const unsigned char byte_) -> void {
                if(bufferIterator < 8192) {
                    if constexpr(std::is_same_v<ByteType, unsigned char>) {
                        byteBuffer.At(bufferIterator) = byte_;
                    } else {
                        byteBuffer.At(bufferIterator) = static_cast<ByteType>(byte_);
                    }

                    bufferIterator += 1;
                } else {
                    decodedByteBuffer += byteBuffer;
                    bufferIterator = 0;

                    if constexpr(std::is_same_v<ByteType, unsigned char>) {
                        byteBuffer.At(bufferIterator) = byte_;
                    } else {
                        byteBuffer.At(bufferIterator) = static_cast<ByteType>(byte_);
                    }

                    bufferIterator += 1;
                }
            };

            for(std::string::const_iterator iter(encodedString_.cbegin()); iter != encodedString_.cend(); ++iter) {
                std::bitset<charSize * 5> bitset;
                unsigned int paddingCounter(0U);
                unsigned int loopCounter(0U);

                for(std::string::const_iterator jter(iter); jter != encodedString_.cend(); ++jter) {
                    loopCounter += 1U;
                    bitset <<= (charSize * 5) / 8;

                    if(paddingCounter > 0U and *jter != '=') {
                        throw Error(Error::Type::STRING_PARSE_ERROR);
                    } else {
                        switch(*jter) {
                            case 'A': bitset |= std::bitset<charSize * 5>(0B00000ULL); break;
                            case 'B': bitset |= std::bitset<charSize * 5>(0B00001ULL); break;
                            case 'C': bitset |= std::bitset<charSize * 5>(0B00010ULL); break;
                            case 'D': bitset |= std::bitset<charSize * 5>(0B00011ULL); break;
                            case 'E': bitset |= std::bitset<charSize * 5>(0B00100ULL); break;
                            case 'F': bitset |= std::bitset<charSize * 5>(0B00101ULL); break;
                            case 'G': bitset |= std::bitset<charSize * 5>(0B00110ULL); break;
                            case 'H': bitset |= std::bitset<charSize * 5>(0B00111ULL); break;
                            case 'I': bitset |= std::bitset<charSize * 5>(0B01000ULL); break;
                            case 'J': bitset |= std::bitset<charSize * 5>(0B01001ULL); break;
                            case 'K': bitset |= std::bitset<charSize * 5>(0B01010ULL); break;
                            case 'L': bitset |= std::bitset<charSize * 5>(0B01011ULL); break;
                            case 'M': bitset |= std::bitset<charSize * 5>(0B01100ULL); break;
                            case 'N': bitset |= std::bitset<charSize * 5>(0B01101ULL); break;
                            case 'O': bitset |= std::bitset<charSize * 5>(0B01110ULL); break;
                            case 'P': bitset |= std::bitset<charSize * 5>(0B01111ULL); break;
                            case 'Q': bitset |= std::bitset<charSize * 5>(0B10000ULL); break;
                            case 'R': bitset |= std::bitset<charSize * 5>(0B10001ULL); break;
                            case 'S': bitset |= std::bitset<charSize * 5>(0B10010ULL); break;
                            case 'T': bitset |= std::bitset<charSize * 5>(0B10011ULL); break;
                            case 'U': bitset |= std::bitset<charSize * 5>(0B10100ULL); break;
                            case 'V': bitset |= std::bitset<charSize * 5>(0B10101ULL); break;
                            case 'W': bitset |= std::bitset<charSize * 5>(0B10110ULL); break;
                            case 'X': bitset |= std::bitset<charSize * 5>(0B10111ULL); break;
                            case 'Y': bitset |= std::bitset<charSize * 5>(0B11000ULL); break;
                            case 'Z': bitset |= std::bitset<charSize * 5>(0B11001ULL); break;
                            case '2': bitset |= std::bitset<charSize * 5>(0B11010ULL); break;
                            case '3': bitset |= std::bitset<charSize * 5>(0B11011ULL); break;
                            case '4': bitset |= std::bitset<charSize * 5>(0B11100ULL); break;
                            case '5': bitset |= std::bitset<charSize * 5>(0B11101ULL); break;
                            case '6': bitset |= std::bitset<charSize * 5>(0B11110ULL); break;
                            case '7': bitset |= std::bitset<charSize * 5>(0B11111ULL); break;
                            case '=': paddingCounter += 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }
                    }

                    if(std::next(jter, 1) == encodedString_.cend() or std::next(jter, 1) == std::next(iter, 8)) {
                        iter = jter;

                        break;
                    }
                }

                switch(loopCounter) {
                    case 8U: break;
                    case 7U: {
                        bitset <<= (charSize * 5) / 8;

                        switch(paddingCounter) {
                            case 5U: paddingCounter = 6U; break;
                            case 3U: paddingCounter = 4U; break;
                            case 2U: paddingCounter = 3U; break;
                            case 0U: paddingCounter = 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 6U: {
                        bitset <<= ((charSize * 5) / 8) * 2;

                        switch(paddingCounter) {
                            case 4U: paddingCounter = 6U; break;
                            case 2U: paddingCounter = 4U; break;
                            case 1U: paddingCounter = 3U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 5U: {
                        bitset <<= ((charSize * 5) / 8) * 3;

                        switch(paddingCounter) {
                            case 3U: paddingCounter = 6U; break;
                            case 1U: paddingCounter = 4U; break;
                            case 0U: paddingCounter = 3U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 4U: {
                        bitset <<= ((charSize * 5) / 8) * 4;

                        switch(paddingCounter) {
                            case 2U: paddingCounter = 6U; break;
                            case 0U: paddingCounter = 4U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 3U: {
                        bitset <<= ((charSize * 5) / 8) * 5;

                        switch(paddingCounter) {
                            case 1U: paddingCounter = 6U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 2U: {
                        bitset <<= ((charSize * 5) / 8) * 6;

                        switch(paddingCounter) {
                            case 0U: paddingCounter = 6U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 1U: throw Error(Error::Type::STRING_PARSE_ERROR);
                    default: UnreachableTerminate();
                }

                switch(paddingCounter) {
                    case 0U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> charSize).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>(bitset.to_ullong()));

                        break;
                    }
                    case 1U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> charSize).to_ullong()));

                        break;
                    }
                    case 3U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()));

                        break;
                    }
                    case 4U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong()));

                        break;
                    }
                    case 6U: addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong())); break;
                    default: throw Error(Error::Type::STRING_PARSE_ERROR);
                }

                if(paddingCounter > 0U) {
                    break;
                }
            }

            if(bufferIterator > 0) {
                byteBuffer.Resize(bufferIterator);

                decodedByteBuffer += byteBuffer;
            }

            return decodedByteBuffer;
        }
    };

    /// @brief A namespace that has functions that implement Base32Hex encoding and decoding in accordance to RFC 4648 §7.
    namespace Base32Hex
    {
        /// @brief A simple error class for the Base32Hex namespace.
        class Error : public std::exception
        {
        public:
            /// @brief The type of Error.
            enum class Type
            {
                INTERNAL_STRING_RESERVE_ERROR, ///< Failed to reserve size to internal string.
                STRING_PARSE_ERROR             ///< Failed to parse string.
            };

            /**
             * @brief Creates an Error of given Type.
             * @param[in] type_ Type of Error.
             * @param[in] sourceLocation_ Source location of Error.
            */
            explicit Error(const Type type_, const std::source_location sourceLocation_ = std::source_location::current()) :
                _type(type_),
                _sourceLocation(sourceLocation_)
            {
                switch(_type) {
                    case Type::INTERNAL_STRING_RESERVE_ERROR: _what = "Faild to reserve size to internal string"; break;
                    case Type::STRING_PARSE_ERROR: _what = "Failed to parse string"; break;
                    default: _what = "Invalid error type"; break;
                }
            }

            /**
             * @brief Gets the Type of the Error.
             * @returns Type of Error.
            */
            Type GetType() const noexcept { return _type; }
            /**
             * @brief Gets the location at which the Error was thrown.
             * @returns Source location of Error.
            */
            std::source_location GetSourceLocation() const { return _sourceLocation; }
            /**
             * @brief Gets reason for the Error.
             * @returns Reason for the Error.
            */
            std::string What() const { return _what; }

            // For C++ compatibility purposes

            const char* what() const noexcept override { return _what.c_str(); }

        private:
            Type _type;
            std::source_location _sourceLocation;
            std::string _what;
        };

        static_assert(charSize == 8, "These Base32Hex functions only works if a char is 8 bits big");

        /**
         * @brief Encodes a not-encoded string into a Base32Hex encoded string.
         * @param[in] string_ String to be encoded.
         * @param[in] withPadding_ Whether or not padding (the '=' character) should be included.
         * @throws BinaryText::Base32Hex::Error
        */
        std::string EncodeStringToString(const std::string& string_, const bool withPadding_ = true)
        {
            std::string encodedString;

            try {
                if(string_.size() > 0) {
                    encodedString.reserve(static_cast<std::size_t>(std::ceil(static_cast<double>(string_.size()) / 5.0) * 8.0));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(std::string::const_iterator iter = string_.cbegin(); iter != string_.cend(); ++iter) {
                std::bitset<charSize * 5> bitset;
                unsigned int counter(0U);

                for(std::string::const_iterator jter(iter); jter != string_.cend(); ++jter) {
                    counter += 1U;
                    bitset <<= charSize;
                    bitset |= std::bitset<charSize * 5>(std::bitset<charSize>(*jter).to_ullong());

                    if(std::next(jter, 1) == string_.cend() or std::next(jter, 1) == std::next(iter, 5)) {
                        iter = jter;

                        break;
                    }
                }

                switch(counter) {
                    case 5U: counter = 8U; break;
                    case 4U: {
                        bitset <<= charSize;
                        counter = 7U;

                        break;
                    }
                    case 3U: {
                        bitset <<= (charSize * 2);
                        counter = 5U;

                        break;
                    }
                    case 2U: {
                        bitset <<= (charSize * 3);
                        counter = 4U;

                        break;
                    }
                    case 1U: {
                        bitset <<= (charSize * 4);
                        counter = 2U;

                        break;
                    }
                    default: UnreachableTerminate();
                }

                for(unsigned int i(1U); i < 9U; ++i) {
                    if(i > counter) {
                        if(withPadding_) {
                            encodedString.append(1, '=');
                        } else {
                            break;
                        }
                    } else {
                        std::bitset<(charSize * 5) / 8> partialBitset((bitset >> ((charSize * 5) - (5 * static_cast<std::size_t>(i)))).to_ullong());

                        switch(partialBitset.to_ullong()) {
                            case 0B00000ULL: encodedString.append(1, '0'); break;
                            case 0B00001ULL: encodedString.append(1, '1'); break;
                            case 0B00010ULL: encodedString.append(1, '2'); break;
                            case 0B00011ULL: encodedString.append(1, '3'); break;
                            case 0B00100ULL: encodedString.append(1, '4'); break;
                            case 0B00101ULL: encodedString.append(1, '5'); break;
                            case 0B00110ULL: encodedString.append(1, '6'); break;
                            case 0B00111ULL: encodedString.append(1, '7'); break;
                            case 0B01000ULL: encodedString.append(1, '8'); break;
                            case 0B01001ULL: encodedString.append(1, '9'); break;
                            case 0B01010ULL: encodedString.append(1, 'A'); break;
                            case 0B01011ULL: encodedString.append(1, 'B'); break;
                            case 0B01100ULL: encodedString.append(1, 'C'); break;
                            case 0B01101ULL: encodedString.append(1, 'D'); break;
                            case 0B01110ULL: encodedString.append(1, 'E'); break;
                            case 0B01111ULL: encodedString.append(1, 'F'); break;
                            case 0B10000ULL: encodedString.append(1, 'G'); break;
                            case 0B10001ULL: encodedString.append(1, 'H'); break;
                            case 0B10010ULL: encodedString.append(1, 'I'); break;
                            case 0B10011ULL: encodedString.append(1, 'J'); break;
                            case 0B10100ULL: encodedString.append(1, 'K'); break;
                            case 0B10101ULL: encodedString.append(1, 'L'); break;
                            case 0B10110ULL: encodedString.append(1, 'M'); break;
                            case 0B10111ULL: encodedString.append(1, 'N'); break;
                            case 0B11000ULL: encodedString.append(1, 'O'); break;
                            case 0B11001ULL: encodedString.append(1, 'P'); break;
                            case 0B11010ULL: encodedString.append(1, 'Q'); break;
                            case 0B11011ULL: encodedString.append(1, 'R'); break;
                            case 0B11100ULL: encodedString.append(1, 'S'); break;
                            case 0B11101ULL: encodedString.append(1, 'T'); break;
                            case 0B11110ULL: encodedString.append(1, 'U'); break;
                            case 0B11111ULL: encodedString.append(1, 'V'); break;
                            default: UnreachableTerminate();
                        }
                    }
                }
            }

            return encodedString;
        }
        /**
         * @brief Encodes a ByteBuffer into a Base32Hex encoded string.
         * @tparam ByteType Type that satisfies the ByteBufferCompatible concept (char, signed char, unsigned char and std::byte).
         * @param[in] string_ String to be encoded.
         * @param[in] withPadding_ Whether or not padding (the '=' character) should be included.
         * @throws BinaryText::Base32Hex::Error
         * @throws BinaryText::ByteBuffer::Error
        */
        template<ByteBufferCompatible ByteType>
        std::string EncodeByteBufferToString(const ByteBuffer<ByteType>& byteBuffer_, const bool withPadding_ = true)
        {
            std::string encodedString;

            try {
                if(byteBuffer_.GetSize() > 0) {
                    encodedString.reserve(static_cast<std::size_t>(std::ceil(static_cast<double>(byteBuffer_.GetSize()) / 5.0) * 8.0));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(typename ByteBuffer<ByteType>::ConstantIterator iter(byteBuffer_.ConstantBegin()); iter != byteBuffer_.ConstantEnd(); ++iter) {
                std::bitset<charSize * 5> bitset;
                unsigned int counter(0U);

                for(typename ByteBuffer<ByteType>::ConstantIterator jter(iter); jter != byteBuffer_.ConstantEnd(); ++jter) {
                    counter += 1U;
                    bitset <<= charSize;
                    bitset |= std::bitset<charSize * 5>(std::bitset<charSize>(static_cast<char>(*jter)).to_ullong());

                    if(std::next(jter, 1) == byteBuffer_.ConstantEnd() or std::next(jter, 1) == std::next(iter, 5)) {
                        iter = jter;

                        break;
                    }
                }

                switch(counter) {
                    case 5U: counter = 8U; break;
                    case 4U: {
                        bitset <<= charSize;
                        counter = 7U;

                        break;
                    }
                    case 3U: {
                        bitset <<= (charSize * 2);
                        counter = 5U;

                        break;
                    }
                    case 2U: {
                        bitset <<= (charSize * 3);
                        counter = 4U;

                        break;
                    }
                    case 1U: {
                        bitset <<= (charSize * 4);
                        counter = 2U;

                        break;
                    }
                    default: UnreachableTerminate();
                }

                for(unsigned int i(1U); i < 9U; ++i) {
                    if(i > counter) {
                        if(withPadding_) {
                            encodedString.append(1, '=');
                        } else {
                            break;
                        }
                    } else {
                        std::bitset<(charSize * 5) / 8> partialBitset((bitset >> ((charSize * 5) - (5 * i))).to_ullong());

                        switch(partialBitset.to_ullong()) {
                            case 0B00000ULL: encodedString.append(1, '0'); break;
                            case 0B00001ULL: encodedString.append(1, '1'); break;
                            case 0B00010ULL: encodedString.append(1, '2'); break;
                            case 0B00011ULL: encodedString.append(1, '3'); break;
                            case 0B00100ULL: encodedString.append(1, '4'); break;
                            case 0B00101ULL: encodedString.append(1, '5'); break;
                            case 0B00110ULL: encodedString.append(1, '6'); break;
                            case 0B00111ULL: encodedString.append(1, '7'); break;
                            case 0B01000ULL: encodedString.append(1, '8'); break;
                            case 0B01001ULL: encodedString.append(1, '9'); break;
                            case 0B01010ULL: encodedString.append(1, 'A'); break;
                            case 0B01011ULL: encodedString.append(1, 'B'); break;
                            case 0B01100ULL: encodedString.append(1, 'C'); break;
                            case 0B01101ULL: encodedString.append(1, 'D'); break;
                            case 0B01110ULL: encodedString.append(1, 'E'); break;
                            case 0B01111ULL: encodedString.append(1, 'F'); break;
                            case 0B10000ULL: encodedString.append(1, 'G'); break;
                            case 0B10001ULL: encodedString.append(1, 'H'); break;
                            case 0B10010ULL: encodedString.append(1, 'I'); break;
                            case 0B10011ULL: encodedString.append(1, 'J'); break;
                            case 0B10100ULL: encodedString.append(1, 'K'); break;
                            case 0B10101ULL: encodedString.append(1, 'L'); break;
                            case 0B10110ULL: encodedString.append(1, 'M'); break;
                            case 0B10111ULL: encodedString.append(1, 'N'); break;
                            case 0B11000ULL: encodedString.append(1, 'O'); break;
                            case 0B11001ULL: encodedString.append(1, 'P'); break;
                            case 0B11010ULL: encodedString.append(1, 'Q'); break;
                            case 0B11011ULL: encodedString.append(1, 'R'); break;
                            case 0B11100ULL: encodedString.append(1, 'S'); break;
                            case 0B11101ULL: encodedString.append(1, 'T'); break;
                            case 0B11110ULL: encodedString.append(1, 'U'); break;
                            case 0B11111ULL: encodedString.append(1, 'V'); break;
                            default: UnreachableTerminate();
                        }
                    }
                }
            }

            return encodedString;
        }
        /**
         * @brief Decodes a Base32Hex encoded string into a decoded string. Whitespace and newline characters are not ignored.
         * @param[in] encodedString_ String to be decoded.
         * @throws BinaryText::Base32Hex::Error
        */
        std::string DecodeStringToString(const std::string& encodedString_)
        {
            std::string decodedString;

            try {
                if(encodedString_.size() > 0) {
                    decodedString.reserve(static_cast<std::size_t>(std::ceil(static_cast<double>(encodedString_.size()) / 8.0) * 5.0));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(std::string::const_iterator iter(encodedString_.cbegin()); iter != encodedString_.cend(); ++iter) {
                std::bitset<charSize * 5> bitset;
                unsigned int paddingCounter(0U);
                unsigned int loopCounter(0U);

                for(std::string::const_iterator jter(iter); jter != encodedString_.cend(); ++jter) {
                    loopCounter += 1U;
                    bitset <<= (charSize * 5) / 8;

                    if(paddingCounter > 0U and *jter != '=') {
                        throw Error(Error::Type::STRING_PARSE_ERROR);
                    } else {
                        switch(*jter) {
                            case '0': bitset |= std::bitset<charSize * 5>(0B00000ULL); break;
                            case '1': bitset |= std::bitset<charSize * 5>(0B00001ULL); break;
                            case '2': bitset |= std::bitset<charSize * 5>(0B00010ULL); break;
                            case '3': bitset |= std::bitset<charSize * 5>(0B00011ULL); break;
                            case '4': bitset |= std::bitset<charSize * 5>(0B00100ULL); break;
                            case '5': bitset |= std::bitset<charSize * 5>(0B00101ULL); break;
                            case '6': bitset |= std::bitset<charSize * 5>(0B00110ULL); break;
                            case '7': bitset |= std::bitset<charSize * 5>(0B00111ULL); break;
                            case '8': bitset |= std::bitset<charSize * 5>(0B01000ULL); break;
                            case '9': bitset |= std::bitset<charSize * 5>(0B01001ULL); break;
                            case 'A': bitset |= std::bitset<charSize * 5>(0B01010ULL); break;
                            case 'B': bitset |= std::bitset<charSize * 5>(0B01011ULL); break;
                            case 'C': bitset |= std::bitset<charSize * 5>(0B01100ULL); break;
                            case 'D': bitset |= std::bitset<charSize * 5>(0B01101ULL); break;
                            case 'E': bitset |= std::bitset<charSize * 5>(0B01110ULL); break;
                            case 'F': bitset |= std::bitset<charSize * 5>(0B01111ULL); break;
                            case 'G': bitset |= std::bitset<charSize * 5>(0B10000ULL); break;
                            case 'H': bitset |= std::bitset<charSize * 5>(0B10001ULL); break;
                            case 'I': bitset |= std::bitset<charSize * 5>(0B10010ULL); break;
                            case 'J': bitset |= std::bitset<charSize * 5>(0B10011ULL); break;
                            case 'K': bitset |= std::bitset<charSize * 5>(0B10100ULL); break;
                            case 'L': bitset |= std::bitset<charSize * 5>(0B10101ULL); break;
                            case 'M': bitset |= std::bitset<charSize * 5>(0B10110ULL); break;
                            case 'N': bitset |= std::bitset<charSize * 5>(0B10111ULL); break;
                            case 'O': bitset |= std::bitset<charSize * 5>(0B11000ULL); break;
                            case 'P': bitset |= std::bitset<charSize * 5>(0B11001ULL); break;
                            case 'Q': bitset |= std::bitset<charSize * 5>(0B11010ULL); break;
                            case 'R': bitset |= std::bitset<charSize * 5>(0B11011ULL); break;
                            case 'S': bitset |= std::bitset<charSize * 5>(0B11100ULL); break;
                            case 'T': bitset |= std::bitset<charSize * 5>(0B11101ULL); break;
                            case 'U': bitset |= std::bitset<charSize * 5>(0B11110ULL); break;
                            case 'V': bitset |= std::bitset<charSize * 5>(0B11111ULL); break;
                            case '=': paddingCounter += 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }
                    }

                    if(std::next(jter, 1) == encodedString_.cend() or std::next(jter, 1) == std::next(iter, 8)) {
                        iter = jter;

                        break;
                    }
                }

                switch(loopCounter) {
                    case 8U: break;
                    case 7U: {
                        bitset <<= (charSize * 5) / 8;

                        switch(paddingCounter) {
                            case 5U: paddingCounter = 6U; break;
                            case 3U: paddingCounter = 4U; break;
                            case 2U: paddingCounter = 3U; break;
                            case 0U: paddingCounter = 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 6U: {
                        bitset <<= ((charSize * 5) / 8) * 2;

                        switch(paddingCounter) {
                            case 4U: paddingCounter = 6U; break;
                            case 2U: paddingCounter = 4U; break;
                            case 1U: paddingCounter = 3U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 5U: {
                        bitset <<= ((charSize * 5) / 8) * 3;

                        switch(paddingCounter) {
                            case 3U: paddingCounter = 6U; break;
                            case 1U: paddingCounter = 4U; break;
                            case 0U: paddingCounter = 3U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 4U: {
                        bitset <<= ((charSize * 5) / 8) * 4;

                        switch(paddingCounter) {
                            case 2U: paddingCounter = 6U; break;
                            case 0U: paddingCounter = 4U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 3U: {
                        bitset <<= ((charSize * 5) / 8) * 5;

                        switch(paddingCounter) {
                            case 1U: paddingCounter = 6U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 2U: {
                        bitset <<= ((charSize * 5) / 8) * 6;

                        switch(paddingCounter) {
                            case 0U: paddingCounter = 6U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 1U: throw Error(Error::Type::STRING_PARSE_ERROR);
                    default: UnreachableTerminate();
                }

                switch(paddingCounter) {
                    case 0U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> charSize).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>(bitset.to_ullong())));

                        break;
                    }
                    case 1U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> charSize).to_ullong())));

                        break;
                    }
                    case 3U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())));

                        break;
                    }
                    case 4U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong())));

                        break;
                    }
                    case 6U: decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong()))); break;
                    default: throw Error(Error::Type::STRING_PARSE_ERROR);
                }

                if(paddingCounter > 0U) {
                    break;
                }
            }

            return decodedString;
        }
        /**
         * @brief Decodes a Base32Hex encoded string into a decoded ByteBuffer. Whitespace and newline characters are not ignored.
         * @tparam ByteType Type that satisfies the ByteBufferCompatible concept (char, signed char, unsigned char and std::byte).
         * @param[in] encodedString_ String to be decoded.
         * @throws BinaryText::Base32Hex::Error
         * @throws BinaryText::ByteBuffer::Error
        */
        template<ByteBufferCompatible ByteType>
        ByteBuffer<ByteType> DecodeStringToByteBuffer(const std::string& encodedString_)
        {
            ByteBuffer<ByteType> decodedByteBuffer;
            ByteBuffer<ByteType> byteBuffer(8192);
            typename ByteBuffer<ByteType>::SizeType bufferIterator(0);
            auto addToByteBuffer = [&decodedByteBuffer, &byteBuffer, &bufferIterator](const unsigned char byte_) -> void {
                if(bufferIterator < 8192) {
                    if constexpr(std::is_same_v<ByteType, unsigned char>) {
                        byteBuffer.At(bufferIterator) = byte_;
                    } else {
                        byteBuffer.At(bufferIterator) = static_cast<ByteType>(byte_);
                    }

                    bufferIterator += 1;
                } else {
                    decodedByteBuffer += byteBuffer;
                    bufferIterator = 0;

                    if constexpr(std::is_same_v<ByteType, unsigned char>) {
                        byteBuffer.At(bufferIterator) = byte_;
                    } else {
                        byteBuffer.At(bufferIterator) = static_cast<ByteType>(byte_);
                    }

                    bufferIterator += 1;
                }
            };

            for(std::string::const_iterator iter(encodedString_.cbegin()); iter != encodedString_.cend(); ++iter) {
                std::bitset<charSize * 5> bitset;
                unsigned int paddingCounter(0U);
                unsigned int loopCounter(0U);

                for(std::string::const_iterator jter(iter); jter != encodedString_.cend(); ++jter) {
                    loopCounter += 1U;
                    bitset <<= (charSize * 5) / 8;

                    if(paddingCounter > 0U and *jter != '=') {
                        throw Error(Error::Type::STRING_PARSE_ERROR);
                    } else {
                        switch(*jter) {
                            case '0': bitset |= std::bitset<charSize * 5>(0B00000ULL); break;
                            case '1': bitset |= std::bitset<charSize * 5>(0B00001ULL); break;
                            case '2': bitset |= std::bitset<charSize * 5>(0B00010ULL); break;
                            case '3': bitset |= std::bitset<charSize * 5>(0B00011ULL); break;
                            case '4': bitset |= std::bitset<charSize * 5>(0B00100ULL); break;
                            case '5': bitset |= std::bitset<charSize * 5>(0B00101ULL); break;
                            case '6': bitset |= std::bitset<charSize * 5>(0B00110ULL); break;
                            case '7': bitset |= std::bitset<charSize * 5>(0B00111ULL); break;
                            case '8': bitset |= std::bitset<charSize * 5>(0B01000ULL); break;
                            case '9': bitset |= std::bitset<charSize * 5>(0B01001ULL); break;
                            case 'A': bitset |= std::bitset<charSize * 5>(0B01010ULL); break;
                            case 'B': bitset |= std::bitset<charSize * 5>(0B01011ULL); break;
                            case 'C': bitset |= std::bitset<charSize * 5>(0B01100ULL); break;
                            case 'D': bitset |= std::bitset<charSize * 5>(0B01101ULL); break;
                            case 'E': bitset |= std::bitset<charSize * 5>(0B01110ULL); break;
                            case 'F': bitset |= std::bitset<charSize * 5>(0B01111ULL); break;
                            case 'G': bitset |= std::bitset<charSize * 5>(0B10000ULL); break;
                            case 'H': bitset |= std::bitset<charSize * 5>(0B10001ULL); break;
                            case 'I': bitset |= std::bitset<charSize * 5>(0B10010ULL); break;
                            case 'J': bitset |= std::bitset<charSize * 5>(0B10011ULL); break;
                            case 'K': bitset |= std::bitset<charSize * 5>(0B10100ULL); break;
                            case 'L': bitset |= std::bitset<charSize * 5>(0B10101ULL); break;
                            case 'M': bitset |= std::bitset<charSize * 5>(0B10110ULL); break;
                            case 'N': bitset |= std::bitset<charSize * 5>(0B10111ULL); break;
                            case 'O': bitset |= std::bitset<charSize * 5>(0B11000ULL); break;
                            case 'P': bitset |= std::bitset<charSize * 5>(0B11001ULL); break;
                            case 'Q': bitset |= std::bitset<charSize * 5>(0B11010ULL); break;
                            case 'R': bitset |= std::bitset<charSize * 5>(0B11011ULL); break;
                            case 'S': bitset |= std::bitset<charSize * 5>(0B11100ULL); break;
                            case 'T': bitset |= std::bitset<charSize * 5>(0B11101ULL); break;
                            case 'U': bitset |= std::bitset<charSize * 5>(0B11110ULL); break;
                            case 'V': bitset |= std::bitset<charSize * 5>(0B11111ULL); break;
                            case '=': paddingCounter += 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }
                    }

                    if(std::next(jter, 1) == encodedString_.cend() or std::next(jter, 1) == std::next(iter, 8)) {
                        iter = jter;

                        break;
                    }
                }

                switch(loopCounter) {
                    case 8U: break;
                    case 7U: {
                        bitset <<= (charSize * 5) / 8;

                        switch(paddingCounter) {
                            case 5U: paddingCounter = 6U; break;
                            case 3U: paddingCounter = 4U; break;
                            case 2U: paddingCounter = 3U; break;
                            case 0U: paddingCounter = 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 6U: {
                        bitset <<= ((charSize * 5) / 8) * 2;

                        switch(paddingCounter) {
                            case 4U: paddingCounter = 6U; break;
                            case 2U: paddingCounter = 4U; break;
                            case 1U: paddingCounter = 3U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 5U: {
                        bitset <<= ((charSize * 5) / 8) * 3;

                        switch(paddingCounter) {
                            case 3U: paddingCounter = 6U; break;
                            case 1U: paddingCounter = 4U; break;
                            case 0U: paddingCounter = 3U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 4U: {
                        bitset <<= ((charSize * 5) / 8) * 4;

                        switch(paddingCounter) {
                            case 2U: paddingCounter = 6U; break;
                            case 0U: paddingCounter = 4U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 3U: {
                        bitset <<= ((charSize * 5) / 8) * 5;

                        switch(paddingCounter) {
                            case 1U: paddingCounter = 6U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 2U: {
                        bitset <<= ((charSize * 5) / 8) * 6;

                        switch(paddingCounter) {
                            case 0U: paddingCounter = 6U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 1U: throw Error(Error::Type::STRING_PARSE_ERROR);
                    default: UnreachableTerminate();
                }

                switch(paddingCounter) {
                    case 0U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> charSize).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>(bitset.to_ullong()));

                        break;
                    }
                    case 1U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> charSize).to_ullong()));

                        break;
                    }
                    case 3U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()));

                        break;
                    }
                    case 4U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong()));

                        break;
                    }
                    case 6U: addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 4)).to_ullong())); break;
                    default: throw Error(Error::Type::STRING_PARSE_ERROR);
                }

                if(paddingCounter > 0U) {
                    break;
                }
            }

            if(bufferIterator > 0) {
                byteBuffer.Resize(bufferIterator);

                decodedByteBuffer += byteBuffer;
            }

            return decodedByteBuffer;
        }
    };

    /// @brief A namespace that has functions that implement Base64 encoding and decoding in accordance to RFC 4648 §4.
    namespace Base64
    {
        /// @brief A simple error class for the Base64 namespace.
        class Error : public std::exception
        {
        public:
            /// @brief The type of Error.
            enum class Type
            {
                INTERNAL_STRING_RESERVE_ERROR, ///< Failed to reserve size to internal string.
                STRING_PARSE_ERROR             ///< Failed to parse string.
            };

            /**
             * @brief Creates an Error of given Type.
             * @param[in] type_ Type of Error.
             * @param[in] sourceLocation_ Source location of Error.
            */
            explicit Error(const Type type_, const std::source_location sourceLocation_ = std::source_location::current()) :
                _type(type_),
                _sourceLocation(sourceLocation_)
            {
                switch(_type) {
                    case Type::INTERNAL_STRING_RESERVE_ERROR: _what = "Faild to reserve size to internal string"; break;
                    case Type::STRING_PARSE_ERROR: _what = "Failed to parse string"; break;
                    default: _what = "Invalid error type"; break;
                }
            }

            /**
             * @brief Gets the Type of the Error.
             * @returns Type of Error.
            */
            Type GetType() const noexcept { return _type; }
            /**
             * @brief Gets the location at which the Error was thrown.
             * @returns Source location of Error.
            */
            std::source_location GetSourceLocation() const { return _sourceLocation; }
            /**
             * @brief Gets reason for the Error.
             * @returns Reason for the Error.
            */
            std::string What() const { return _what; }

            // For C++ compatibility purposes

            const char* what() const noexcept override { return _what.c_str(); }

        private:
            Type _type;
            std::source_location _sourceLocation;
            std::string _what;
        };

        static_assert(charSize == 8, "These Base64 functions only works if a char is 8 bits big");

        /**
         * @brief Encodes a not-encoded string into a Base64 encoded string.
         * @param[in] string_ String to be encoded.
         * @param[in] withPadding_ Whether or not padding (the '=' character) should be included.
         * @throws BinaryText::Base64::Error
        */
        std::string EncodeStringToString(const std::string& string_, const bool withPadding_ = true)
        {
            std::string encodedString;

            try {
                if(string_.size() > 0) {
                    encodedString.reserve(static_cast<std::size_t>(std::ceil((static_cast<double>(string_.size())) / 3.0) * 4.0));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(std::string::const_iterator iter = string_.cbegin(); iter != string_.cend(); ++iter) {
                std::bitset<charSize * 3> bitset;
                unsigned int counter(0U);

                for(std::string::const_iterator jter(iter); jter != string_.cend(); ++jter) {
                    counter += 1U;
                    bitset <<= charSize;
                    bitset |= std::bitset<charSize * 3>(std::bitset<charSize>(*jter).to_ullong());

                    if(std::next(jter, 1) == string_.cend() or std::next(jter, 1) == std::next(iter, 3)) {
                        iter = jter;

                        break;
                    }
                }

                switch(counter) {
                    case 3U: counter = 4U; break;
                    case 2U: {
                        bitset <<= charSize;
                        counter = 3U;

                        break;
                    }
                    case 1U: {
                        bitset <<= charSize * 2;
                        counter = 2U;

                        break;
                    }
                    default: UnreachableTerminate();
                }

                for(unsigned int i(1U); i < 5U; ++i) {
                    if(i > counter) {
                        if(withPadding_) {
                            encodedString.append(1, '=');
                        } else {
                            break;
                        }
                    } else {
                        std::bitset<(charSize * 3) / 4> partialBitset((bitset >> ((charSize * 3) - (6 * i))).to_ullong());

                        switch(partialBitset.to_ullong()) {
                            case 0B000000ULL: encodedString.append(1, 'A'); break;
                            case 0B000001ULL: encodedString.append(1, 'B'); break;
                            case 0B000010ULL: encodedString.append(1, 'C'); break;
                            case 0B000011ULL: encodedString.append(1, 'D'); break;
                            case 0B000100ULL: encodedString.append(1, 'E'); break;
                            case 0B000101ULL: encodedString.append(1, 'F'); break;
                            case 0B000110ULL: encodedString.append(1, 'G'); break;
                            case 0B000111ULL: encodedString.append(1, 'H'); break;
                            case 0B001000ULL: encodedString.append(1, 'I'); break;
                            case 0B001001ULL: encodedString.append(1, 'J'); break;
                            case 0B001010ULL: encodedString.append(1, 'K'); break;
                            case 0B001011ULL: encodedString.append(1, 'L'); break;
                            case 0B001100ULL: encodedString.append(1, 'M'); break;
                            case 0B001101ULL: encodedString.append(1, 'N'); break;
                            case 0B001110ULL: encodedString.append(1, 'O'); break;
                            case 0B001111ULL: encodedString.append(1, 'P'); break;
                            case 0B010000ULL: encodedString.append(1, 'Q'); break;
                            case 0B010001ULL: encodedString.append(1, 'R'); break;
                            case 0B010010ULL: encodedString.append(1, 'S'); break;
                            case 0B010011ULL: encodedString.append(1, 'T'); break;
                            case 0B010100ULL: encodedString.append(1, 'U'); break;
                            case 0B010101ULL: encodedString.append(1, 'V'); break;
                            case 0B010110ULL: encodedString.append(1, 'W'); break;
                            case 0B010111ULL: encodedString.append(1, 'X'); break;
                            case 0B011000ULL: encodedString.append(1, 'Y'); break;
                            case 0B011001ULL: encodedString.append(1, 'Z'); break;
                            case 0B011010ULL: encodedString.append(1, 'a'); break;
                            case 0B011011ULL: encodedString.append(1, 'b'); break;
                            case 0B011100ULL: encodedString.append(1, 'c'); break;
                            case 0B011101ULL: encodedString.append(1, 'd'); break;
                            case 0B011110ULL: encodedString.append(1, 'e'); break;
                            case 0B011111ULL: encodedString.append(1, 'f'); break;
                            case 0B100000ULL: encodedString.append(1, 'g'); break;
                            case 0B100001ULL: encodedString.append(1, 'h'); break;
                            case 0B100010ULL: encodedString.append(1, 'i'); break;
                            case 0B100011ULL: encodedString.append(1, 'j'); break;
                            case 0B100100ULL: encodedString.append(1, 'k'); break;
                            case 0B100101ULL: encodedString.append(1, 'l'); break;
                            case 0B100110ULL: encodedString.append(1, 'm'); break;
                            case 0B100111ULL: encodedString.append(1, 'n'); break;
                            case 0B101000ULL: encodedString.append(1, 'o'); break;
                            case 0B101001ULL: encodedString.append(1, 'p'); break;
                            case 0B101010ULL: encodedString.append(1, 'q'); break;
                            case 0B101011ULL: encodedString.append(1, 'r'); break;
                            case 0B101100ULL: encodedString.append(1, 's'); break;
                            case 0B101101ULL: encodedString.append(1, 't'); break;
                            case 0B101110ULL: encodedString.append(1, 'u'); break;
                            case 0B101111ULL: encodedString.append(1, 'v'); break;
                            case 0B110000ULL: encodedString.append(1, 'w'); break;
                            case 0B110001ULL: encodedString.append(1, 'x'); break;
                            case 0B110010ULL: encodedString.append(1, 'y'); break;
                            case 0B110011ULL: encodedString.append(1, 'z'); break;
                            case 0B110100ULL: encodedString.append(1, '0'); break;
                            case 0B110101ULL: encodedString.append(1, '1'); break;
                            case 0B110110ULL: encodedString.append(1, '2'); break;
                            case 0B110111ULL: encodedString.append(1, '3'); break;
                            case 0B111000ULL: encodedString.append(1, '4'); break;
                            case 0B111001ULL: encodedString.append(1, '5'); break;
                            case 0B111010ULL: encodedString.append(1, '6'); break;
                            case 0B111011ULL: encodedString.append(1, '7'); break;
                            case 0B111100ULL: encodedString.append(1, '8'); break;
                            case 0B111101ULL: encodedString.append(1, '9'); break;
                            case 0B111110ULL: encodedString.append(1, '+'); break;
                            case 0B111111ULL: encodedString.append(1, '/'); break;
                            default: UnreachableTerminate();
                        }
                    }
                }
            }

            return encodedString;
        }
        /**
         * @brief Encodes a ByteBuffer into a Base64 encoded string.
         * @tparam ByteType Type that satisfies the ByteBufferCompatible concept (char, signed char, unsigned char and std::byte).
         * @param[in] string_ String to be encoded.
         * @param[in] withPadding_ Whether or not padding (the '=' character) should be included.
         * @throws BinaryText::Base64::Error
         * @throws BinaryText::ByteBuffer::Error
        */
        template<ByteBufferCompatible ByteType>
        std::string EncodeByteBufferToString(const ByteBuffer<ByteType>& byteBuffer_, const bool withPadding_ = true)
        {
            std::string encodedString;

            try {
                if(byteBuffer_.GetSize() > 0) {
                    encodedString.reserve(static_cast<std::size_t>(std::ceil((static_cast<double>(byteBuffer_.GetSize())) / 3.0) * 4.0));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(typename ByteBuffer<ByteType>::ConstantIterator iter(byteBuffer_.ConstantBegin()); iter != byteBuffer_.ConstantEnd(); ++iter) {
                std::bitset<charSize * 3> bitset;
                unsigned int counter(0U);

                for(typename ByteBuffer<ByteType>::ConstantIterator jter(iter); jter != byteBuffer_.ConstantEnd(); ++jter) {
                    counter += 1U;
                    bitset <<= charSize;
                    bitset |= std::bitset<charSize * 3>(std::bitset<charSize>(static_cast<char>(*jter)).to_ullong());

                    if(std::next(jter, 1) == byteBuffer_.ConstantEnd() or std::next(jter, 1) == std::next(iter, 3)) {
                        iter = jter;

                        break;
                    }
                }

                switch(counter) {
                    case 3U: counter = 4U; break;
                    case 2U: {
                        bitset <<= charSize;
                        counter = 3U;

                        break;
                    }
                    case 1U: {
                        bitset <<= charSize * 2;
                        counter = 2U;

                        break;
                    }
                    default: UnreachableTerminate();
                }

                for(unsigned int i(1U); i < 5U; ++i) {
                    if(i > counter) {
                        if(withPadding_) {
                            encodedString.append(1, '=');
                        } else {
                            break;
                        }
                    } else {
                        std::bitset<(charSize * 3) / 4> partialBitset((bitset >> ((charSize * 3) - (6 * i))).to_ullong());

                        switch(partialBitset.to_ullong()) {
                            case 0B000000ULL: encodedString.append(1, 'A'); break;
                            case 0B000001ULL: encodedString.append(1, 'B'); break;
                            case 0B000010ULL: encodedString.append(1, 'C'); break;
                            case 0B000011ULL: encodedString.append(1, 'D'); break;
                            case 0B000100ULL: encodedString.append(1, 'E'); break;
                            case 0B000101ULL: encodedString.append(1, 'F'); break;
                            case 0B000110ULL: encodedString.append(1, 'G'); break;
                            case 0B000111ULL: encodedString.append(1, 'H'); break;
                            case 0B001000ULL: encodedString.append(1, 'I'); break;
                            case 0B001001ULL: encodedString.append(1, 'J'); break;
                            case 0B001010ULL: encodedString.append(1, 'K'); break;
                            case 0B001011ULL: encodedString.append(1, 'L'); break;
                            case 0B001100ULL: encodedString.append(1, 'M'); break;
                            case 0B001101ULL: encodedString.append(1, 'N'); break;
                            case 0B001110ULL: encodedString.append(1, 'O'); break;
                            case 0B001111ULL: encodedString.append(1, 'P'); break;
                            case 0B010000ULL: encodedString.append(1, 'Q'); break;
                            case 0B010001ULL: encodedString.append(1, 'R'); break;
                            case 0B010010ULL: encodedString.append(1, 'S'); break;
                            case 0B010011ULL: encodedString.append(1, 'T'); break;
                            case 0B010100ULL: encodedString.append(1, 'U'); break;
                            case 0B010101ULL: encodedString.append(1, 'V'); break;
                            case 0B010110ULL: encodedString.append(1, 'W'); break;
                            case 0B010111ULL: encodedString.append(1, 'X'); break;
                            case 0B011000ULL: encodedString.append(1, 'Y'); break;
                            case 0B011001ULL: encodedString.append(1, 'Z'); break;
                            case 0B011010ULL: encodedString.append(1, 'a'); break;
                            case 0B011011ULL: encodedString.append(1, 'b'); break;
                            case 0B011100ULL: encodedString.append(1, 'c'); break;
                            case 0B011101ULL: encodedString.append(1, 'd'); break;
                            case 0B011110ULL: encodedString.append(1, 'e'); break;
                            case 0B011111ULL: encodedString.append(1, 'f'); break;
                            case 0B100000ULL: encodedString.append(1, 'g'); break;
                            case 0B100001ULL: encodedString.append(1, 'h'); break;
                            case 0B100010ULL: encodedString.append(1, 'i'); break;
                            case 0B100011ULL: encodedString.append(1, 'j'); break;
                            case 0B100100ULL: encodedString.append(1, 'k'); break;
                            case 0B100101ULL: encodedString.append(1, 'l'); break;
                            case 0B100110ULL: encodedString.append(1, 'm'); break;
                            case 0B100111ULL: encodedString.append(1, 'n'); break;
                            case 0B101000ULL: encodedString.append(1, 'o'); break;
                            case 0B101001ULL: encodedString.append(1, 'p'); break;
                            case 0B101010ULL: encodedString.append(1, 'q'); break;
                            case 0B101011ULL: encodedString.append(1, 'r'); break;
                            case 0B101100ULL: encodedString.append(1, 's'); break;
                            case 0B101101ULL: encodedString.append(1, 't'); break;
                            case 0B101110ULL: encodedString.append(1, 'u'); break;
                            case 0B101111ULL: encodedString.append(1, 'v'); break;
                            case 0B110000ULL: encodedString.append(1, 'w'); break;
                            case 0B110001ULL: encodedString.append(1, 'x'); break;
                            case 0B110010ULL: encodedString.append(1, 'y'); break;
                            case 0B110011ULL: encodedString.append(1, 'z'); break;
                            case 0B110100ULL: encodedString.append(1, '0'); break;
                            case 0B110101ULL: encodedString.append(1, '1'); break;
                            case 0B110110ULL: encodedString.append(1, '2'); break;
                            case 0B110111ULL: encodedString.append(1, '3'); break;
                            case 0B111000ULL: encodedString.append(1, '4'); break;
                            case 0B111001ULL: encodedString.append(1, '5'); break;
                            case 0B111010ULL: encodedString.append(1, '6'); break;
                            case 0B111011ULL: encodedString.append(1, '7'); break;
                            case 0B111100ULL: encodedString.append(1, '8'); break;
                            case 0B111101ULL: encodedString.append(1, '9'); break;
                            case 0B111110ULL: encodedString.append(1, '+'); break;
                            case 0B111111ULL: encodedString.append(1, '/'); break;
                            default: UnreachableTerminate();
                        }
                    }
                }
            }

            return encodedString;
        }
        /**
         * @brief Decodes a Base64 encoded string into a decoded string. Whitespace and newline characters are not ignored.
         * @param[in] encodedString_ String to be decoded.
         * @throws BinaryText::Base64::Error
        */
        std::string DecodeStringToString(const std::string& encodedString_)
        {
            std::string decodedString;

            try {
                if(encodedString_.size() > 0) {
                    decodedString.reserve(static_cast<std::size_t>(std::ceil((static_cast<double>(encodedString_.size())) / 4.0) * 3.0));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(std::string::const_iterator iter(encodedString_.cbegin()); iter != encodedString_.cend(); ++iter) {
                std::bitset<charSize * 3> bitset;
                unsigned int paddingCounter(0U);
                unsigned int loopCounter(0U);

                for(std::string::const_iterator jter(iter); jter != encodedString_.cend(); ++jter) {
                    loopCounter += 1U;
                    bitset <<= (charSize * 3) / 4;

                    if(paddingCounter > 0U and *jter != '=') {
                        throw Error(Error::Type::STRING_PARSE_ERROR);
                    } else {
                        switch(*jter) {
                            case 'A': bitset |= std::bitset<charSize * 3>(0B000000ULL); break;
                            case 'B': bitset |= std::bitset<charSize * 3>(0B000001ULL); break;
                            case 'C': bitset |= std::bitset<charSize * 3>(0B000010ULL); break;
                            case 'D': bitset |= std::bitset<charSize * 3>(0B000011ULL); break;
                            case 'E': bitset |= std::bitset<charSize * 3>(0B000100ULL); break;
                            case 'F': bitset |= std::bitset<charSize * 3>(0B000101ULL); break;
                            case 'G': bitset |= std::bitset<charSize * 3>(0B000110ULL); break;
                            case 'H': bitset |= std::bitset<charSize * 3>(0B000111ULL); break;
                            case 'I': bitset |= std::bitset<charSize * 3>(0B001000ULL); break;
                            case 'J': bitset |= std::bitset<charSize * 3>(0B001001ULL); break;
                            case 'K': bitset |= std::bitset<charSize * 3>(0B001010ULL); break;
                            case 'L': bitset |= std::bitset<charSize * 3>(0B001011ULL); break;
                            case 'M': bitset |= std::bitset<charSize * 3>(0B001100ULL); break;
                            case 'N': bitset |= std::bitset<charSize * 3>(0B001101ULL); break;
                            case 'O': bitset |= std::bitset<charSize * 3>(0B001110ULL); break;
                            case 'P': bitset |= std::bitset<charSize * 3>(0B001111ULL); break;
                            case 'Q': bitset |= std::bitset<charSize * 3>(0B010000ULL); break;
                            case 'R': bitset |= std::bitset<charSize * 3>(0B010001ULL); break;
                            case 'S': bitset |= std::bitset<charSize * 3>(0B010010ULL); break;
                            case 'T': bitset |= std::bitset<charSize * 3>(0B010011ULL); break;
                            case 'U': bitset |= std::bitset<charSize * 3>(0B010100ULL); break;
                            case 'V': bitset |= std::bitset<charSize * 3>(0B010101ULL); break;
                            case 'W': bitset |= std::bitset<charSize * 3>(0B010110ULL); break;
                            case 'X': bitset |= std::bitset<charSize * 3>(0B010111ULL); break;
                            case 'Y': bitset |= std::bitset<charSize * 3>(0B011000ULL); break;
                            case 'Z': bitset |= std::bitset<charSize * 3>(0B011001ULL); break;
                            case 'a': bitset |= std::bitset<charSize * 3>(0B011010ULL); break;
                            case 'b': bitset |= std::bitset<charSize * 3>(0B011011ULL); break;
                            case 'c': bitset |= std::bitset<charSize * 3>(0B011100ULL); break;
                            case 'd': bitset |= std::bitset<charSize * 3>(0B011101ULL); break;
                            case 'e': bitset |= std::bitset<charSize * 3>(0B011110ULL); break;
                            case 'f': bitset |= std::bitset<charSize * 3>(0B011111ULL); break;
                            case 'g': bitset |= std::bitset<charSize * 3>(0B100000ULL); break;
                            case 'h': bitset |= std::bitset<charSize * 3>(0B100001ULL); break;
                            case 'i': bitset |= std::bitset<charSize * 3>(0B100010ULL); break;
                            case 'j': bitset |= std::bitset<charSize * 3>(0B100011ULL); break;
                            case 'k': bitset |= std::bitset<charSize * 3>(0B100100ULL); break;
                            case 'l': bitset |= std::bitset<charSize * 3>(0B100101ULL); break;
                            case 'm': bitset |= std::bitset<charSize * 3>(0B100110ULL); break;
                            case 'n': bitset |= std::bitset<charSize * 3>(0B100111ULL); break;
                            case 'o': bitset |= std::bitset<charSize * 3>(0B101000ULL); break;
                            case 'p': bitset |= std::bitset<charSize * 3>(0B101001ULL); break;
                            case 'q': bitset |= std::bitset<charSize * 3>(0B101010ULL); break;
                            case 'r': bitset |= std::bitset<charSize * 3>(0B101011ULL); break;
                            case 's': bitset |= std::bitset<charSize * 3>(0B101100ULL); break;
                            case 't': bitset |= std::bitset<charSize * 3>(0B101101ULL); break;
                            case 'u': bitset |= std::bitset<charSize * 3>(0B101110ULL); break;
                            case 'v': bitset |= std::bitset<charSize * 3>(0B101111ULL); break;
                            case 'w': bitset |= std::bitset<charSize * 3>(0B110000ULL); break;
                            case 'x': bitset |= std::bitset<charSize * 3>(0B110001ULL); break;
                            case 'y': bitset |= std::bitset<charSize * 3>(0B110010ULL); break;
                            case 'z': bitset |= std::bitset<charSize * 3>(0B110011ULL); break;
                            case '0': bitset |= std::bitset<charSize * 3>(0B110100ULL); break;
                            case '1': bitset |= std::bitset<charSize * 3>(0B110101ULL); break;
                            case '2': bitset |= std::bitset<charSize * 3>(0B110110ULL); break;
                            case '3': bitset |= std::bitset<charSize * 3>(0B110111ULL); break;
                            case '4': bitset |= std::bitset<charSize * 3>(0B111000ULL); break;
                            case '5': bitset |= std::bitset<charSize * 3>(0B111001ULL); break;
                            case '6': bitset |= std::bitset<charSize * 3>(0B111010ULL); break;
                            case '7': bitset |= std::bitset<charSize * 3>(0B111011ULL); break;
                            case '8': bitset |= std::bitset<charSize * 3>(0B111100ULL); break;
                            case '9': bitset |= std::bitset<charSize * 3>(0B111101ULL); break;
                            case '+': bitset |= std::bitset<charSize * 3>(0B111110ULL); break;
                            case '/': bitset |= std::bitset<charSize * 3>(0B111111ULL); break;
                            case '=': paddingCounter += 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }
                    }

                    if(std::next(jter, 1) == encodedString_.cend() or std::next(jter, 1) == std::next(iter, 4)) {
                        iter = jter;

                        break;
                    }
                }

                switch(loopCounter) {
                    case 4U: break;
                    case 3U: {
                        bitset <<= (charSize * 3) / 4;

                        switch(paddingCounter) {
                            case 1U: paddingCounter = 2U; break;
                            case 0U: paddingCounter = 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 2U: {
                        bitset <<= ((charSize * 3) / 4) * 2;

                        switch(paddingCounter) {
                            case 0U: paddingCounter = 2U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 1U: throw Error(Error::Type::STRING_PARSE_ERROR);
                    default: UnreachableTerminate();
                }

                switch(paddingCounter) {
                    case 0U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> charSize).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>(bitset.to_ullong())));

                        break;
                    }
                    case 1U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> charSize).to_ullong())));

                        break;
                    }
                    case 2U: decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()))); break;
                    default: throw Error(Error::Type::STRING_PARSE_ERROR);
                }

                if(paddingCounter > 0U) {
                    break;
                }
            }

            return decodedString;
        }
        /**
         * @brief Decodes a Base64 encoded string into a decoded ByteBuffer. Whitespace and newline characters are not ignored.
         * @tparam ByteType Type that satisfies the ByteBufferCompatible concept (char, signed char, unsigned char and std::byte).
         * @param[in] encodedString_ String to be decoded.
         * @throws BinaryText::Base64::Error
         * @throws BinaryText::ByteBuffer::Error
        */
        template<ByteBufferCompatible ByteType>
        ByteBuffer<ByteType> DecodeStringToByteBuffer(const std::string& encodedString_)
        {
            ByteBuffer<ByteType> decodedByteBuffer;
            ByteBuffer<ByteType> byteBuffer(8192);
            typename ByteBuffer<ByteType>::SizeType bufferIterator(0);
            auto addToByteBuffer = [&decodedByteBuffer, &byteBuffer, &bufferIterator](const unsigned char byte_) -> void {
                if(bufferIterator < 8192) {
                    if constexpr(std::is_same_v<ByteType, unsigned char>) {
                        byteBuffer.At(bufferIterator) = byte_;
                    } else {
                        byteBuffer.At(bufferIterator) = static_cast<ByteType>(byte_);
                    }

                    bufferIterator += 1;
                } else {
                    decodedByteBuffer += byteBuffer;
                    bufferIterator = 0;

                    if constexpr(std::is_same_v<ByteType, unsigned char>) {
                        byteBuffer.At(bufferIterator) = byte_;
                    } else {
                        byteBuffer.At(bufferIterator) = static_cast<ByteType>(byte_);
                    }

                    bufferIterator += 1;
                }
            };

            for(std::string::const_iterator iter(encodedString_.cbegin()); iter != encodedString_.cend(); ++iter) {
                std::bitset<charSize * 3> bitset;
                unsigned int paddingCounter(0U);
                unsigned int loopCounter(0U);

                for(std::string::const_iterator jter(iter); jter != encodedString_.cend(); ++jter) {
                    loopCounter += 1U;
                    bitset <<= (charSize * 3) / 4;

                    if(paddingCounter > 0U and *jter != '=') {
                        throw Error(Error::Type::STRING_PARSE_ERROR);
                    } else {
                        switch(*jter) {
                            case 'A': bitset |= std::bitset<charSize * 3>(0B000000ULL); break;
                            case 'B': bitset |= std::bitset<charSize * 3>(0B000001ULL); break;
                            case 'C': bitset |= std::bitset<charSize * 3>(0B000010ULL); break;
                            case 'D': bitset |= std::bitset<charSize * 3>(0B000011ULL); break;
                            case 'E': bitset |= std::bitset<charSize * 3>(0B000100ULL); break;
                            case 'F': bitset |= std::bitset<charSize * 3>(0B000101ULL); break;
                            case 'G': bitset |= std::bitset<charSize * 3>(0B000110ULL); break;
                            case 'H': bitset |= std::bitset<charSize * 3>(0B000111ULL); break;
                            case 'I': bitset |= std::bitset<charSize * 3>(0B001000ULL); break;
                            case 'J': bitset |= std::bitset<charSize * 3>(0B001001ULL); break;
                            case 'K': bitset |= std::bitset<charSize * 3>(0B001010ULL); break;
                            case 'L': bitset |= std::bitset<charSize * 3>(0B001011ULL); break;
                            case 'M': bitset |= std::bitset<charSize * 3>(0B001100ULL); break;
                            case 'N': bitset |= std::bitset<charSize * 3>(0B001101ULL); break;
                            case 'O': bitset |= std::bitset<charSize * 3>(0B001110ULL); break;
                            case 'P': bitset |= std::bitset<charSize * 3>(0B001111ULL); break;
                            case 'Q': bitset |= std::bitset<charSize * 3>(0B010000ULL); break;
                            case 'R': bitset |= std::bitset<charSize * 3>(0B010001ULL); break;
                            case 'S': bitset |= std::bitset<charSize * 3>(0B010010ULL); break;
                            case 'T': bitset |= std::bitset<charSize * 3>(0B010011ULL); break;
                            case 'U': bitset |= std::bitset<charSize * 3>(0B010100ULL); break;
                            case 'V': bitset |= std::bitset<charSize * 3>(0B010101ULL); break;
                            case 'W': bitset |= std::bitset<charSize * 3>(0B010110ULL); break;
                            case 'X': bitset |= std::bitset<charSize * 3>(0B010111ULL); break;
                            case 'Y': bitset |= std::bitset<charSize * 3>(0B011000ULL); break;
                            case 'Z': bitset |= std::bitset<charSize * 3>(0B011001ULL); break;
                            case 'a': bitset |= std::bitset<charSize * 3>(0B011010ULL); break;
                            case 'b': bitset |= std::bitset<charSize * 3>(0B011011ULL); break;
                            case 'c': bitset |= std::bitset<charSize * 3>(0B011100ULL); break;
                            case 'd': bitset |= std::bitset<charSize * 3>(0B011101ULL); break;
                            case 'e': bitset |= std::bitset<charSize * 3>(0B011110ULL); break;
                            case 'f': bitset |= std::bitset<charSize * 3>(0B011111ULL); break;
                            case 'g': bitset |= std::bitset<charSize * 3>(0B100000ULL); break;
                            case 'h': bitset |= std::bitset<charSize * 3>(0B100001ULL); break;
                            case 'i': bitset |= std::bitset<charSize * 3>(0B100010ULL); break;
                            case 'j': bitset |= std::bitset<charSize * 3>(0B100011ULL); break;
                            case 'k': bitset |= std::bitset<charSize * 3>(0B100100ULL); break;
                            case 'l': bitset |= std::bitset<charSize * 3>(0B100101ULL); break;
                            case 'm': bitset |= std::bitset<charSize * 3>(0B100110ULL); break;
                            case 'n': bitset |= std::bitset<charSize * 3>(0B100111ULL); break;
                            case 'o': bitset |= std::bitset<charSize * 3>(0B101000ULL); break;
                            case 'p': bitset |= std::bitset<charSize * 3>(0B101001ULL); break;
                            case 'q': bitset |= std::bitset<charSize * 3>(0B101010ULL); break;
                            case 'r': bitset |= std::bitset<charSize * 3>(0B101011ULL); break;
                            case 's': bitset |= std::bitset<charSize * 3>(0B101100ULL); break;
                            case 't': bitset |= std::bitset<charSize * 3>(0B101101ULL); break;
                            case 'u': bitset |= std::bitset<charSize * 3>(0B101110ULL); break;
                            case 'v': bitset |= std::bitset<charSize * 3>(0B101111ULL); break;
                            case 'w': bitset |= std::bitset<charSize * 3>(0B110000ULL); break;
                            case 'x': bitset |= std::bitset<charSize * 3>(0B110001ULL); break;
                            case 'y': bitset |= std::bitset<charSize * 3>(0B110010ULL); break;
                            case 'z': bitset |= std::bitset<charSize * 3>(0B110011ULL); break;
                            case '0': bitset |= std::bitset<charSize * 3>(0B110100ULL); break;
                            case '1': bitset |= std::bitset<charSize * 3>(0B110101ULL); break;
                            case '2': bitset |= std::bitset<charSize * 3>(0B110110ULL); break;
                            case '3': bitset |= std::bitset<charSize * 3>(0B110111ULL); break;
                            case '4': bitset |= std::bitset<charSize * 3>(0B111000ULL); break;
                            case '5': bitset |= std::bitset<charSize * 3>(0B111001ULL); break;
                            case '6': bitset |= std::bitset<charSize * 3>(0B111010ULL); break;
                            case '7': bitset |= std::bitset<charSize * 3>(0B111011ULL); break;
                            case '8': bitset |= std::bitset<charSize * 3>(0B111100ULL); break;
                            case '9': bitset |= std::bitset<charSize * 3>(0B111101ULL); break;
                            case '+': bitset |= std::bitset<charSize * 3>(0B111110ULL); break;
                            case '/': bitset |= std::bitset<charSize * 3>(0B111111ULL); break;
                            case '=': paddingCounter += 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }
                    }

                    if(std::next(jter, 1) == encodedString_.cend() or std::next(jter, 1) == std::next(iter, 4)) {
                        iter = jter;

                        break;
                    }
                }

                switch(loopCounter) {
                    case 4U: break;
                    case 3U: {
                        bitset <<= (charSize * 3) / 4;

                        switch(paddingCounter) {
                            case 1U: paddingCounter = 2U; break;
                            case 0U: paddingCounter = 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 2U: {
                        bitset <<= ((charSize * 3) / 4) * 2;

                        switch(paddingCounter) {
                            case 0U: paddingCounter = 2U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 1U: throw Error(Error::Type::STRING_PARSE_ERROR);
                    default: UnreachableTerminate();
                }

                switch(paddingCounter) {
                    case 0U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> charSize).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>(bitset.to_ullong()));

                        break;
                    }
                    case 1U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> charSize).to_ullong()));

                        break;
                    }
                    case 2U: addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())); break;
                    default: throw Error(Error::Type::STRING_PARSE_ERROR);
                }

                if(paddingCounter > 0U) {
                    break;
                }
            }

            if(bufferIterator > 0) {
                byteBuffer.Resize(bufferIterator);

                decodedByteBuffer += byteBuffer;
            }

            return decodedByteBuffer;
        }
    };

    /// @brief A namespace that has functions that implement Base64Url encoding and decoding in accordance to RFC 4648 §5.
    namespace Base64Url
    {
        /// @brief A simple error class for the Base64Url namespace.
        class Error : public std::exception
        {
        public:
            /// @brief The type of Error.
            enum class Type
            {
                INTERNAL_STRING_RESERVE_ERROR, ///< Failed to reserve size to internal string.
                STRING_PARSE_ERROR             ///< Failed to parse string.
            };

            /**
             * @brief Creates an Error of given Type.
             * @param[in] type_ Type of Error.
             * @param[in] sourceLocation_ Source location of Error.
            */
            explicit Error(const Type type_, const std::source_location sourceLocation_ = std::source_location::current()) :
                _type(type_),
                _sourceLocation(sourceLocation_)
            {
                switch(_type) {
                    case Type::INTERNAL_STRING_RESERVE_ERROR: _what = "Faild to reserve size to internal string"; break;
                    case Type::STRING_PARSE_ERROR: _what = "Failed to parse string"; break;
                    default: _what = "Invalid error type"; break;
                }
            }

            /**
             * @brief Gets the Type of the Error.
             * @returns Type of Error.
            */
            Type GetType() const noexcept { return _type; }
            /**
             * @brief Gets the location at which the Error was thrown.
             * @returns Source location of Error.
            */
            std::source_location GetSourceLocation() const { return _sourceLocation; }
            /**
             * @brief Gets reason for the Error.
             * @returns Reason for the Error.
            */
            std::string What() const { return _what; }

            // For C++ compatibility purposes

            const char* what() const noexcept override { return _what.c_str(); }

        private:
            Type _type;
            std::source_location _sourceLocation;
            std::string _what;
        };

        static_assert(std::numeric_limits<unsigned char>::digits == 8, "These Base64Url functions only works if a char is 8 bits big");

        /**
         * @brief Encodes a not-encoded string into a Base64Url encoded string.
         * @param[in] string_ String to be encoded.
         * @param[in] withPadding_ Whether or not padding (the '=' character) should be included.
         * @throws BinaryText::Base64Url::Error
        */
        std::string EncodeStringToString(const std::string& string_, const bool withPadding_ = true)
        {
            std::string encodedString;

            try {
                if(string_.size() > 0) {
                    encodedString.reserve(static_cast<std::size_t>(std::ceil((static_cast<double>(string_.size())) / 3.0) * 4.0));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(std::string::const_iterator iter = string_.cbegin(); iter != string_.cend(); ++iter) {
                std::bitset<charSize * 3> bitset;
                unsigned int counter(0U);

                for(std::string::const_iterator jter(iter); jter != string_.cend(); ++jter) {
                    counter += 1U;
                    bitset <<= charSize;
                    bitset |= std::bitset<charSize * 3>(std::bitset<charSize>(*jter).to_ullong());

                    if((std::next(jter, 1) == string_.cend()) or (std::next(jter, 1) == std::next(iter, 3))) {
                        iter = jter;

                        break;
                    }
                }

                switch(counter) {
                    case 3U: counter = 4U; break;
                    case 2U: {
                        bitset <<= charSize;
                        counter = 3U;

                        break;
                    }
                    case 1U: {
                        bitset <<= (charSize * 2);
                        counter = 2U;

                        break;
                    }
                    default: UnreachableTerminate();
                }

                for(unsigned int i(1U); i < 5U; ++i) {
                    if(i > counter) {
                        if(withPadding_) {
                            encodedString.append(1, '=');
                        } else {
                            break;
                        }
                    } else {
                        std::bitset<(charSize * 3) / 4> partialBitset((bitset >> ((charSize * 3) - (6 * i))).to_ullong());

                        switch(partialBitset.to_ullong()) {
                            case 0B000000ULL: encodedString.append(1, 'A'); break;
                            case 0B000001ULL: encodedString.append(1, 'B'); break;
                            case 0B000010ULL: encodedString.append(1, 'C'); break;
                            case 0B000011ULL: encodedString.append(1, 'D'); break;
                            case 0B000100ULL: encodedString.append(1, 'E'); break;
                            case 0B000101ULL: encodedString.append(1, 'F'); break;
                            case 0B000110ULL: encodedString.append(1, 'G'); break;
                            case 0B000111ULL: encodedString.append(1, 'H'); break;
                            case 0B001000ULL: encodedString.append(1, 'I'); break;
                            case 0B001001ULL: encodedString.append(1, 'J'); break;
                            case 0B001010ULL: encodedString.append(1, 'K'); break;
                            case 0B001011ULL: encodedString.append(1, 'L'); break;
                            case 0B001100ULL: encodedString.append(1, 'M'); break;
                            case 0B001101ULL: encodedString.append(1, 'N'); break;
                            case 0B001110ULL: encodedString.append(1, 'O'); break;
                            case 0B001111ULL: encodedString.append(1, 'P'); break;
                            case 0B010000ULL: encodedString.append(1, 'Q'); break;
                            case 0B010001ULL: encodedString.append(1, 'R'); break;
                            case 0B010010ULL: encodedString.append(1, 'S'); break;
                            case 0B010011ULL: encodedString.append(1, 'T'); break;
                            case 0B010100ULL: encodedString.append(1, 'U'); break;
                            case 0B010101ULL: encodedString.append(1, 'V'); break;
                            case 0B010110ULL: encodedString.append(1, 'W'); break;
                            case 0B010111ULL: encodedString.append(1, 'X'); break;
                            case 0B011000ULL: encodedString.append(1, 'Y'); break;
                            case 0B011001ULL: encodedString.append(1, 'Z'); break;
                            case 0B011010ULL: encodedString.append(1, 'a'); break;
                            case 0B011011ULL: encodedString.append(1, 'b'); break;
                            case 0B011100ULL: encodedString.append(1, 'c'); break;
                            case 0B011101ULL: encodedString.append(1, 'd'); break;
                            case 0B011110ULL: encodedString.append(1, 'e'); break;
                            case 0B011111ULL: encodedString.append(1, 'f'); break;
                            case 0B100000ULL: encodedString.append(1, 'g'); break;
                            case 0B100001ULL: encodedString.append(1, 'h'); break;
                            case 0B100010ULL: encodedString.append(1, 'i'); break;
                            case 0B100011ULL: encodedString.append(1, 'j'); break;
                            case 0B100100ULL: encodedString.append(1, 'k'); break;
                            case 0B100101ULL: encodedString.append(1, 'l'); break;
                            case 0B100110ULL: encodedString.append(1, 'm'); break;
                            case 0B100111ULL: encodedString.append(1, 'n'); break;
                            case 0B101000ULL: encodedString.append(1, 'o'); break;
                            case 0B101001ULL: encodedString.append(1, 'p'); break;
                            case 0B101010ULL: encodedString.append(1, 'q'); break;
                            case 0B101011ULL: encodedString.append(1, 'r'); break;
                            case 0B101100ULL: encodedString.append(1, 's'); break;
                            case 0B101101ULL: encodedString.append(1, 't'); break;
                            case 0B101110ULL: encodedString.append(1, 'u'); break;
                            case 0B101111ULL: encodedString.append(1, 'v'); break;
                            case 0B110000ULL: encodedString.append(1, 'w'); break;
                            case 0B110001ULL: encodedString.append(1, 'x'); break;
                            case 0B110010ULL: encodedString.append(1, 'y'); break;
                            case 0B110011ULL: encodedString.append(1, 'z'); break;
                            case 0B110100ULL: encodedString.append(1, '0'); break;
                            case 0B110101ULL: encodedString.append(1, '1'); break;
                            case 0B110110ULL: encodedString.append(1, '2'); break;
                            case 0B110111ULL: encodedString.append(1, '3'); break;
                            case 0B111000ULL: encodedString.append(1, '4'); break;
                            case 0B111001ULL: encodedString.append(1, '5'); break;
                            case 0B111010ULL: encodedString.append(1, '6'); break;
                            case 0B111011ULL: encodedString.append(1, '7'); break;
                            case 0B111100ULL: encodedString.append(1, '8'); break;
                            case 0B111101ULL: encodedString.append(1, '9'); break;
                            case 0B111110ULL: encodedString.append(1, '-'); break;
                            case 0B111111ULL: encodedString.append(1, '_'); break;
                            default: UnreachableTerminate();
                        }
                    }
                }
            }

            return encodedString;
        }
        /**
         * @brief Encodes a ByteBuffer into a Base64Url encoded string.
         * @tparam ByteType Type that satisfies the ByteBufferCompatible concept (char, signed char, unsigned char and std::byte).
         * @param[in] string_ String to be encoded.
         * @param[in] withPadding_ Whether or not padding (the '=' character) should be included.
         * @throws BinaryText::Base64Url::Error
         * @throws BinaryText::ByteBuffer::Error
        */
        template<ByteBufferCompatible ByteType>
        std::string EncodeByteBufferToString(const ByteBuffer<ByteType>& byteBuffer_, const bool withPadding_ = true)
        {
            std::string encodedString;

            try {
                if(byteBuffer_.GetSize() > 0) {
                    encodedString.reserve(static_cast<std::size_t>(std::ceil((static_cast<double>(byteBuffer_.GetSize())) / 3.0) * 4.0));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(typename ByteBuffer<ByteType>::ConstantIterator iter(byteBuffer_.ConstantBegin()); iter != byteBuffer_.ConstantEnd(); ++iter) {
                std::bitset<charSize * 3> bitset;
                unsigned int counter(0U);

                for(typename ByteBuffer<ByteType>::ConstantIterator jter(iter); jter != byteBuffer_.ConstantEnd(); ++jter) {
                    counter += 1U;
                    bitset <<= charSize;
                    bitset |= std::bitset<charSize * 3>(std::bitset<charSize>(static_cast<char>(*jter)).to_ullong());

                    if(std::next(jter, 1) == byteBuffer_.ConstantEnd() or std::next(jter, 1) == std::next(iter, 3)) {
                        iter = jter;

                        break;
                    }
                }

                switch(counter) {
                    case 3U: counter = 4U; break;
                    case 2U: {
                        bitset <<= charSize;
                        counter = 3U;

                        break;
                    }
                    case 1U: {
                        bitset <<= (charSize * 2);
                        counter = 2U;

                        break;
                    }
                    default: UnreachableTerminate();
                }

                for(unsigned int i(1U); i < 5U; ++i) {
                    if(i > counter) {
                        if(withPadding_) {
                            encodedString.append(1, '=');
                        } else {
                            break;
                        }
                    } else {
                        std::bitset<(charSize * 3) / 4> partialBitset((bitset >> ((charSize * 3) - (6 * i))).to_ullong());

                        switch(partialBitset.to_ullong()) {
                            case 0B000000ULL: encodedString.append(1, 'A'); break;
                            case 0B000001ULL: encodedString.append(1, 'B'); break;
                            case 0B000010ULL: encodedString.append(1, 'C'); break;
                            case 0B000011ULL: encodedString.append(1, 'D'); break;
                            case 0B000100ULL: encodedString.append(1, 'E'); break;
                            case 0B000101ULL: encodedString.append(1, 'F'); break;
                            case 0B000110ULL: encodedString.append(1, 'G'); break;
                            case 0B000111ULL: encodedString.append(1, 'H'); break;
                            case 0B001000ULL: encodedString.append(1, 'I'); break;
                            case 0B001001ULL: encodedString.append(1, 'J'); break;
                            case 0B001010ULL: encodedString.append(1, 'K'); break;
                            case 0B001011ULL: encodedString.append(1, 'L'); break;
                            case 0B001100ULL: encodedString.append(1, 'M'); break;
                            case 0B001101ULL: encodedString.append(1, 'N'); break;
                            case 0B001110ULL: encodedString.append(1, 'O'); break;
                            case 0B001111ULL: encodedString.append(1, 'P'); break;
                            case 0B010000ULL: encodedString.append(1, 'Q'); break;
                            case 0B010001ULL: encodedString.append(1, 'R'); break;
                            case 0B010010ULL: encodedString.append(1, 'S'); break;
                            case 0B010011ULL: encodedString.append(1, 'T'); break;
                            case 0B010100ULL: encodedString.append(1, 'U'); break;
                            case 0B010101ULL: encodedString.append(1, 'V'); break;
                            case 0B010110ULL: encodedString.append(1, 'W'); break;
                            case 0B010111ULL: encodedString.append(1, 'X'); break;
                            case 0B011000ULL: encodedString.append(1, 'Y'); break;
                            case 0B011001ULL: encodedString.append(1, 'Z'); break;
                            case 0B011010ULL: encodedString.append(1, 'a'); break;
                            case 0B011011ULL: encodedString.append(1, 'b'); break;
                            case 0B011100ULL: encodedString.append(1, 'c'); break;
                            case 0B011101ULL: encodedString.append(1, 'd'); break;
                            case 0B011110ULL: encodedString.append(1, 'e'); break;
                            case 0B011111ULL: encodedString.append(1, 'f'); break;
                            case 0B100000ULL: encodedString.append(1, 'g'); break;
                            case 0B100001ULL: encodedString.append(1, 'h'); break;
                            case 0B100010ULL: encodedString.append(1, 'i'); break;
                            case 0B100011ULL: encodedString.append(1, 'j'); break;
                            case 0B100100ULL: encodedString.append(1, 'k'); break;
                            case 0B100101ULL: encodedString.append(1, 'l'); break;
                            case 0B100110ULL: encodedString.append(1, 'm'); break;
                            case 0B100111ULL: encodedString.append(1, 'n'); break;
                            case 0B101000ULL: encodedString.append(1, 'o'); break;
                            case 0B101001ULL: encodedString.append(1, 'p'); break;
                            case 0B101010ULL: encodedString.append(1, 'q'); break;
                            case 0B101011ULL: encodedString.append(1, 'r'); break;
                            case 0B101100ULL: encodedString.append(1, 's'); break;
                            case 0B101101ULL: encodedString.append(1, 't'); break;
                            case 0B101110ULL: encodedString.append(1, 'u'); break;
                            case 0B101111ULL: encodedString.append(1, 'v'); break;
                            case 0B110000ULL: encodedString.append(1, 'w'); break;
                            case 0B110001ULL: encodedString.append(1, 'x'); break;
                            case 0B110010ULL: encodedString.append(1, 'y'); break;
                            case 0B110011ULL: encodedString.append(1, 'z'); break;
                            case 0B110100ULL: encodedString.append(1, '0'); break;
                            case 0B110101ULL: encodedString.append(1, '1'); break;
                            case 0B110110ULL: encodedString.append(1, '2'); break;
                            case 0B110111ULL: encodedString.append(1, '3'); break;
                            case 0B111000ULL: encodedString.append(1, '4'); break;
                            case 0B111001ULL: encodedString.append(1, '5'); break;
                            case 0B111010ULL: encodedString.append(1, '6'); break;
                            case 0B111011ULL: encodedString.append(1, '7'); break;
                            case 0B111100ULL: encodedString.append(1, '8'); break;
                            case 0B111101ULL: encodedString.append(1, '9'); break;
                            case 0B111110ULL: encodedString.append(1, '-'); break;
                            case 0B111111ULL: encodedString.append(1, '_'); break;
                            default: UnreachableTerminate();
                        }
                    }
                }
            }

            return encodedString;
        }
        /**
         * @brief Decodes a Base64Url encoded string into a decoded string. Whitespace and newline characters are not ignored.
         * @param[in] encodedString_ String to be decoded.
         * @throws BinaryText::Base64Url::Error
        */
        std::string DecodeStringToString(const std::string& encodedString_)
        {
            std::string decodedString;

            try {
                if(encodedString_.size() > 0) {
                    decodedString.reserve(static_cast<std::size_t>(std::ceil((static_cast<double>(encodedString_.size())) / 4.0) * 3.0));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            for(std::string::const_iterator iter(encodedString_.cbegin()); iter != encodedString_.cend(); ++iter) {
                std::bitset<charSize * 3> bitset;
                unsigned int paddingCounter(0U);
                unsigned int loopCounter(0U);

                for(std::string::const_iterator jter(iter); jter != encodedString_.cend(); ++jter) {
                    loopCounter += 1U;
                    bitset <<= (charSize * 3) / 4;

                    if(paddingCounter > 0U and *jter != '=') {
                        throw Error(Error::Type::STRING_PARSE_ERROR);
                    } else {
                        switch(*jter) {
                            case 'A': bitset |= std::bitset<charSize * 3>(0B000000ULL); break;
                            case 'B': bitset |= std::bitset<charSize * 3>(0B000001ULL); break;
                            case 'C': bitset |= std::bitset<charSize * 3>(0B000010ULL); break;
                            case 'D': bitset |= std::bitset<charSize * 3>(0B000011ULL); break;
                            case 'E': bitset |= std::bitset<charSize * 3>(0B000100ULL); break;
                            case 'F': bitset |= std::bitset<charSize * 3>(0B000101ULL); break;
                            case 'G': bitset |= std::bitset<charSize * 3>(0B000110ULL); break;
                            case 'H': bitset |= std::bitset<charSize * 3>(0B000111ULL); break;
                            case 'I': bitset |= std::bitset<charSize * 3>(0B001000ULL); break;
                            case 'J': bitset |= std::bitset<charSize * 3>(0B001001ULL); break;
                            case 'K': bitset |= std::bitset<charSize * 3>(0B001010ULL); break;
                            case 'L': bitset |= std::bitset<charSize * 3>(0B001011ULL); break;
                            case 'M': bitset |= std::bitset<charSize * 3>(0B001100ULL); break;
                            case 'N': bitset |= std::bitset<charSize * 3>(0B001101ULL); break;
                            case 'O': bitset |= std::bitset<charSize * 3>(0B001110ULL); break;
                            case 'P': bitset |= std::bitset<charSize * 3>(0B001111ULL); break;
                            case 'Q': bitset |= std::bitset<charSize * 3>(0B010000ULL); break;
                            case 'R': bitset |= std::bitset<charSize * 3>(0B010001ULL); break;
                            case 'S': bitset |= std::bitset<charSize * 3>(0B010010ULL); break;
                            case 'T': bitset |= std::bitset<charSize * 3>(0B010011ULL); break;
                            case 'U': bitset |= std::bitset<charSize * 3>(0B010100ULL); break;
                            case 'V': bitset |= std::bitset<charSize * 3>(0B010101ULL); break;
                            case 'W': bitset |= std::bitset<charSize * 3>(0B010110ULL); break;
                            case 'X': bitset |= std::bitset<charSize * 3>(0B010111ULL); break;
                            case 'Y': bitset |= std::bitset<charSize * 3>(0B011000ULL); break;
                            case 'Z': bitset |= std::bitset<charSize * 3>(0B011001ULL); break;
                            case 'a': bitset |= std::bitset<charSize * 3>(0B011010ULL); break;
                            case 'b': bitset |= std::bitset<charSize * 3>(0B011011ULL); break;
                            case 'c': bitset |= std::bitset<charSize * 3>(0B011100ULL); break;
                            case 'd': bitset |= std::bitset<charSize * 3>(0B011101ULL); break;
                            case 'e': bitset |= std::bitset<charSize * 3>(0B011110ULL); break;
                            case 'f': bitset |= std::bitset<charSize * 3>(0B011111ULL); break;
                            case 'g': bitset |= std::bitset<charSize * 3>(0B100000ULL); break;
                            case 'h': bitset |= std::bitset<charSize * 3>(0B100001ULL); break;
                            case 'i': bitset |= std::bitset<charSize * 3>(0B100010ULL); break;
                            case 'j': bitset |= std::bitset<charSize * 3>(0B100011ULL); break;
                            case 'k': bitset |= std::bitset<charSize * 3>(0B100100ULL); break;
                            case 'l': bitset |= std::bitset<charSize * 3>(0B100101ULL); break;
                            case 'm': bitset |= std::bitset<charSize * 3>(0B100110ULL); break;
                            case 'n': bitset |= std::bitset<charSize * 3>(0B100111ULL); break;
                            case 'o': bitset |= std::bitset<charSize * 3>(0B101000ULL); break;
                            case 'p': bitset |= std::bitset<charSize * 3>(0B101001ULL); break;
                            case 'q': bitset |= std::bitset<charSize * 3>(0B101010ULL); break;
                            case 'r': bitset |= std::bitset<charSize * 3>(0B101011ULL); break;
                            case 's': bitset |= std::bitset<charSize * 3>(0B101100ULL); break;
                            case 't': bitset |= std::bitset<charSize * 3>(0B101101ULL); break;
                            case 'u': bitset |= std::bitset<charSize * 3>(0B101110ULL); break;
                            case 'v': bitset |= std::bitset<charSize * 3>(0B101111ULL); break;
                            case 'w': bitset |= std::bitset<charSize * 3>(0B110000ULL); break;
                            case 'x': bitset |= std::bitset<charSize * 3>(0B110001ULL); break;
                            case 'y': bitset |= std::bitset<charSize * 3>(0B110010ULL); break;
                            case 'z': bitset |= std::bitset<charSize * 3>(0B110011ULL); break;
                            case '0': bitset |= std::bitset<charSize * 3>(0B110100ULL); break;
                            case '1': bitset |= std::bitset<charSize * 3>(0B110101ULL); break;
                            case '2': bitset |= std::bitset<charSize * 3>(0B110110ULL); break;
                            case '3': bitset |= std::bitset<charSize * 3>(0B110111ULL); break;
                            case '4': bitset |= std::bitset<charSize * 3>(0B111000ULL); break;
                            case '5': bitset |= std::bitset<charSize * 3>(0B111001ULL); break;
                            case '6': bitset |= std::bitset<charSize * 3>(0B111010ULL); break;
                            case '7': bitset |= std::bitset<charSize * 3>(0B111011ULL); break;
                            case '8': bitset |= std::bitset<charSize * 3>(0B111100ULL); break;
                            case '9': bitset |= std::bitset<charSize * 3>(0B111101ULL); break;
                            case '-': bitset |= std::bitset<charSize * 3>(0B111110ULL); break;
                            case '_': bitset |= std::bitset<charSize * 3>(0B111111ULL); break;
                            case '=': paddingCounter += 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }
                    }

                    if(std::next(jter, 1) == encodedString_.cend() or std::next(jter, 1) == std::next(iter, 4)) {
                        iter = jter;

                        break;
                    }
                }

                switch(loopCounter) {
                    case 4U: break;
                    case 3U: {
                        bitset <<= ((charSize * 3) / 4);

                        switch(paddingCounter) {
                            case 1U: paddingCounter = 2U; break;
                            case 0U: paddingCounter = 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 2U: {
                        bitset <<= (((charSize * 3) / 4) * 2);

                        switch(paddingCounter) {
                            case 0U: paddingCounter = 2U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 1U: throw Error(Error::Type::STRING_PARSE_ERROR);
                    default: UnreachableTerminate();
                }

                switch(paddingCounter) {
                    case 0U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> charSize).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>(bitset.to_ullong())));

                        break;
                    }
                    case 1U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> charSize).to_ullong())));

                        break;
                    }
                    case 2U: decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()))); break;
                    default: throw Error(Error::Type::STRING_PARSE_ERROR);
                }

                if(paddingCounter > 0U) {
                    break;
                }
            }

            return decodedString;
        }
        /**
         * @brief Decodes a Base64Url encoded string into a decoded ByteBuffer. Whitespace and newline characters are not ignored.
         * @tparam ByteType Type that satisfies the ByteBufferCompatible concept (char, signed char, unsigned char and std::byte).
         * @param[in] encodedString_ String to be decoded.
         * @throws BinaryText::Base64Url::Error
         * @throws BinaryText::ByteBuffer::Error
        */
        template<ByteBufferCompatible ByteType>
        ByteBuffer<ByteType> DecodeStringToByteBuffer(const std::string& encodedString_)
        {
            ByteBuffer<ByteType> decodedByteBuffer;
            ByteBuffer<ByteType> byteBuffer(8192);
            typename ByteBuffer<ByteType>::SizeType bufferIterator(0);
            auto addToByteBuffer = [&decodedByteBuffer, &byteBuffer, &bufferIterator](const unsigned char byte_) -> void {
                if(bufferIterator < 8192) {
                    if constexpr(std::is_same_v<ByteType, unsigned char>) {
                        byteBuffer.At(bufferIterator) = byte_;
                    } else {
                        byteBuffer.At(bufferIterator) = static_cast<ByteType>(byte_);
                    }

                    bufferIterator += 1;
                } else {
                    decodedByteBuffer += byteBuffer;
                    bufferIterator = 0;

                    if constexpr(std::is_same_v<ByteType, unsigned char>) {
                        byteBuffer.At(bufferIterator) = byte_;
                    } else {
                        byteBuffer.At(bufferIterator) = static_cast<ByteType>(byte_);
                    }

                    bufferIterator += 1;
                }
            };

            for(std::string::const_iterator iter(encodedString_.cbegin()); iter != encodedString_.cend(); ++iter) {
                std::bitset<charSize * 3> bitset;
                unsigned int paddingCounter(0U);
                unsigned int loopCounter(0U);

                for(std::string::const_iterator jter(iter); jter != encodedString_.cend(); ++jter) {
                    loopCounter += 1U;
                    bitset <<= (charSize * 3) / 4;

                    if(paddingCounter > 0U and *jter != '=') {
                        throw Error(Error::Type::STRING_PARSE_ERROR);
                    } else {
                        switch(*jter) {
                            case 'A': bitset |= std::bitset<charSize * 3>(0B000000ULL); break;
                            case 'B': bitset |= std::bitset<charSize * 3>(0B000001ULL); break;
                            case 'C': bitset |= std::bitset<charSize * 3>(0B000010ULL); break;
                            case 'D': bitset |= std::bitset<charSize * 3>(0B000011ULL); break;
                            case 'E': bitset |= std::bitset<charSize * 3>(0B000100ULL); break;
                            case 'F': bitset |= std::bitset<charSize * 3>(0B000101ULL); break;
                            case 'G': bitset |= std::bitset<charSize * 3>(0B000110ULL); break;
                            case 'H': bitset |= std::bitset<charSize * 3>(0B000111ULL); break;
                            case 'I': bitset |= std::bitset<charSize * 3>(0B001000ULL); break;
                            case 'J': bitset |= std::bitset<charSize * 3>(0B001001ULL); break;
                            case 'K': bitset |= std::bitset<charSize * 3>(0B001010ULL); break;
                            case 'L': bitset |= std::bitset<charSize * 3>(0B001011ULL); break;
                            case 'M': bitset |= std::bitset<charSize * 3>(0B001100ULL); break;
                            case 'N': bitset |= std::bitset<charSize * 3>(0B001101ULL); break;
                            case 'O': bitset |= std::bitset<charSize * 3>(0B001110ULL); break;
                            case 'P': bitset |= std::bitset<charSize * 3>(0B001111ULL); break;
                            case 'Q': bitset |= std::bitset<charSize * 3>(0B010000ULL); break;
                            case 'R': bitset |= std::bitset<charSize * 3>(0B010001ULL); break;
                            case 'S': bitset |= std::bitset<charSize * 3>(0B010010ULL); break;
                            case 'T': bitset |= std::bitset<charSize * 3>(0B010011ULL); break;
                            case 'U': bitset |= std::bitset<charSize * 3>(0B010100ULL); break;
                            case 'V': bitset |= std::bitset<charSize * 3>(0B010101ULL); break;
                            case 'W': bitset |= std::bitset<charSize * 3>(0B010110ULL); break;
                            case 'X': bitset |= std::bitset<charSize * 3>(0B010111ULL); break;
                            case 'Y': bitset |= std::bitset<charSize * 3>(0B011000ULL); break;
                            case 'Z': bitset |= std::bitset<charSize * 3>(0B011001ULL); break;
                            case 'a': bitset |= std::bitset<charSize * 3>(0B011010ULL); break;
                            case 'b': bitset |= std::bitset<charSize * 3>(0B011011ULL); break;
                            case 'c': bitset |= std::bitset<charSize * 3>(0B011100ULL); break;
                            case 'd': bitset |= std::bitset<charSize * 3>(0B011101ULL); break;
                            case 'e': bitset |= std::bitset<charSize * 3>(0B011110ULL); break;
                            case 'f': bitset |= std::bitset<charSize * 3>(0B011111ULL); break;
                            case 'g': bitset |= std::bitset<charSize * 3>(0B100000ULL); break;
                            case 'h': bitset |= std::bitset<charSize * 3>(0B100001ULL); break;
                            case 'i': bitset |= std::bitset<charSize * 3>(0B100010ULL); break;
                            case 'j': bitset |= std::bitset<charSize * 3>(0B100011ULL); break;
                            case 'k': bitset |= std::bitset<charSize * 3>(0B100100ULL); break;
                            case 'l': bitset |= std::bitset<charSize * 3>(0B100101ULL); break;
                            case 'm': bitset |= std::bitset<charSize * 3>(0B100110ULL); break;
                            case 'n': bitset |= std::bitset<charSize * 3>(0B100111ULL); break;
                            case 'o': bitset |= std::bitset<charSize * 3>(0B101000ULL); break;
                            case 'p': bitset |= std::bitset<charSize * 3>(0B101001ULL); break;
                            case 'q': bitset |= std::bitset<charSize * 3>(0B101010ULL); break;
                            case 'r': bitset |= std::bitset<charSize * 3>(0B101011ULL); break;
                            case 's': bitset |= std::bitset<charSize * 3>(0B101100ULL); break;
                            case 't': bitset |= std::bitset<charSize * 3>(0B101101ULL); break;
                            case 'u': bitset |= std::bitset<charSize * 3>(0B101110ULL); break;
                            case 'v': bitset |= std::bitset<charSize * 3>(0B101111ULL); break;
                            case 'w': bitset |= std::bitset<charSize * 3>(0B110000ULL); break;
                            case 'x': bitset |= std::bitset<charSize * 3>(0B110001ULL); break;
                            case 'y': bitset |= std::bitset<charSize * 3>(0B110010ULL); break;
                            case 'z': bitset |= std::bitset<charSize * 3>(0B110011ULL); break;
                            case '0': bitset |= std::bitset<charSize * 3>(0B110100ULL); break;
                            case '1': bitset |= std::bitset<charSize * 3>(0B110101ULL); break;
                            case '2': bitset |= std::bitset<charSize * 3>(0B110110ULL); break;
                            case '3': bitset |= std::bitset<charSize * 3>(0B110111ULL); break;
                            case '4': bitset |= std::bitset<charSize * 3>(0B111000ULL); break;
                            case '5': bitset |= std::bitset<charSize * 3>(0B111001ULL); break;
                            case '6': bitset |= std::bitset<charSize * 3>(0B111010ULL); break;
                            case '7': bitset |= std::bitset<charSize * 3>(0B111011ULL); break;
                            case '8': bitset |= std::bitset<charSize * 3>(0B111100ULL); break;
                            case '9': bitset |= std::bitset<charSize * 3>(0B111101ULL); break;
                            case '-': bitset |= std::bitset<charSize * 3>(0B111110ULL); break;
                            case '_': bitset |= std::bitset<charSize * 3>(0B111111ULL); break;
                            case '=': paddingCounter += 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }
                    }

                    if((std::next(jter, 1) == encodedString_.cend()) or (std::next(jter, 1) == std::next(iter, 4))) {
                        iter = jter;

                        break;
                    }
                }

                switch(loopCounter) {
                    case 4U: break;
                    case 3U: {
                        bitset <<= ((charSize * 3) / 4);

                        switch(paddingCounter) {
                            case 1U: paddingCounter = 2U; break;
                            case 0U: paddingCounter = 1U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 2U: {
                        bitset <<= (((charSize * 3) / 4) * 2);

                        switch(paddingCounter) {
                            case 0U: paddingCounter = 2U; break;
                            default: throw Error(Error::Type::STRING_PARSE_ERROR);
                        }

                        break;
                    }
                    case 1U: throw Error(Error::Type::STRING_PARSE_ERROR);
                    default: UnreachableTerminate();
                }

                switch(paddingCounter) {
                    case 0U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> charSize).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>(bitset.to_ullong()));

                        break;
                    }
                    case 1U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> charSize).to_ullong()));

                        break;
                    }
                    case 2U: addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())); break;
                    default: throw Error(Error::Type::STRING_PARSE_ERROR);
                }

                if(paddingCounter > 0U) {
                    break;
                }
            }

            if(bufferIterator > 0) {
                byteBuffer.Resize(bufferIterator);

                decodedByteBuffer += byteBuffer;
            }

            return decodedByteBuffer;
        }
    };

    /// @brief A namespace that has functions that implement Ascii85 encoding and decoding.
    namespace Ascii85
    {
        /// @brief A simple error class for the Ascii85 namespace.
        class Error : public std::exception
        {
        public:
            /// @brief The type of Error.
            enum class Type
            {
                INTERNAL_STRING_RESERVE_ERROR, ///< Failed to reserve size to internal string.
                STRING_PARSE_ERROR             ///< Failed to parse string.
            };

            /**
             * @brief Creates an Error of given Type.
             * @param[in] type_ Type of Error.
             * @param[in] sourceLocation_ Source location of Error.
            */
            explicit Error(const Type type_, const std::source_location sourceLocation_ = std::source_location::current()) :
                _type(type_),
                _sourceLocation(sourceLocation_)
            {
                switch(_type) {
                    case Type::INTERNAL_STRING_RESERVE_ERROR: _what = "Faild to reserve size to internal string"; break;
                    case Type::STRING_PARSE_ERROR: _what = "Failed to parse string"; break;
                    default: _what = "Invalid error type"; break;
                }
            }

            /**
             * @brief Gets the Type of the Error.
             * @returns Type of Error.
            */
            Type GetType() const noexcept { return _type; }
            /**
             * @brief Gets the location at which the Error was thrown.
             * @returns Source location of Error.
            */
            std::source_location GetSourceLocation() const { return _sourceLocation; }
            /**
             * @brief Gets reason for the Error.
             * @returns Reason for the Error.
            */
            std::string What() const { return _what; }

            // For C++ compatibility purposes

            const char* what() const noexcept override { return _what.c_str(); }

        private:
            Type _type;
            std::source_location _sourceLocation;
            std::string _what;
        };

        static_assert(charSize == 8, "These Ascii85 functions only works if a char is 8 bits big");

        /**
         * @brief Encodes a not-encoded string into an Ascii85 encoded string.
         * @param[in] string_ String to be encoded.
         * @param[in] foldSpaces_ Whether or not to fold spaces. That is, to turn 4 spaces (00100000001000000010000000100000) into y.
         * @param[in] adobeMode_ Whether or not to surround the encoded string with <~ and ~> delimiters.
         * @throws BinaryText::Ascii85::Error
        */
        std::string EncodeStringToString(const std::string& string_, const bool foldSpaces_ = false, const bool adobeMode_ = false)
        {
            std::string encodedString;

            try {
                if(string_.size() > 0) {
                    encodedString.reserve(string_.size() + static_cast<std::size_t>(std::ceil(static_cast<double>(string_.size()) / 4.0)));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            if(adobeMode_) {
                encodedString.append("<~");
            }

            for(std::string::const_iterator iter = string_.cbegin(); iter != string_.cend(); ++iter) {
                std::bitset<charSize * 4> bitset;
                unsigned int counter(0U);

                for(std::string::const_iterator jter(iter); jter != string_.end(); ++jter) {
                    counter += 1U;
                    bitset <<= charSize;
                    bitset |= std::bitset<charSize * 4>(std::bitset<charSize>(*jter).to_ullong());

                    if(std::next(jter, 1) == string_.cend() or std::next(jter, 1) == std::next(iter, 4)) {
                        iter = jter;

                        break;
                    }
                }

                switch(counter) {
                    case 4U: counter = 0U; break;
                    case 3U: bitset <<= charSize; break;
                    case 2U: bitset <<= (charSize * 2); break;
                    case 1U: bitset <<= (charSize * 3); break;
                    default: UnreachableTerminate();
                }

                if(bitset == std::bitset<charSize * 4>(0B00000000000000000000000000000000ULL)) {
                    encodedString.append(1, 'z');
                } else if(bitset == std::bitset<charSize * 4>(0B00100000001000000010000000100000ULL) and foldSpaces_) {
                    encodedString.append(1, 'y');
                } else {
                    unsigned long long bitsetNumber(bitset.to_ullong());
                    std::array<unsigned long long, 5> codes;

                    for(std::array<unsigned long long, 5>::iterator jter(codes.begin()); jter != codes.end(); ++jter) {
                        *jter = bitsetNumber % 85ULL;
                        bitsetNumber = static_cast<unsigned long long>(std::floor(static_cast<double>(bitsetNumber) / 85.0));
                    }

                    for(std::array<unsigned long long, 5>::reverse_iterator jter(codes.rbegin()); jter != codes.rend(); ++jter) {
                        encodedString.append(1, static_cast<char>(static_cast<unsigned char>(*jter + 33ULL)));
                    }

                    if((4U - counter) != 4U) {
                        encodedString.erase(std::prev(encodedString.end(), 4U - counter), encodedString.end());
                    }
                }
            }

            if(adobeMode_) {
                encodedString.append("~>");
            }

            return encodedString;
        }
        /**
         * @brief Encodes a ByteBuffer into a Ascii85 encoded string.
         * @tparam ByteType Type that satisfies the ByteBufferCompatible concept (char, signed char, unsigned char and std::byte).
         * @param[in] string_ String to be encoded.
         * @param[in] foldSpaces_ Whether or not to fold spaces. That is, to turn 4 spaces (00100000001000000010000000100000) into y.
         * @param[in] adobeMode_ Whether or not to surround the encoded string with <~ and ~> delimiters.
         * @throws BinaryText::Ascii85::Error
         * @throws BinaryText::ByteBuffer::Error
        */
        template<ByteBufferCompatible ByteType>
        std::string EncodeByteBufferToString(const ByteBuffer<ByteType>& byteBuffer_, const bool foldSpaces_ = false, const bool adobeMode_ = false)
        {
            std::string encodedString;

            try {
                if(byteBuffer_.GetSize() > 0) {
                    encodedString.reserve(byteBuffer_.GetSize() + static_cast<std::size_t>(std::ceil(static_cast<double>(byteBuffer_.GetSize()) / 4.0)));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            if(adobeMode_) {
                encodedString.append("<~");
            }

            for(typename ByteBuffer<ByteType>::ConstantIterator iter(byteBuffer_.ConstantBegin()); iter != byteBuffer_.ConstantEnd(); ++iter) {
                std::bitset<charSize * 4> bitset;
                unsigned int counter(0U);

                for(typename ByteBuffer<ByteType>::ConstantIterator jter(iter); jter != byteBuffer_.ConstantEnd(); ++jter) {
                    counter += 1U;
                    bitset <<= charSize;
                    bitset |= std::bitset<charSize>(static_cast<char>(*jter)).to_ullong();

                    if(std::next(jter, 1) == byteBuffer_.ConstantEnd() or std::next(jter, 1) == std::next(iter, 4)) {
                        iter = jter;

                        break;
                    }
                }

                switch(counter) {
                    case 4U: counter = 0U; break;
                    case 3U: bitset <<= charSize; break;
                    case 2U: bitset <<= (charSize * 2); break;
                    case 1U: bitset <<= (charSize * 3); break;
                    default: UnreachableTerminate();
                }

                if(bitset == std::bitset<charSize * 4>(0B00000000000000000000000000000000ULL)) {
                    encodedString.append(1, 'z');
                } else if(bitset == std::bitset<charSize * 4>(0B00100000001000000010000000100000ULL) and foldSpaces_) {
                    encodedString.append(1, 'y');
                } else {
                    unsigned long long bitsetNumber(bitset.to_ullong());
                    std::array<unsigned long long, 5> codes;

                    for(std::array<unsigned long long, 5>::iterator jter(codes.begin()); jter != codes.end(); ++jter) {
                        *jter = bitsetNumber % 85ULL;
                        bitsetNumber = static_cast<unsigned long long>(std::floor(static_cast<double>(bitsetNumber) / 85.0));
                    }

                    for(std::array<unsigned long long, 5>::reverse_iterator jter(codes.rbegin()); jter != codes.rend(); ++jter) {
                        encodedString.append(1, static_cast<char>(static_cast<unsigned char>(*jter + 33ULL)));
                    }

                    if((4U - counter) != 4U) {
                        encodedString.erase(std::prev(encodedString.end(), 4U - counter), encodedString.end());
                    }
                }
            }

            if(adobeMode_) {
                encodedString.append("~>");
            }

            return encodedString;
        }
        /**
         * @brief Decodes a Ascii85 encoded string into a decoded string. Whitespace and newline characters are ignored.
         * @param[in] encodedString_ String to be decoded.
         * @param[in] foldSpaces_ Whether or not to fold spaces. That is, to turn 4 spaces (00100000001000000010000000100000) into y.
         * @param[in] adobeMode_ Whether or not to surround the encoded string with <~ and ~> delimiters.
         * @throws BinaryText::Ascii85::Error
        */
        std::string DecodeStringToString(const std::string& encodedString_, const bool foldSpaces_ = false, const bool adobeMode_ = false)
        {
            std::string decodedString;

            try {
                if(encodedString_.size() > 0) {
                    decodedString.reserve(encodedString_.size() - static_cast<std::size_t>(std::ceil(static_cast<double>(encodedString_.size()) / 5.0)));
                }
            } catch(const std::length_error& error) {
                throw Error(Error::Type::INTERNAL_STRING_RESERVE_ERROR);
            }

            std::string::const_iterator encodedStringBegin(encodedString_.cbegin());
            std::string::const_iterator encodedStringEnd(encodedString_.cend());

            if(adobeMode_) {
                for(std::string::const_iterator iter(encodedString_.cbegin()); iter != encodedString_.cend(); ++iter) {
                    if(*iter == '<' and *std::next(iter, 1) == '~' and std::next(iter, 2) != encodedString_.cend()) {
                        encodedStringBegin = std::next(iter, 2);

                        break;
                    } else if(*iter == ' ' or *iter == '\n') {
                        if(std::next(iter, 1) == encodedString_.cend()) {
                            throw Error(Error::Type::STRING_PARSE_ERROR);
                        } else {
                            continue;
                        }
                    } else {
                        throw Error(Error::Type::STRING_PARSE_ERROR);
                    }
                }

                for(std::string::const_reverse_iterator iter(encodedString_.crbegin()); iter != encodedString_.crend(); ++iter) {
                    if(*iter == '>' and *std::next(iter, 1) == '~' and std::next(iter, 2) != encodedString_.crend()) {
                        encodedStringEnd = std::next(iter, 2).base();

                        break;
                    } else if(*iter == ' ' or *iter == '\n') {
                        if(std::next(iter, 1) == encodedString_.crend()) {
                            throw Error(Error::Type::STRING_PARSE_ERROR);
                        } else {
                            continue;
                        }
                    } else {
                        throw Error(Error::Type::STRING_PARSE_ERROR);
                    }
                }
            }

            for(std::string::const_iterator iter(encodedStringBegin); iter != encodedStringEnd; ++iter) {
                unsigned long long bitsetNumber(0ULL);
                unsigned int counter(0U);

                switch(*iter) {
                    case ' ': continue;
                    case '\n': continue;
                    case 'y': {
                        if(foldSpaces_) {
                            decodedString.append("    ");

                            continue;
                        } else {
                            throw Error(Error::Type::STRING_PARSE_ERROR);
                        }
                    }
                    case 'z': continue; // decodedString.append("\0\0\0\0");
                    default: {
                        for(std::string::const_iterator jter(iter); iter != encodedStringEnd; ++jter) {
                            if(*jter == ' ' or *jter == '\n') {
                                if(std::next(jter, 1) == encodedStringEnd) {
                                    break;
                                } else {
                                    std::advance(iter, 1);

                                    continue;
                                }
                            } else {
                                const unsigned long long partialBitsetNumber(std::bitset<charSize>(*jter).to_ullong());
                                counter += 1U;

                                if(partialBitsetNumber < 33ULL or partialBitsetNumber > 117ULL) {
                                    throw Error(Error::Type::STRING_PARSE_ERROR);
                                } else {
                                    switch(counter) {
                                        case 1U: bitsetNumber += (partialBitsetNumber - 33ULL) * (85ULL * 85ULL * 85ULL * 85ULL); break;
                                        case 2U: bitsetNumber += (partialBitsetNumber - 33ULL) * (85ULL * 85ULL * 85ULL); break;
                                        case 3U: bitsetNumber += (partialBitsetNumber - 33ULL) * (85ULL * 85ULL); break;
                                        case 4U: bitsetNumber += (partialBitsetNumber - 33ULL) * 85ULL; break;
                                        case 5U: bitsetNumber += partialBitsetNumber - 33ULL; break;
                                        default: UnreachableTerminate();
                                    }

                                    if(std::next(jter, 1) == encodedStringEnd or std::next(jter, 1) == std::next(iter, 5)) {
                                        iter = jter;

                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                {
                    const unsigned int previousCounter(counter);

                    while(counter < 5U) {
                        counter += 1U;

                        switch(counter) {
                            case 1U: bitsetNumber += 84ULL * 85ULL * 85ULL * 85ULL * 85ULL; break;
                            case 2U: bitsetNumber += 84ULL * 85ULL * 85ULL * 85ULL; break;
                            case 3U: bitsetNumber += 84ULL * 85ULL * 85ULL; break;
                            case 4U: bitsetNumber += 84ULL * 85ULL; break;
                            case 5U: bitsetNumber += 84ULL; break;
                            default: UnreachableTerminate();
                        }
                    }

                    counter = 5U - previousCounter;
                }

                const std::bitset<charSize * 4> bitset(bitsetNumber);

                switch(counter) {
                    case 0U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> charSize).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>(bitset.to_ullong())));

                        break;
                    }
                    case 1U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> charSize).to_ullong())));

                        break;
                    }
                    case 2U: {
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong())));
                        decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong())));

                        break;
                    }
                    case 3U: decodedString.append(1, static_cast<char>(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong()))); break;
                    case 4U: break;
                    default: UnreachableTerminate();
                }
            }

            return decodedString;
        }
        /**
         * @brief Decodes a Ascii85 encoded string into a decoded ByteBuffer. Whitespace and newline characters are ignored.
         * @tparam ByteType Type that satisfies the ByteBufferCompatible concept (char, signed char, unsigned char and std::byte).
         * @param[in] encodedString_ String to be decoded.
         * @param[in] foldSpaces_ Whether or not to fold spaces. That is, to turn 4 spaces (00100000001000000010000000100000) into y.
         * @param[in] adobeMode_ Whether or not to surround the encoded string with <~ and ~> delimiters.
         * @throws BinaryText::Ascii85::Error
         * @throws BinaryText::ByteBuffer::Error
        */
        template<ByteBufferCompatible ByteType>
        ByteBuffer<ByteType> DecodeStringToByteBuffer(const std::string& encodedString_, const bool foldSpaces_ = false, const bool adobeMode_ = false)
        {
            ByteBuffer<ByteType> decodedByteBuffer;
            ByteBuffer<ByteType> byteBuffer(8192);
            typename ByteBuffer<ByteType>::SizeType bufferIterator(0);
            std::string::const_iterator encodedStringBegin(encodedString_.cbegin());
            std::string::const_iterator encodedStringEnd(encodedString_.cend());
            auto addToByteBuffer = [&decodedByteBuffer, &byteBuffer, &bufferIterator](const unsigned char byte_) -> void {
                if(bufferIterator < 8192) {
                    if constexpr(std::is_same_v<ByteType, unsigned char>) {
                        byteBuffer.At(bufferIterator) = byte_;
                    } else {
                        byteBuffer.At(bufferIterator) = static_cast<ByteType>(byte_);
                    }

                    bufferIterator += 1;
                } else {
                    decodedByteBuffer += byteBuffer;
                    bufferIterator = 0;

                    if constexpr(std::is_same_v<ByteType, unsigned char>) {
                        byteBuffer.At(bufferIterator) = byte_;
                    } else {
                        byteBuffer.At(bufferIterator) = static_cast<ByteType>(byte_);
                    }

                    bufferIterator += 1;
                }
            };

            if(adobeMode_) {
                for(std::string::const_iterator iter(encodedString_.cbegin()); iter != encodedString_.cend(); ++iter) {
                    if(*iter == '<' and *std::next(iter, 1) == '~' and std::next(iter, 2) != encodedString_.cend()) {
                        encodedStringBegin = std::next(iter, 2);

                        break;
                    } else if(*iter == ' ' or *iter == '\n') {
                        if(std::next(iter, 1) == encodedString_.cend()) {
                            throw Error(Error::Type::STRING_PARSE_ERROR);
                        } else {
                            continue;
                        }
                    } else {
                        throw Error(Error::Type::STRING_PARSE_ERROR);
                    }
                }

                for(std::string::const_reverse_iterator iter(encodedString_.crbegin()); iter != encodedString_.crend(); ++iter) {
                    if(*iter == '>' and *std::next(iter, 1) == '~' and std::next(iter, 2) != encodedString_.crend()) {
                        encodedStringEnd = std::next(iter, 2).base();

                        break;
                    } else if(*iter == ' ' or *iter == '\n') {
                        if(std::next(iter, 1) == encodedString_.crend()) {
                            throw Error(Error::Type::STRING_PARSE_ERROR);
                        } else {
                            continue;
                        }
                    } else {
                        throw Error(Error::Type::STRING_PARSE_ERROR);
                    }
                }
            }

            for(std::string::const_iterator iter(encodedStringBegin); iter != encodedStringEnd; ++iter) {
                unsigned long long bitsetNumber(0ULL);
                unsigned int counter(0U);

                switch(*iter) {
                    case ' ': continue;
                    case '\n': continue;
                    case 'y': {
                        if(foldSpaces_) {
                            for(unsigned int i(0U); i < 4U; ++i) {
                                addToByteBuffer(static_cast<unsigned char>(' '));
                            }

                            continue;
                        } else {
                            throw Error(Error::Type::STRING_PARSE_ERROR);
                        }
                    }
                    case 'z': {
                        for(unsigned int i(0U); i < 4U; ++i) {
                            addToByteBuffer(static_cast<unsigned char>('\0'));
                        }

                        continue;
                    }
                    default: {
                        for(std::string::const_iterator jter(iter); iter != encodedStringEnd; ++jter) {
                            if(*jter == ' ' or *jter == '\n') {
                                if(std::next(jter, 1) == encodedStringEnd) {
                                    break;
                                } else {
                                    std::advance(iter, 1);

                                    continue;
                                }
                            } else {
                                const unsigned long long partialBitsetNumber(std::bitset<charSize>(*jter).to_ullong());
                                counter += 1U;

                                if(partialBitsetNumber < 33ULL or partialBitsetNumber > 117ULL) {
                                    throw Error(Error::Type::STRING_PARSE_ERROR);
                                } else {
                                    switch(counter) {
                                        case 1U: bitsetNumber += (partialBitsetNumber - 33ULL) * (85ULL * 85ULL * 85ULL * 85ULL); break;
                                        case 2U: bitsetNumber += (partialBitsetNumber - 33ULL) * (85ULL * 85ULL * 85ULL); break;
                                        case 3U: bitsetNumber += (partialBitsetNumber - 33ULL) * (85ULL * 85ULL); break;
                                        case 4U: bitsetNumber += (partialBitsetNumber - 33ULL) * 85ULL; break;
                                        case 5U: bitsetNumber += partialBitsetNumber - 33ULL; break;
                                        default: UnreachableTerminate();
                                    }

                                    if(std::next(jter, 1) == encodedStringEnd or std::next(jter, 1) == std::next(iter, 5)) {
                                        iter = jter;

                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                {
                    const unsigned int previousCounter(counter);

                    while(counter < 5U) {
                        counter += 1U;

                        switch(counter) {
                            case 1U: bitsetNumber += 84ULL * 85ULL * 85ULL * 85ULL * 85ULL; break;
                            case 2U: bitsetNumber += 84ULL * 85ULL * 85ULL * 85ULL; break;
                            case 3U: bitsetNumber += 84ULL * 85ULL * 85ULL; break;
                            case 4U: bitsetNumber += 84ULL * 85ULL; break;
                            case 5U: bitsetNumber += 84ULL; break;
                            default: UnreachableTerminate();
                        }
                    }

                    counter = 5U - previousCounter;
                }

                const std::bitset<charSize * 4> bitset(bitsetNumber);

                switch(counter) {
                    case 0U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> charSize).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>(bitset.to_ullong()));

                        break;
                    }
                    case 1U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> charSize).to_ullong()));

                        break;
                    }
                    case 2U: {
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong()));
                        addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 2)).to_ullong()));

                        break;
                    }
                    case 3U: addToByteBuffer(static_cast<unsigned char>((bitset >> (charSize * 3)).to_ullong())); break;
                    case 4U: break;
                    default: UnreachableTerminate();
                }
            }

            if(bufferIterator > 0) {
                byteBuffer.Resize(bufferIterator);

                decodedByteBuffer += byteBuffer;
            }

            return decodedByteBuffer;
        }
    };
};