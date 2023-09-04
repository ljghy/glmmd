#ifndef CHAT_CHAT_SESSION_H_
#define CHAT_CHAT_SESSION_H_

#include <string>

#include <Python.h>

#include "AudioFile.h"

inline constexpr size_t nVisemeTypes = 55;

using Viseme = std::array<float, nVisemeTypes>;

class ChatSession
{
public:
    ChatSession();
    ~ChatSession();

    bool synthesizeSpeech(const std::string &text);

private:
    void loadAudio(const std::string &filename);
    void loadVisemes(const std::string &filename);

private:
    PyObject *m_ttsInstance;

    std::string         m_text;
    AudioFile<float>    m_audioFile;
    std::vector<Viseme> m_visemes;
};

#endif
