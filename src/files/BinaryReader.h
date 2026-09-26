#ifndef GLMMD_FILES_BINARY_READER_H_
#define GLMMD_FILES_BINARY_READER_H_

// Internal helper (not installed): a small binary file reader shared by the
// PMX and VMD loaders. Every read is bounds/EOF checked, and element counts are
// validated against the remaining file size before allocation so a corrupt or
// malicious header cannot trigger a huge allocation.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace glmmd {

class BinaryReader {
public:
  BinaryReader() = default;
  BinaryReader(const BinaryReader &) = delete;
  BinaryReader &operator=(const BinaryReader &) = delete;

  void open(const std::filesystem::path &path) {
    m_fin.open(path, std::ios::binary);
    if (!m_fin)
      throw std::runtime_error("Failed to open file \"" + path.string() +
                               "\".");

    std::error_code ec;
    m_fileSize = std::filesystem::file_size(path, ec);
    if (ec)
      throw std::runtime_error("Failed to query size of file \"" +
                               path.string() + "\".");
  }

  void check() {
    if (!m_fin)
      throw std::runtime_error("Unexpected end of file or read error.");
  }

  std::uintmax_t bytesRemaining() {
    auto pos = m_fin.tellg();
    if (pos < 0)
      throw std::runtime_error("File stream error.");
    auto p = static_cast<std::uintmax_t>(pos);
    return p >= m_fileSize ? 0 : m_fileSize - p;
  }

  // Each element occupies at least `elementSize` bytes on disk, so a count that
  // exceeds bytesRemaining() / elementSize must be corrupt.
  void ensureAvailable(std::uintmax_t count, std::uintmax_t elementSize = 1) {
    check();
    if (elementSize != 0 && count > bytesRemaining() / elementSize)
      throw std::runtime_error("File is truncated or corrupt.");
  }

  void readBytes(void *dst, std::size_t n) {
    m_fin.read(reinterpret_cast<char *>(dst), static_cast<std::streamsize>(n));
    check();
  }

  template <int count = 1> void readFloat(float &val) {
    readBytes(&val, sizeof(float) * count);
  }

  template <typename T> void read(T &val) { readBytes(&val, sizeof(T)); }

  // Reads a little-endian unsigned integer of `sz` (1, 2, or 4) bytes and
  // widens it into `val`.
  template <typename UIntType> void readUInt(UIntType &val, int sz) {
    switch (sz) {
    case 1: {
      uint8_t tmp;
      readBytes(&tmp, 1);
      val = static_cast<UIntType>(tmp);
      break;
    }
    case 2: {
      uint16_t tmp;
      readBytes(&tmp, 2);
      val = static_cast<UIntType>(tmp);
      break;
    }
    case 4: {
      uint32_t tmp;
      readBytes(&tmp, 4);
      val = static_cast<UIntType>(tmp);
      break;
    }
    default:
      throw std::runtime_error("File contains an invalid index size.");
    }
  }

  // Reads a little-endian signed integer of `sz` (1, 2, or 4) bytes,
  // sign-extending it into `val`.
  template <typename IntType> void readInt(IntType &val, int sz) {
    switch (sz) {
    case 1: {
      int8_t tmp;
      readBytes(&tmp, 1);
      val = static_cast<IntType>(tmp);
      break;
    }
    case 2: {
      int16_t tmp;
      readBytes(&tmp, 2);
      val = static_cast<IntType>(tmp);
      break;
    }
    case 4: {
      int32_t tmp;
      readBytes(&tmp, 4);
      val = static_cast<IntType>(tmp);
      break;
    }
    default:
      throw std::runtime_error("File contains an invalid index size.");
    }
  }

private:
  std::ifstream m_fin;
  std::uintmax_t m_fileSize = 0;
};

} // namespace glmmd

#endif
