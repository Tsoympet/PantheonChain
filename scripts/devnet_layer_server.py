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
    "stakes": {},
    "governance": {"proposals": [], "next_id": 1, "votes": []},
    "treasury": {"total": 1000000, "core_development": 500000, "grants": 200000, "operations": 200000, "emergency": 100000},
    "ostracism": {"banned": []},
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
        elif method == "staking/stake":
            p = params[0]
            addr = p.get("address", "")
            amount = p.get("amount", 0)
            layer = p.get("layer", LAYER)
            state["stakes"].setdefault(addr, 0)
            state["stakes"][addr] += amount
            state["height"] += 1
            persist()
            res = {"status": "accepted", "address": addr, "amount": amount, "layer": layer}
        elif method == "staking/unstake":
            p = params[0]
            addr = p.get("address", "")
            amount = p.get("amount", 0)
            current = state["stakes"].get(addr, 0)
            state["stakes"][addr] = max(0, current - amount)
            state["height"] += 1
            persist()
            res = {"status": "accepted", "address": addr, "amount": amount}
        elif method == "staking/get_power":
            addr = params[0].get("address", "")
            power = 1 if state["stakes"].get(addr, 0) > 0 else 0
            res = {"address": addr, "voting_power": power, "source": "one_address_one_vote",
                   "total_voters": len([a for a, s in state["stakes"].items() if s > 0])}
        elif method in {"staking_getValidator", "staking/getValidator"}:
            addr = params[0].get("address", "")
            stake = state["stakes"].get(addr, 0)
            res = {"address": addr, "stake": stake, "active": stake > 0}
        elif method in {"staking_listValidators", "staking/listValidators"}:
            validators = [{"address": a, "stake": s, "active": s > 0}
                          for a, s in state["stakes"].items()]
            res = {"validators": validators, "count": len(validators)}
        elif method in {"staking_getDelegation", "staking/getDelegation"}:
            addr = params[0].get("address", "") if params else ""
            res = {"address": addr, "delegated": state["stakes"].get(addr, 0)}
        elif method in {"governance_submitProposal", "governance/submitProposal"}:
            p = params[0]
            pid = state["governance"]["next_id"]
            state["governance"]["next_id"] += 1
            proposal = {
                "proposal_id": pid,
                "title": p.get("title", ""),
                "description": p.get("description", ""),
                "proposer": p.get("proposer", ""),
                "status": "voting",
                "yes_votes": 0, "no_votes": 0, "abstain_votes": 0, "veto_votes": 0,
                "quorum_requirement": 1000, "approval_threshold": 50,
                "deposit_amount": p.get("deposit_amount", 0),
            }
            state["governance"]["proposals"].append(proposal)
            state["height"] += 1
            persist()
            res = {"proposal_id": pid, "status": "accepted"}
        elif method in {"governance_getProposal", "governance/getProposal"}:
            pid = params[0].get("proposal_id") if params else None
            found = next((p for p in state["governance"]["proposals"]
                          if p.get("proposal_id") == pid), None)
            res = {"found": bool(found), "proposal": found}
        elif method in {"governance_listProposals", "governance/listProposals"}:
            res = {"proposals": state["governance"]["proposals"],
                   "count": len(state["governance"]["proposals"])}
        elif method in {"governance_castVote", "governance/castVote"}:
            p = params[0]
            pid = p.get("proposal_id")
            vote = p.get("vote", "yes")
            voter = p.get("voter", "")
            for proposal in state["governance"]["proposals"]:
                if proposal.get("proposal_id") == pid:
                    key = f"{vote}_votes"
                    if key in proposal:
                        proposal[key] += 1
                    break
            state["governance"]["votes"].append({"proposal_id": pid, "voter": voter, "vote": vote})
            persist()
            res = {"status": "accepted", "proposal_id": pid, "vote": vote}
        elif method in {"governance_executeProposal", "governance/executeProposal"}:
            pid = params[0].get("proposal_id") if params else None
            for proposal in state["governance"]["proposals"]:
                if proposal.get("proposal_id") == pid:
                    proposal["status"] = "executed"
                    break
            persist()
            res = {"status": "executed", "proposal_id": pid}
        elif method in {"governance_getVoteRecord", "governance/getVoteRecord"}:
            pid = params[0].get("proposal_id") if params else None
            voter = params[0].get("voter", "") if params else ""
            record = next((v for v in state["governance"]["votes"]
                           if v.get("proposal_id") == pid and v.get("voter") == voter), None)
            res = {"found": bool(record), "record": record}
        elif method in {"treasury_getBalance", "treasury/getBalance"}:
            res = state["treasury"]
        elif method in {"treasury_listGrants", "treasury/listGrants"}:
            res = {"grants": [], "count": 0}
        elif method in {"ostracism_getStatus", "ostracism/getStatus"}:
            addr = params[0].get("address", "") if params else ""
            banned = any(b.get("address") == addr for b in state["ostracism"]["banned"])
            res = {"address": addr, "banned": banned,
                   "ban_end": 0 if not banned else next(
                       b.get("ban_end", 0) for b in state["ostracism"]["banned"]
                       if b.get("address") == addr)}
        else:
            self._send({"jsonrpc": "2.0", "id": rid, "error": {"message": "method not found"}}, 400)
            return

        self._send({"jsonrpc": "2.0", "id": rid, "result": res})


load_state()
HTTPServer(("127.0.0.1", PORT), Handler).serve_forever()
