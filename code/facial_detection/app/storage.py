from __future__ import annotations

import json
from pathlib import Path

from app.schemas import ProfileRecord


class ProfileStore:
    def __init__(self, path: Path) -> None:
        self.path = path
        self.path.parent.mkdir(parents=True, exist_ok=True)
        if not self.path.exists():
            self._write([])

    def list_profiles(self) -> list[ProfileRecord]:
        raw_items = json.loads(self.path.read_text(encoding="utf-8"))
        return [ProfileRecord.model_validate(item) for item in raw_items]

    def create_profile(self, profile: ProfileRecord) -> ProfileRecord:
        profiles = self.list_profiles()
        profiles.append(profile)
        self._write([item.model_dump(mode="json") for item in profiles])
        return profile

    def _write(self, payload: list[dict]) -> None:
        self.path.write_text(json.dumps(payload, indent=2), encoding="utf-8")
