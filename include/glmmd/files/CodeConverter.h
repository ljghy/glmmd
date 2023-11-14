#ifndef GLMMD_FILES_CODE_CONVERTER_H_
#define GLMMD_FILES_CODE_CONVERTER_H_

#include <string>
#include <string_view>

namespace glmmd::CodeCvt
{

std::string UTF16_LE_to_UTF8(std::string_view input);
std::string UTF8_to_UTF16_LE(std::string_view input);
std::string shiftJIS_to_UTF8(std::string_view input);

} // namespace glmmd::CodeCvt

#endif
