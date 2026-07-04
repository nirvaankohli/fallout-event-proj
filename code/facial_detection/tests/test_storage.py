from datetime import datetime, UTC

import numpy as np

from app.face_service import FaceService
from app.schemas import ProfileRecord
from app.storage import ProfileStore


def test_store_persists_profiles(tmp_path) -> None:
    store = ProfileStore(tmp_path / "profiles.json")
    profile = ProfileRecord(
        id="abc123",
        name="Alice",
        embedding=[0.1, 0.2, 0.3],
        image_path="uploads/abc123.jpg",
        created_at=datetime.now(UTC),
    )

    store.create_profile(profile)
    loaded = store.list_profiles()

    assert len(loaded) == 1
    assert loaded[0].id == "abc123"
    assert loaded[0].name == "Alice"


def test_cosine_similarity_prefers_closest_match() -> None:
    left = np.asarray([1.0, 0.0, 0.0], dtype=np.float32)
    right = np.asarray([0.9, 0.1, 0.0], dtype=np.float32)
    far = np.asarray([0.0, 1.0, 0.0], dtype=np.float32)

    close_score = FaceService._cosine_similarity(left, right)
    far_score = FaceService._cosine_similarity(left, far)

    assert close_score > far_score
