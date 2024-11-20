#include <stdexcept>
#include <algorithm>
#include <fstream>

#include <glmmd/files/VpdFileLoader.h>

namespace glmmd
{

std::shared_ptr<VpdData> VpdFileLoader::load(const std::filesystem::path &path)
{
    std::ifstream fin(path);
    if (!fin)
        throw std::runtime_error("Failed to open file \"" + path.string() +
                                 "\".");

    auto data = std::make_shared<VpdData>();

    m_pos = 0;
    m_buffer.assign(std::istreambuf_iterator<char>(fin),
                    std::istreambuf_iterator<char>{});

    removeComment();

    loadHeader(*data);
    loadBoneData(*data);

    return data;
}

void VpdFileLoader::removeComment()
{
    size_t i = 0;
    size_t j = 0;
    while (i < m_buffer.size())
    {
        if (i + 1 < m_buffer.size() && m_buffer[i] == '/' &&
            m_buffer[i + 1] == '/')
        {
            constexpr char newLineCh[]{'\r', '\n'};

            auto end =
                std::find_first_of(m_buffer.begin() + i, m_buffer.end(),
                                   std::begin(newLineCh), std::end(newLineCh));

            i = end - m_buffer.begin();
        }
        else
        {
            m_buffer[j++] = m_buffer[i++];
        }
    }
    m_buffer.resize(j);
}

void VpdFileLoader::readLine(std::string &line)
{
    line.clear();
    while (m_pos < m_buffer.size() && m_buffer[m_pos] != '\r' &&
           m_buffer[m_pos] != '\n')
        line.push_back(m_buffer[m_pos++]);
}

void VpdFileLoader::skipSpace()
{
    while (m_pos < m_buffer.size() && std::isspace(m_buffer[m_pos]))
        ++m_pos;
}

bool VpdFileLoader::readUntil(char end)
{
    while (m_pos < m_buffer.size() && m_buffer[m_pos] != end)
        ++m_pos;
    return m_pos < m_buffer.size() && m_buffer[m_pos++] == end;
}

bool VpdFileLoader::readUntil(std::string &buffer, char end)
{
    buffer.clear();
    while (m_pos < m_buffer.size() && m_buffer[m_pos] != end)
        buffer.push_back(m_buffer[m_pos++]);
    return m_pos < m_buffer.size() && m_buffer[m_pos++] == end;
}

bool VpdFileLoader::readUntilSpace(std::string &buffer)
{
    buffer.clear();
    while (m_pos < m_buffer.size() && !std::isspace(m_buffer[m_pos]))
        buffer.push_back(m_buffer[m_pos++]);
    return !buffer.empty();
}

bool VpdFileLoader::s2i(const std::string &str, int &value)
{
    try
    {
        value = std::stoi(str);
        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

bool VpdFileLoader::s2f(const std::string &str, float &value)
{
    try
    {
        value = std::stof(str);
        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

void VpdFileLoader::loadHeader(VpdData &data)
{
    std::string header;
    readLine(header);
    if (header != "Vocaloid Pose Data file")
        throw std::runtime_error("VPD file format error.");

    skipSpace();
    if (!readUntil(data.modelName, ';'))
        throw std::runtime_error("VPD file format error.");
}

void VpdFileLoader::loadBoneData(VpdData &data)
{
    skipSpace();

    std::string buffer;

    int boneCount = -1;
    if (!readUntil(buffer, ';') || !s2i(buffer, boneCount) || boneCount < 0)
        throw std::runtime_error("VPD file format error.");

    data.bones.resize(boneCount);

    for (int i = 0; i < boneCount; ++i)
    {
        m_pos = m_buffer.find("Bone", m_pos);
        if (m_pos == std::string::npos)
            break;

        m_pos += 4;

        int boneIndex = -1;
        if (!readUntil(buffer, '{') || !s2i(buffer, boneIndex) || boneIndex < 0)
            throw std::runtime_error("VPD file format error.");

        // skipSpace();

        auto &bone = data.bones[boneIndex];
        if (!readUntilSpace(bone.name))
            throw std::runtime_error("VPD file format error.");

        for (int j = 0; j < 3; ++j)
        {
            skipSpace();
            if (!readUntil(buffer, j == 2 ? ';' : ',') ||
                !s2f(buffer, bone.translation[j]))
                throw std::runtime_error("VPD file format error.");
        }

        float q[4];
        for (int j = 0; j < 4; ++j)
        {
            skipSpace();
            if (!readUntil(buffer, j == 3 ? ';' : ',') || !s2f(buffer, q[j]))
                throw std::runtime_error("VPD file format error.");
        }
        bone.rotation.x = q[0];
        bone.rotation.y = q[1];
        bone.rotation.z = q[2];
        bone.rotation.w = q[3];

        if (!readUntil('}'))
            throw std::runtime_error("VPD file format error.");
    }
}

} // namespace glmmd