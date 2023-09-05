import json
from multiprocessing import cpu_count
from llama_cpp import Llama


class LLM:

    def init(self, config_filename: str) -> bool:
        self.config_filename = config_filename

        try:
            self.messages = []
            self.temperature = 0.9
            self.max_tokens = -1

            with open(config_filename, "r") as f:
                self.config = json.load(f)

                if "messages" in self.config:
                    self.messages = self.config["messages"]
                if "temperature" in self.config:
                    self.temperature = self.config["temperature"]
                if "max_tokens" in self.config:
                    self.max_tokens = self.config["max_tokens"]

            if "n_threads" in self.config:
                self.n_threads = self.config["n_threads"]
            else:
                self.n_threads = cpu_count()
            if "n_gpu_layers" in self.config:
                self.n_gpu_layers = self.config["n_gpu_layers"]
            else:
                self.n_gpu_layers = 0
            if "n_ctx" in self.config:
                self.n_ctx = self.config["n_ctx"]
            else:
                self.n_ctx = 4096

            self.llm = Llama(model_path=self.config["model_path"],
                             n_threads=self.n_threads,
                             n_gpu_layers=self.n_gpu_layers,
                             n_ctx=self.n_ctx,
                             verbose=False)

        except FileNotFoundError:
            return False
        except KeyError:
            return False
        return True

    def get_response(self, text: str) -> tuple[bool, str, int]:
        text = text.strip()

        msg = self.messages + \
            [{"role": self.config["user_name"], "content": text}]

        prompt = self.config["system"] + "\n" + \
            "\n".join([(m["role"] + ": " + m["content"]) for m in msg]) + \
            f"\n{self.config['assistant_name']}:"

        response = self.llm(prompt=prompt, max_tokens=self.max_tokens,
                            temperature=self.temperature,
                            stop=[self.config["user_name"] + ":",
                                  self.config["assistant_name"] + ":"])

        response_text = response["choices"][0]["text"].strip()
        usage = response["usage"]["total_tokens"]

        return True, response_text, usage
