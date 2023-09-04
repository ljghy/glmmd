#include <stdexcept>

#include <json.hpp>

#include "ChatSession.h"

using json = nlohmann::json;

ChatSession::ChatSession()
{
    Py_Initialize();
    if (!Py_IsInitialized())
    {
        throw std::runtime_error("Failed to initialize Python interpreter");
    }

    std::string assetsDir = "apps/chat/assets";

    PyRun_SimpleString("import sys");
    PyRun_SimpleString(("sys.path.append('" + assetsDir + "')").c_str());

    auto ttsModule = PyImport_ImportModule("TTS");
    if (!ttsModule)
    {
        throw std::runtime_error("Failed to import TTS module");
    }

    auto ttsClass = PyObject_GetAttrString(ttsModule, "TTS");
    if (!ttsClass)
    {
        throw std::runtime_error("Failed to get TTS class");
    }

    m_ttsInstance = PyObject_CallObject(ttsClass, nullptr);
    if (!m_ttsInstance)
    {
        throw std::runtime_error("Failed to create TTS instance");
    }

    auto ret = PyObject_CallMethod(m_ttsInstance, "init", "s",
                                   (assetsDir + "/ssml.xml").c_str());
    if (!ret || !PyObject_IsTrue(ret))
    {
        throw std::runtime_error("Failed to initialize TTS instance");
    }

    Py_DECREF(ttsClass);
    Py_DECREF(ttsModule);
}

bool ChatSession::synthesizeSpeech(const std::string &text)
{
    auto ret =
        PyObject_CallMethod(m_ttsInstance, "synthesize", "s", text.c_str());

    bool success = ret && PyObject_IsTrue(PyTuple_GetItem(ret, 0));
    if (success)
    {
        std::string audioFilename = PyUnicode_AsUTF8(PyTuple_GetItem(ret, 1));
        loadAudio(audioFilename);

        std::string visemeFilename = PyUnicode_AsUTF8(PyTuple_GetItem(ret, 2));
        loadVisemes(visemeFilename);
    }

    Py_DECREF(ret);
    return success;
}

void ChatSession::loadAudio(const std::string &filename)
{
    m_audioFile.load(filename);
}

void ChatSession::loadVisemes(const std::string &filename)
{
    std::ifstream visemeFile(filename);

    json   data   = json::parse(visemeFile);
    size_t frames = 0;
    for (const auto &chunk : data)
        frames += chunk["BlendShapes"].size();

    m_visemes.resize(frames);
    size_t frame = 0;
    for (const auto &chunk : data)
        for (const auto &ratios : chunk["BlendShapes"])
        {
            for (size_t i = 0; i < nVisemeTypes; ++i)
                m_visemes[frame][i] = ratios[i].get<float>();
            ++frame;
        }
}

ChatSession::~ChatSession()
{
    Py_DECREF(m_ttsInstance);

    Py_Finalize();
}