#!/usr/bin/env python3
"""Indoor comfort from CO₂ + RH. Same bands as aeriq-comfort. Not medical."""
from __future__ import annotations

import argparse
import json


def band(ppm: float) -> str:
    if ppm < 800:
        return "good"
    if ppm < 1200:
        return "fair"
    if ppm < 2000:
        return "poor"
    return "bad"


def comfort(ppm: float, rh: float | None = None) -> dict:
    b = band(ppm)
    score = {"good": 1.0, "fair": 0.65, "poor": 0.35, "bad": 0.1}[b]
    rh_note = None
    if rh is not None:
        if 40 <= rh <= 60:
            rh_note = "rh ok"
        elif rh < 40:
            rh_note = "dry"
            score *= 0.9
        else:
            rh_note = "humid"
            score *= 0.9
    return {"ppm": ppm, "band": b, "rh": rh, "rh_note": rh_note, "score": round(score, 3)}


def main() -> None:
    p = argparse.ArgumentParser(description="AERIQ comfort score")
    p.add_argument("--ppm", type=float, required=True)
    p.add_argument("--rh", type=float)
    a = p.parse_args()
    print(json.dumps(comfort(a.ppm, a.rh), indent=2))


if __name__ == "__main__":
    main()
