from __future__ import annotations

import argparse
import json
import mimetypes
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import parse_qs, urlencode, urlparse, urlunparse
from urllib.request import Request, urlopen


ROOT = Path(__file__).resolve().parent
DEFAULT_ORANGEPI_BASE = os.getenv("ORANGEPI_MEMORY_BASE_URL", "http://192.168.1.190:8765")


def _json_bytes(payload: dict) -> bytes:
    return json.dumps(payload, ensure_ascii=False).encode("utf-8")


def _clean_base_url(value: str | None) -> str:
    base = (value or DEFAULT_ORANGEPI_BASE).strip().rstrip("/")
    parsed = urlparse(base)
    if parsed.scheme not in {"http", "https"} or not parsed.netloc:
        raise ValueError("香橙派地址必须是 http://IP:端口 或 https://域名")
    return base


def _target_url(base_url: str, raw_path: str) -> str:
    parsed = urlparse(raw_path)
    proxy_path = parsed.path.removeprefix("/proxy")
    if not proxy_path.startswith("/"):
        proxy_path = "/" + proxy_path

    query = parse_qs(parsed.query, keep_blank_values=True)
    query.pop("target", None)
    clean_query = urlencode(query, doseq=True)
    return f"{base_url}{proxy_path}" + (f"?{clean_query}" if clean_query else "")


class SimulatorHandler(BaseHTTPRequestHandler):
    server_version = "ESP32WebSimulator/1.0"

    def log_message(self, fmt: str, *args) -> None:
        print("%s - %s" % (self.address_string(), fmt % args))

    def do_OPTIONS(self) -> None:
        self._send_empty(204)

    def do_GET(self) -> None:
        if self.path.startswith("/proxy/"):
            self._proxy("GET")
            return
        self._serve_static()

    def do_POST(self) -> None:
        if self.path.startswith("/proxy/"):
            self._proxy("POST")
            return
        self._send_json(404, {"detail": "未找到接口"})

    def _serve_static(self) -> None:
        path = urlparse(self.path).path
        if path in {"", "/"}:
            path = "/index.html"
        file_path = (ROOT / path.lstrip("/")).resolve()
        if ROOT not in file_path.parents and file_path != ROOT:
            self._send_json(403, {"detail": "禁止访问"})
            return
        if not file_path.exists() or not file_path.is_file():
            self._send_json(404, {"detail": "文件不存在"})
            return

        content_type = mimetypes.guess_type(str(file_path))[0] or "application/octet-stream"
        data = file_path.read_bytes()
        self.send_response(200)
        self._send_common_headers(content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _proxy(self, method: str) -> None:
        try:
            parsed_self = urlparse(self.path)
            query = parse_qs(parsed_self.query)
            base_url = _clean_base_url(self.headers.get("X-OrangePi-Base") or query.get("target", [None])[0])
            timeout_ms = int(self.headers.get("X-OrangePi-Timeout-Ms", "3500"))
            timeout = max(0.2, min(timeout_ms / 1000.0, 15.0))
            target = _target_url(base_url, self.path)
        except ValueError as exc:
            self._send_json(400, {"detail": str(exc)})
            return

        body = b""
        if method == "POST":
            length = int(self.headers.get("Content-Length", "0") or "0")
            body = self.rfile.read(length) if length > 0 else b""

        headers = {"Accept": "application/json"}
        if self.headers.get("Content-Type"):
            headers["Content-Type"] = self.headers["Content-Type"]

        request = Request(target, data=body if method == "POST" else None, headers=headers, method=method)
        try:
            with urlopen(request, timeout=timeout) as response:
                data = response.read()
                content_type = response.headers.get("Content-Type", "application/json")
                self._send_bytes(response.status, data, content_type)
        except HTTPError as exc:
            data = exc.read()
            content_type = exc.headers.get("Content-Type", "application/json")
            self._send_bytes(exc.code, data, content_type)
        except URLError as exc:
            self._send_json(502, {"detail": f"无法访问香橙派服务: {exc.reason}"})
        except TimeoutError:
            self._send_json(504, {"detail": "访问香橙派服务超时"})

    def _send_common_headers(self, content_type: str) -> None:
        self.send_header("Content-Type", content_type)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, X-OrangePi-Base, X-OrangePi-Timeout-Ms")
        self.send_header("Cache-Control", "no-store")

    def _send_empty(self, status: int) -> None:
        self.send_response(status)
        self._send_common_headers("text/plain; charset=utf-8")
        self.send_header("Content-Length", "0")
        self.end_headers()

    def _send_json(self, status: int, payload: dict) -> None:
        self._send_bytes(status, _json_bytes(payload), "application/json; charset=utf-8")

    def _send_bytes(self, status: int, data: bytes, content_type: str) -> None:
        self.send_response(status)
        self._send_common_headers(content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)


def main() -> None:
    parser = argparse.ArgumentParser(description="ESP32 网页模拟器")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8780)
    args = parser.parse_args()

    server = ThreadingHTTPServer((args.host, args.port), SimulatorHandler)
    print(f"ESP32 网页模拟器: http://{args.host}:{args.port}")
    print(f"默认香橙派地址: {DEFAULT_ORANGEPI_BASE}")
    server.serve_forever()


if __name__ == "__main__":
    main()
