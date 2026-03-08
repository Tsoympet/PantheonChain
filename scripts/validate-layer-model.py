#!/usr/bin/env python3
"""Validate layered architecture consistency from a canonical model file.

Checks:
- model schema sanity in configs/layer-model.json
- policy metadata presence in configs/layer-model.json
- expected layer IDs in l1/l2/l3 config filenames
- role naming convention in node_id (l1=miner, l2/l3=validator)
- monotonic checkpoint cadence (l3 interval < l2 interval < l1)
- l1_min_confirmation_depth and bridge_unlock_min_l1_depth present in all l1 configs
"""

from __future__ import annotations

import json
import pathlib
import sys
from typing import Any

NETWORKS = ("devnet", "testnet", "mainnet")
MODEL_PATH = pathlib.Path("configs/layer-model.json")

REQUIRED_POLICY_FIELDS = (
    "l1_min_confirmation_depth",
    "bridge_unlock_min_l1_depth",
    "checkpoint_freshness_slo_seconds",
    "relayer_liveness_threshold_seconds",
)


def fail(msg: str) -> None:
    print(f"LAYER MODEL ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def load_json(path: pathlib.Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:  # noqa: BLE001
        fail(f"{path}: invalid JSON ({exc})")


def validate_model(model: dict[str, Any]) -> dict[str, Any]:
    layers = model.get("layers")
    if not isinstance(layers, dict):
        fail(f"{MODEL_PATH}: missing object field 'layers'")

    required_layers = ("l1", "l2", "l3")
    for layer in required_layers:
        if layer not in layers:
            fail(f"{MODEL_PATH}: missing layer definition '{layer}'")
        if "name" not in layers[layer] or "node_role_hint" not in layers[layer]:
            fail(f"{MODEL_PATH}: layer '{layer}' requires 'name' and 'node_role_hint'")

    checkpoint_path = model.get("checkpoint_path")
    if checkpoint_path != ["l3", "l2", "l1"]:
        fail(f"{MODEL_PATH}: checkpoint_path must be ['l3', 'l2', 'l1']")

    policy = model.get("policy")
    if not isinstance(policy, dict):
        fail(f"{MODEL_PATH}: missing object field 'policy'")
    for field in REQUIRED_POLICY_FIELDS:
        if field not in policy:
            fail(f"{MODEL_PATH}: policy is missing required field '{field}'")
        if not isinstance(policy[field], (int, float)) or policy[field] <= 0:
            fail(f"{MODEL_PATH}: policy.{field} must be a positive number")

    return layers


model = load_json(MODEL_PATH)
layers = validate_model(model)
policy = model.get("policy", {})
expected_freshness = int(policy.get("checkpoint_freshness_slo_seconds", 0))
expected_liveness = int(policy.get("relayer_liveness_threshold_seconds", 0))

for network in NETWORKS:
    base = pathlib.Path("configs") / network
    cfgs = {layer: load_json(base / f"{layer}.json") for layer in ("l1", "l2", "l3")}

    for layer, cfg in cfgs.items():
        if cfg.get("layer") != layer:
            fail(f"{base}/{layer}.json: layer must be '{layer}'")

        node_id = str(cfg.get("node_id", "")).lower()
        layer_name = str(layers[layer]["name"]).lower()
        if layer_name not in node_id:
            fail(f"{base}/{layer}.json: node_id must include '{layer_name}'")

        role_hint = str(layers[layer]["node_role_hint"]).lower()
        if network == "mainnet" and role_hint not in node_id:
            fail(f"{base}/{layer}.json: mainnet node_id should include role hint '{role_hint}'")

    # Validate that all l1 configs carry confirmation-depth policy fields.
    l1_cfg = cfgs["l1"]
    for field in ("l1_min_confirmation_depth", "bridge_unlock_min_l1_depth"):
        val = l1_cfg.get(field)
        if val is None:
            fail(f"{base}/l1.json: missing required field '{field}'")
        if not isinstance(val, (int, float)) or val <= 0:
            fail(f"{base}/l1.json: '{field}' must be a positive number")

    for layer, cfg in cfgs.items():
        freshness = int(cfg.get("checkpoint_freshness_slo_seconds", 0))
        liveness = int(cfg.get("relayer_liveness_threshold_seconds", 0))
        if freshness != expected_freshness:
            fail(
                f"{base}/{layer}.json: checkpoint_freshness_slo_seconds must equal policy value {expected_freshness}"
            )
        if liveness != expected_liveness:
            fail(
                f"{base}/{layer}.json: relayer_liveness_threshold_seconds must equal policy value {expected_liveness}"
            )

    l1 = int(cfgs["l1"].get("commitment_interval", 0))
    l2 = int(cfgs["l2"].get("commitment_interval", 0))
    l3 = int(cfgs["l3"].get("commitment_interval", 0))
    if not (l3 < l2 < l1):
        fail(
            f"{base}: expected checkpoint cadence l3 < l2 < l1, got l1={l1}, l2={l2}, l3={l3}"
        )

print("Layer model validation passed")
