import os
import json
import requests
from requests.adapters import HTTPAdapter, Retry

requests.packages.urllib3.disable_warnings()


class LLM:

    def init(self, config_filename: str) -> bool:
        self.config_filename = config_filename
        try:
            self._openai_api_key = os.environ["OPENAI_API_KEY"]
            self.model_max_tokens = 4096
            self.messages = []
            self.temperature = 0.9
            self.proxies = {}

            with open(config_filename, "r") as f:
                self.config = json.load(f)

                if "messages" in self.config:
                    self.messages = self.config["messages"]
                if "proxies" in self.config:
                    self.proxies = self.config["proxies"]

            self._session = requests.Session()
            retry = Retry(total=5, backoff_factor=0.5, status_forcelist=[429])
            adapter = HTTPAdapter(max_retries=retry)
            self._session.mount("http://", adapter)
            self._session.mount("https://", adapter)

        except FileNotFoundError:
            return False
        except KeyError:
            return False
        return True

    def _send_request(self, data: dict):
        response = self._session.post(
            url="https://api.openai.com/v1/chat/completions",
            headers={
                "Content-Type": "application/json",
                "Authorization": "Bearer " + self._openai_api_key
            },
            json=data,
            proxies=self.proxies,
            verify=False)
        return response

    def get_response(self, text: str) -> tuple[bool, str, int]:
        text = text.strip()

        system_prompt = [{
            "role": "system",
            "content": self.config["system"]
        }]

        messages = self.messages + [{"role": "user", "content": text}]

        data = {
            "model": "gpt-3.5-turbo",
            "messages": system_prompt + messages,
            "stop": [self.config["user_name"] + ":"]
        }

        if "temperature" in self.config:
            data["temperature"] = self.config["temperature"]
        if "max_tokens" in self.config:
            data["max_tokens"] = self.config["max_tokens"]

        response = None
        try:
            response = self._send_request(data)
        except requests.exceptions.ConnectionError:
            return False, "Connection error", 0
        except requests.exceptions.Timeout:
            return False, "Connection timeout", 0

        if response.status_code == 200:
            response_message = response.json()["choices"][0]["message"]
            usage = response.json()["usage"]["total_tokens"]
            self.messages = messages + [response_message]
            return True, response_message["content"], usage
        else:
            return False, f"Request failed with code {response.status_code}", 0
