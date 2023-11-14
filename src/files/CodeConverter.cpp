#include <glmmd/files/CodeConverter.h>

namespace glmmd::CodeCvt
{

static uint32_t decodeUTF8(std::string_view input, size_t &i)
{
    uint32_t u = 0;

    if (!(input[i] & 0x80))
    {
        u = uint8_t(input[i++]);
    }
    else if ((input[i] & 0xE0) == 0xC0)
    {
        u = (uint8_t(input[i++]) & 0x1F) << 6;
        u |= uint8_t(input[i++]) & 0x3F;
    }
    else if ((uint8_t(input[i]) & 0xF0) == 0xE0)
    {
        u = (uint8_t(input[i++]) & 0x0F) << 12;
        u |= (uint8_t(input[i++]) & 0x3F) << 6;
        u |= uint8_t(input[i++]) & 0x3F;
    }
    else if ((uint8_t(input[i]) & 0xF8) == 0xF0)
    {
        u = (uint8_t(input[i++]) & 0x07) << 18;
        u |= (uint8_t(input[i++]) & 0x3F) << 12;
        u |= (uint8_t(input[i++]) & 0x3F) << 6;
        u |= uint8_t(input[i++]) & 0x3F;
    }

    return u;
}

static void encodeUTF8(uint32_t u, std::string &output)
{
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

static uint32_t decodeUTF16_LE(std::string_view input, size_t &i)
{
    uint32_t u = 0;

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

    return u;
}

static void encodeUTF16_LE(uint32_t u, std::string &output)
{
    if (u <= 0xFFFF)
    {
        output.push_back(u & 0xFF);
        output.push_back((u >> 8) & 0xFF);
    }
    else
    {
        u -= 0x10000;

        uint16_t lead  = 0xD800 | ((u >> 10) & 0x3FF);
        uint16_t trail = 0xDC00 | (u & 0x3FF);

        output.push_back(lead & 0xFF);
        output.push_back((lead >> 8) & 0xFF);
        output.push_back(trail & 0xFF);
        output.push_back((trail >> 8) & 0xFF);
    }
}

std::string UTF16_LE_to_UTF8(std::string_view input)
{
    std::string output;
    output.reserve(input.size() / 2);

    size_t i = 0;
    while (i < input.size())
        encodeUTF8(decodeUTF16_LE(input, i), output);
    return output;
}

std::string UTF8_to_UTF16_LE(std::string_view input)
{
    std::string output;
    output.reserve(input.size() * 2);

    size_t i = 0;
    while (i < input.size())
        encodeUTF16_LE(decodeUTF8(input, i), output);
    return output;
}

#include "ShiftJIS_convTable.inl"

std::string shiftJIS_to_UTF8(std::string_view input)
{
    std::string output;

    output.reserve(3 * input.length());

    size_t indexInput = 0;

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

        encodeUTF8(unicodeValue, output);
    }

    return output;
}

} // namespace glmmd::CodeCvt
