#!/usr/bin/env python3
import json
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer

PORT = int(sys.argv[1])
LAYER = sys.argv[2]
state = {"commitments": []}


class Handler(BaseHTTPRequestHandler):
    def _send(self, payload, status=200):
        body = json.dumps(payload).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path == "/health":
            self._send({"status": "ok", "layer": LAYER})
        else:
            self._send({"error": "not found"}, 404)

    def do_POST(self):
        if self.path != "/":
            self._send({"error": "not found"}, 404)
            return
        length = int(self.headers.get("Content-Length", "0"))
        req = json.loads(self.rfile.read(length) or "{}")
        method = req.get("method", "")
        params = req.get("params", [])
        rid = req.get("id", "1")
        if method in {"getinfo", "chain/info"}:
            res = {"layer": LAYER, "blocks": 0, "connections": 0}
        elif method == "commitments/submit":
            state["commitments"].append(params)
            res = {"status": "queued", "count": len(state["commitments"]) }
        elif method == "commitments/list":
            res = {"commitments": state["commitments"], "count": len(state["commitments"])}
        else:
            self._send({"jsonrpc": "2.0", "id": rid, "error": {"message": "method not found"}}, 400)
            return
        self._send({"jsonrpc": "2.0", "id": rid, "result": res})


HTTPServer(("127.0.0.1", PORT), Handler).serve_forever()
