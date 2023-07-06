#include <glmmd/files/CodeConverter.h>

namespace glmmd
{

std::string UTF16_LE_to_UTF8(const std::string &input)
{
    std::string output;
    output.reserve(input.size() / 2);

    size_t i = 0;
    while (i < input.size())
    {
        uint32_t u = 0; // code point

        if ((uint8_t(input[i + 1]) >> 2) == 0x36)
        {
            uint16_t lead  = *(uint16_t *)(input.data() + i) & 0x03FF;
            uint16_t trail = *(uint16_t *)(input.data() + i + 2) & 0x03FF;
            u              = (lead << 10 | trail) + 0x10000;
            i += 4;
        }
        else
        {
            u = *(uint16_t *)(input.data() + i);
            i += 2;
        }

        if (u <= 0x7F)
            output.push_back(u & 0xFF);
        else if (u <= 0x7FF)
        {
            output.push_back(0xC0 | ((u >> 6) & 0xFF));
            output.push_back(0x80 | (u & 0x3F));
        }
        else if (u <= 0xFFFF)
        {
            output.push_back(0xE0 | ((u >> 12) & 0xFF));
            output.push_back(0x80 | ((u >> 6) & 0x3F));
            output.push_back(0x80 | (u & 0x3F));
        }
        else // if (u <= 0x10FFFF)
        {
            output.push_back(0xF0 | ((u >> 18) & 0xFF));
            output.push_back(0x80 | ((u >> 12) & 0x3F));
            output.push_back(0x80 | ((u >> 6) & 0x3F));
            output.push_back(0x80 | (u & 0x3F));
        }
    }
    return output;
}

#include "ShiftJIS_convTable.inl"

std::string shiftJIS_to_UTF8(const std::string &input)
{
    std::string output;

    output.resize(3 * input.length(), ' ');

    size_t indexInput = 0, indexOutput = 0;

    while (indexInput < input.length())
    {
        char arraySection = ((uint8_t)input[indexInput]) >> 4;

        size_t arrayOffset;
        if (arraySection == 0x8)
            arrayOffset = 0x100; // these are two-byte shiftjis
        else if (arraySection == 0x9)
            arrayOffset = 0x1100;
        else if (arraySection == 0xE)
            arrayOffset = 0x2100;
        else
            arrayOffset = 0; // this is one byte shiftjis

        // determining real array offset
        if (arrayOffset)
        {
            arrayOffset += (((uint8_t)input[indexInput]) & 0xf) << 8;
            indexInput++;
            if (indexInput >= input.length())
                break;
        }
        arrayOffset += (uint8_t)input[indexInput++];
        arrayOffset <<= 1;

        // unicode number is...
        uint16_t unicodeValue = (shiftJIS_convTable[arrayOffset] << 8) |
                                shiftJIS_convTable[arrayOffset + 1];

        // converting to UTF8
        if (unicodeValue < 0x80)
        {
            output[indexOutput++] = static_cast<char>(unicodeValue);
        }
        else if (unicodeValue < 0x800)
        {
            output[indexOutput++] =
                static_cast<char>(0xC0 | (unicodeValue >> 6));
            output[indexOutput++] =
                static_cast<char>(0x80 | (unicodeValue & 0x3f));
        }
        else
        {
            output[indexOutput++] =
                static_cast<char>(0xE0 | (unicodeValue >> 12));
            output[indexOutput++] =
                static_cast<char>(0x80 | ((unicodeValue & 0xfff) >> 6));
            output[indexOutput++] =
                static_cast<char>(0x80 | (unicodeValue & 0x3f));
        }
    }

    output.resize(indexOutput); // remove the unnecessary bytes
    return output;
}

} // namespace glmmd
