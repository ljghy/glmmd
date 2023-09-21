#ifndef GLMMD_FILES_CODE_CONVERTER_H_
#define GLMMD_FILES_CODE_CONVERTER_H_

#include <string>

namespace glmmd
{

extern std::string UTF16_LE_to_UTF8(const std::string &input);
extern std::string UTF8_to_UTF16_LE(const std::string &input);
extern std::string shiftJIS_to_UTF8(const std::string &input);

} // namespace glmmd

#endif
