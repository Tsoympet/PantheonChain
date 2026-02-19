#!/usr/bin/env python3
import json
import pathlib
import sys

REQUIRED = ["layer", "node_id", "network", "p2p_port", "rpc_port", "data_dir", "commitment_interval"]
VALID_LAYERS = {"l1", "l2", "l3"}
VALID_NETWORKS = {"devnet", "testnet", "mainnet"}


def fail(msg: str) -> None:
    print(f"CONFIG ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def validate(path: pathlib.Path) -> dict:
    try:
        cfg = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        fail(f"{path}: invalid JSON ({exc})")

    for key in REQUIRED:
        if key not in cfg:
            fail(f"{path}: missing required key '{key}'")

    if cfg["layer"] not in VALID_LAYERS:
        fail(f"{path}: layer must be one of {sorted(VALID_LAYERS)}")

    if cfg["network"] not in VALID_NETWORKS:
        fail(f"{path}: network must be one of {sorted(VALID_NETWORKS)}")

    if not isinstance(cfg["node_id"], str) or len(cfg["node_id"].strip()) < 3:
        fail(f"{path}: node_id must be a non-empty string (>=3 chars)")

    if not isinstance(cfg["data_dir"], str) or not cfg["data_dir"].strip():
        fail(f"{path}: data_dir must be a non-empty string")

    for port_key in ("p2p_port", "rpc_port"):
        port = cfg[port_key]
        if not isinstance(port, int) or not (1 <= port <= 65535):
            fail(f"{path}: {port_key} must be an integer in range 1..65535")

    if cfg["p2p_port"] == cfg["rpc_port"]:
        fail(f"{path}: p2p_port and rpc_port must be different")

    if not isinstance(cfg["commitment_interval"], int) or cfg["commitment_interval"] <= 0:
        fail(f"{path}: commitment_interval must be a positive integer")

    genesis = cfg.get("genesis_file")
    if genesis:
        genesis_path = pathlib.Path(genesis)
        if not genesis_path.exists():
            fail(f"{path}: genesis_file '{genesis}' does not exist")

    path_hint = path.as_posix().lower()
    if "/devnet/" in path_hint and cfg["network"] != "devnet":
        fail(f"{path}: network must be 'devnet' for configs/devnet files")
    if "/testnet/" in path_hint and cfg["network"] != "testnet":
        fail(f"{path}: network must be 'testnet' for configs/testnet files")
    if "/mainnet/" in path_hint and cfg["network"] != "mainnet":
        fail(f"{path}: network must be 'mainnet' for configs/mainnet files")

    return cfg


if __name__ == "__main__":
    if len(sys.argv) < 2:
        fail("usage: validate-config.py <config.json> [<config2.json> ...]")

    seen_ports = {}
    for arg in sys.argv[1:]:
        path = pathlib.Path(arg)
        cfg = validate(path)
        for kind in ("p2p_port", "rpc_port"):
            port = cfg[kind]
            if port in seen_ports:
                prev = seen_ports[port]
                fail(f"{path}: {kind}={port} conflicts with {prev}")
            seen_ports[port] = f"{path}:{kind}"

    print("Config validation passed")
