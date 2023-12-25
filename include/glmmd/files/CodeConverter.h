#ifndef GLMMD_FILES_CODE_CONVERTER_H_
#define GLMMD_FILES_CODE_CONVERTER_H_

#include <string>
#include <string_view>

namespace glmmd
{

template <class From, class To>
std::string codeCvt(std::string_view input)
{
    std::string output;
    output.reserve(input.size() * To::bytes / From::bytes);
    for (size_t i = 0; i < input.size();
         To::encode(From::decode(input, i), output))
        ;
    return output;
}

class UTF8
{
public:
    static void     encode(uint32_t u, std::string &output);
    static uint32_t decode(std::string_view input, size_t &i);

    static constexpr size_t bytes = 2;
};

class UTF16_LE
{
public:
    static void     encode(uint32_t u, std::string &output);
    static uint32_t decode(std::string_view input, size_t &i);

    static constexpr size_t bytes = 2;
};

class ShiftJIS
{
public:
    static uint32_t decode(std::string_view input, size_t &i);

    static constexpr size_t bytes = 3;
};

} // namespace glmmd

#endif
