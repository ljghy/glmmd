import os
import re
import json
from datetime import datetime
import azure.cognitiveservices.speech as speechsdk


class TTS:

    def init(self, ssml_filename) -> bool:
        try:
            self.speech_config = speechsdk.SpeechConfig(
                subscription=os.environ['SPEECH_KEY'],
                region=os.environ['SPEECH_REGION'])

            with open(ssml_filename, 'r') as f:
                self.ssml_string = f.read()

        except FileNotFoundError:
            return False
        except KeyError:
            return False

        self.speech_synthesizer = speechsdk.SpeechSynthesizer(
            speech_config=self.speech_config, audio_config=None)
        self.speech_synthesizer.viseme_received.connect(
            self._viseme_received_cb)

        return True

    def _viseme_received_cb(self, evt: speechsdk.SessionEventArgs):
        self.viseme_key_frames.append(json.loads(evt.animation))

    def _preprocess(self, text: str) -> str:
        text = re.sub(r'[\n\r]', '', text)
        text = re.sub(
            r"\(.*?\)|\{.*?\}|\[.*?\]|```.*?```|\*.*?\*", "<break />", text)
        text = re.sub("(?i)nya+n*~*",
                      "<phoneme alphabet=\"ipa\" ph=\"njan\"></phoneme>", text)
        return text

    def synthesize(self, text: str) -> tuple[bool, str, str]:
        ssml_string = self.ssml_string.format(text=self._preprocess(text))

        self.viseme_key_frames = []
        result = self.speech_synthesizer.speak_ssml_async(ssml_string).get()
        stream = speechsdk.AudioDataStream(result)

        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        wav_filename = f'tts_{timestamp}.wav'
        viseme_filename = f'viseme_{timestamp}.json'

        stream.save_to_wav_file(wav_filename)
        with open(viseme_filename, 'w') as f:
            json.dump(self.viseme_key_frames, f)

        if result.reason == speechsdk.ResultReason.SynthesizingAudioCompleted:
            return True, wav_filename, viseme_filename
        else:
            return False, "TTS failed", ""
