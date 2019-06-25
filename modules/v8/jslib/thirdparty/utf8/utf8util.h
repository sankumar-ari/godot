#pragma once
#include <string>
#include "utf8.h"
namespace __internal {
template <int ToCharSize = sizeof(char), int FromCharSize = sizeof(wchar_t)>
struct utf8_converter;
/// \brief helper class that determines the size of wchar_t and choses correct conversion method to and from char
/// string.
template <>
struct utf8_converter<2, 1> {
  template <typename to_iterator, typename from_iterator>
  static to_iterator Convert(from_iterator start, from_iterator end, to_iterator result) {
    return utf8::utf8to16(start, end, result);
  }
};

template <>
struct utf8_converter<4, 1> {
  template <typename to_iterator, typename from_iterator>
  static to_iterator Convert(from_iterator start, from_iterator end, to_iterator result) {
    return utf8::utf8to32(start, end, result);
  }
};

template <>
struct utf8_converter<1, 2> {
  template <typename to_iterator, typename from_iterator>
  static to_iterator Convert(from_iterator start, from_iterator end, to_iterator result) {
    return utf8::utf16to8(start, end, result);
  }
};

template <>
struct utf8_converter<1, 4> {
  template <typename to_iterator, typename from_iterator>
  static to_iterator Convert(from_iterator start, from_iterator end, to_iterator result) {
    return utf8::utf32to8(start, end, result);
  }
};
}

/// \brief class to provide methods to convert Unicode string to UTF8 string and vice versa.
class UTF8 {
 public:
  /// \brief converts Unicode string to utf8 string.
  /// \param[in] value Unicode string.
  /// \return UTF8 string.
  static std::string Convert(const std::wstring& value) {
    std::string return_value;
    __internal::utf8_converter<sizeof(std::string::value_type), sizeof(std::wstring::value_type)>::Convert(
        value.begin(), value.end(), std::back_inserter(return_value));
    return return_value;
  }

  /// \brief converts UTF8 string to Unicode string.
  /// \param[in] value UTF8 string.
  /// \return Unicode string.
  static std::wstring Convert(const std::string& value) {
    std::wstring return_value;
    __internal::utf8_converter<sizeof(std::wstring::value_type), sizeof(std::string::value_type)>::Convert(
        value.begin(), value.end(), std::back_inserter(return_value));
    return return_value;
  }

  static std::string Convert(const std::u16string& value) {
    std::string return_value;
    __internal::utf8_converter<sizeof(std::string::value_type), sizeof(std::u16string::value_type)>::Convert(
        value.begin(), value.end(), std::back_inserter(return_value));
    return return_value;
  }
  
  static std::u16string Convert16(const std::string& value) {
    std::u16string return_value;
    __internal::utf8_converter<sizeof(std::u16string::value_type), sizeof(std::string::value_type)>::Convert(
        value.begin(), value.end(), std::back_inserter(return_value));
    return return_value;
  }
 private:
};
