#!/usr/bin/env python3
import json
import pathlib
import sys

REQUIRED = ["layer", "node_id", "network", "p2p_port", "rpc_port", "data_dir", "commitment_interval"]


def fail(msg: str) -> None:
    print(f"CONFIG ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def validate(path: pathlib.Path) -> None:
    try:
        cfg = json.loads(path.read_text())
    except Exception as exc:
        fail(f"{path}: invalid JSON ({exc})")

    for key in REQUIRED:
        if key not in cfg:
            fail(f"{path}: missing required key '{key}'")

    if cfg["layer"] not in {"l1", "l2", "l3"}:
        fail(f"{path}: layer must be one of l1/l2/l3")

    for port_key in ("p2p_port", "rpc_port"):
        port = cfg[port_key]
        if not isinstance(port, int) or not (1 <= port <= 65535):
            fail(f"{path}: {port_key} must be an integer in range 1..65535")

    if not isinstance(cfg["commitment_interval"], int) or cfg["commitment_interval"] <= 0:
        fail(f"{path}: commitment_interval must be a positive integer")

    genesis = cfg.get("genesis_file")
    if genesis:
        genesis_path = pathlib.Path(genesis)
        if not genesis_path.exists():
            fail(f"{path}: genesis_file '{genesis}' does not exist")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        fail("usage: validate-config.py <config.json> [<config2.json> ...]")

    for arg in sys.argv[1:]:
        validate(pathlib.Path(arg))
    print("Config validation passed")
