#ifndef CHAT_CHAT_SESSION_H_
#define CHAT_CHAT_SESSION_H_

#include <string>
#include <atomic>

#include <Python.h>

#include "AudioFile.h"
#include "VisemeMotion.h"

enum class ChatSessionState
{
    Idle,
    Waiting,
    Ready,
    Playing
};

class ChatSession
{
public:
    ChatSession();
    ~ChatSession();

    bool getResponse(const std::string &text);
    bool synthesizeSpeech(const std::string &text);

    void sendText(const std::string &text);

    ChatSessionState getState() const { return m_state; }
    void             setState(ChatSessionState state) { m_state = state; }

    const std::string &response() const { return m_response; }
    size_t             usage() const { return m_usage; }

    const AudioFile<float>    &audioFile() const { return m_audioFile; }
    const std::vector<Viseme> &visemes() const { return m_visemes; }

private:
    void initLLM();
    void initTTS();

private:
    void loadAudio(const std::string &filename);
    void loadVisemes(const std::string &filename);

private:
    std::atomic<ChatSessionState> m_state;

    PyObject *m_llmInstance = nullptr;
    PyObject *m_ttsInstance = nullptr;

    std::string m_response;
    size_t      m_usage;

    AudioFile<float>    m_audioFile;
    std::vector<Viseme> m_visemes;
};

#endif
