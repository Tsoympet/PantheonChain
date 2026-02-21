#!/usr/bin/env python3
import json
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path

PORT = int(sys.argv[1])
LAYER = sys.argv[2]
STATE_FILE = Path(sys.argv[3]) if len(sys.argv) > 3 else None

state = {
    "height": 1,
    "transfers": [],
    "contracts": {},
    "commitments": [],
    "staking": {"validators": 1, "active": True},
}


def load_state() -> None:
    if STATE_FILE and STATE_FILE.exists():
        loaded = json.loads(STATE_FILE.read_text(encoding="utf-8"))
        state.update(loaded)


def persist() -> None:
    if STATE_FILE:
        STATE_FILE.parent.mkdir(parents=True, exist_ok=True)
        STATE_FILE.write_text(json.dumps(state), encoding="utf-8")


class Handler(BaseHTTPRequestHandler):
    def _send(self, payload, status=200):
        body = json.dumps(payload).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format, *args):
        return

    def do_GET(self):
        if self.path == "/health":
            self._send({"status": "ok", "layer": LAYER, "height": state["height"]})
            return
        if self.path == "/chain/info":
            self._send({"layer": LAYER, "height": state["height"], "network": "devnet"})
            return
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
            res = {"layer": LAYER, "height": state["height"], "connections": 2}
        elif method == "staking/status":
            res = state["staking"]
        elif method == "transfers/send":
            tx = params[0]
            tx["txid"] = f"{LAYER}-tx-{len(state['transfers']) + 1}"
            state["transfers"].append(tx)
            state["height"] += 1
            persist()
            res = {"accepted": True, "txid": tx["txid"]}
        elif method == "transfers/get":
            txid = params[0]
            found = next((t for t in state["transfers"] if t.get("txid") == txid), None)
            res = {"found": bool(found), "tx": found}
        elif method == "evm/deploy":
            code = params[0].get("bytecode", "")
            address = f"0x{len(state['contracts']) + 1:040x}"
            state["contracts"][address] = {"bytecode": code, "calls": []}
            state["height"] += 1
            persist()
            res = {"deployed": True, "address": address}
        elif method == "evm/call":
            address = params[0].get("address")
            data = params[0].get("data", "0x")
            contract = state["contracts"].get(address)
            if not contract:
                self._send({"jsonrpc": "2.0", "id": rid, "error": {"message": "contract not found"}}, 404)
                return
            contract["calls"].append(data)
            persist()
            res = {"ok": True, "return": "0x01"}
        elif method == "commitments/submit":
            item = params[0]
            item.setdefault("id", f"{LAYER}-commit-{len(state['commitments']) + 1}")
            state["commitments"].append(item)
            state["height"] += 1
            persist()
            res = {"status": "queued", "count": len(state["commitments"]), "id": item["id"]}
        elif method == "commitments/list":
            res = {"commitments": state["commitments"], "count": len(state["commitments"])}
        elif method == "commitments/get":
            cid = params[0]
            found = next((c for c in state["commitments"] if c.get("id") == cid), None)
            res = {"found": bool(found), "commitment": found}
        else:
            self._send({"jsonrpc": "2.0", "id": rid, "error": {"message": "method not found"}}, 400)
            return

        self._send({"jsonrpc": "2.0", "id": rid, "result": res})


load_state()
HTTPServer(("127.0.0.1", PORT), Handler).serve_forever()
