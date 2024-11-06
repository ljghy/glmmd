#include <fstream>

#include <glmmd/files/CodeConverter.h>
#include <glmmd/files/ObjFileDumper.h>

namespace glmmd
{

void dumpObjFile(const std::filesystem::path &path, const ModelData &modelData,
                 const RenderData *renderData)
{
    std::ofstream objFile(path);
    if (!objFile)
        throw std::runtime_error("Failed to open file: " + path.string());

    objFile << "# mmd2obj\n";
    auto mtlPath = path;
    mtlPath.replace_extension(".mtl");
    objFile << "mtllib " << mtlPath.filename().string() << '\n';

    size_t nV = modelData.vertices.size();
    if (renderData)
    {
        for (size_t i = 0; i < nV; ++i)
        {
            auto pos = renderData->getVertexPosition(i);
            objFile << "v " << pos.x << ' ' << pos.y << ' ' << pos.z << '\n';
        }
        for (size_t i = 0; i < nV; ++i)
        {
            auto normal = renderData->getVertexNormal(i);
            objFile << "vn " << normal.x << ' ' << normal.y << ' ' << normal.z
                    << '\n';
        }
        for (size_t i = 0; i < nV; ++i)
        {
            glm::vec2 uv = renderData->getVertexUV(i);
            objFile << "vt " << uv.x << ' ' << 1.f - uv.y << '\n';
        }
    }
    else
    {
        for (size_t i = 0; i < nV; ++i)
        {
            auto pos = modelData.vertices[i].position;
            objFile << "v " << pos.x << ' ' << pos.y << ' ' << pos.z << '\n';
        }
        for (size_t i = 0; i < nV; ++i)
        {
            auto normal = modelData.vertices[i].normal;
            objFile << "vn " << normal.x << ' ' << normal.y << ' ' << normal.z
                    << '\n';
        }
        for (size_t i = 0; i < nV; ++i)
        {
            glm::vec2 uv = modelData.vertices[i].uv;
            objFile << "vt " << uv.x << ' ' << 1.f - uv.y << '\n';
        }
    }

    size_t indexOffset = 0;
    for (size_t i = 0; i < modelData.materials.size(); ++i)
    {
        const auto &mat = modelData.materials[i];
        objFile << "g " << std::to_string(i) << '\n';
        objFile << "\nusemtl " << std::to_string(i) << '\n';
        for (int j = 0; j < mat.indicesCount; j += 3)
        {
            auto u = modelData.indices[indexOffset + j + 0] + 1;
            auto v = modelData.indices[indexOffset + j + 1] + 1;
            auto w = modelData.indices[indexOffset + j + 2] + 1;
            objFile << "f " << u << '/' << u << '/' << u << ' ' << v << '/' << v
                    << '/' << v << ' ' << w << '/' << w << '/' << w << '\n';
        }
        indexOffset += mat.indicesCount;
    }
    objFile.close();

    std::ofstream mtlFile(mtlPath);
    if (!mtlFile)
        throw std::runtime_error("Failed to open file: " + mtlPath.string());

    mtlFile << "# mmd2obj\n";
    for (size_t i = 0; i < modelData.materials.size(); ++i)
    {
        const auto &mat = modelData.materials[i];
        mtlFile << "newmtl " << std::to_string(i) << '\n';

        auto diffuse =
            renderData ? renderData->materials[i].diffuse : mat.diffuse;
        mtlFile << "Kd " << diffuse.r << ' ' << diffuse.g << ' ' << diffuse.b
                << '\n';
        mtlFile << "d " << diffuse.a << '\n';

        auto specular =
            renderData ? renderData->materials[i].specular : mat.specular;
        mtlFile << "Ks " << specular.r << ' ' << specular.g << ' ' << specular.b
                << '\n';
        float specularPower = renderData
                                  ? renderData->materials[i].specularPower
                                  : mat.specularPower;
        mtlFile << "Ns " << specularPower << '\n';
        mtlFile << "illum 2\n";

        auto ambient =
            renderData ? renderData->materials[i].ambient : mat.ambient;
        mtlFile << "Ka " << ambient.r << ' ' << ambient.g << ' ' << ambient.b
                << '\n';

        if (mat.textureIndex != -1)
        {
            const auto &tex = modelData.textures[mat.textureIndex];
            mtlFile << "map_Kd " << tex.path << '\n';
        }

        mtlFile << '\n';
    }
    mtlFile.close();
}

} // namespace glmmd
