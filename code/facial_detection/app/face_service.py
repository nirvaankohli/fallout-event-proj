from __future__ import annotations

import urllib.request
import uuid
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path

import cv2
import numpy as np
from fastapi import HTTPException, UploadFile, status

from app.config import (
    DEFAULT_MATCH_THRESHOLD,
    DETECTOR_MODEL_PATH,
    DETECTOR_MODEL_URL,
    RECOGNIZER_MODEL_PATH,
    RECOGNIZER_MODEL_URL,
    UPLOADS_DIR,
)
from app.schemas import ProfileRecord
from app.storage import ProfileStore


@dataclass
class MatchResult:
    matched: bool
    profile: ProfileRecord | None
    score: float | None
    detected_faces: int


class FaceService:
    def __init__(self, store: ProfileStore) -> None:
        self.store = store
        self._detector: cv2.FaceDetectorYN | None = None
        self._recognizer: cv2.FaceRecognizerSF | None = None

    def create_profile(self, name: str, image_bytes: bytes, filename: str | None) -> ProfileRecord:
        image = self._decode_image(image_bytes)
        face, _ = self._detect_primary_face(image)
        embedding = self._extract_embedding(image, face)

        profile_id = str(uuid.uuid4())
        upload_path = self._save_upload(profile_id, filename, image_bytes)
        profile = ProfileRecord(
            id=profile_id,
            name=name,
            embedding=embedding.tolist(),
            image_path=str(upload_path.relative_to(UPLOADS_DIR.parent)),
            created_at=datetime.now(UTC),
        )
        return self.store.create_profile(profile)

    def detect(self, image_bytes: bytes) -> MatchResult:
        profiles = self.store.list_profiles()
        if not profiles:
            raise HTTPException(
                status_code=status.HTTP_400_BAD_REQUEST,
                detail="No profiles exist yet. Create a profile first.",
            )

        image = self._decode_image(image_bytes)
        face, face_count = self._detect_primary_face(image)
        query_embedding = self._extract_embedding(image, face)

        best_profile: ProfileRecord | None = None
        best_score: float | None = None
        for profile in profiles:
            known_embedding = np.asarray(profile.embedding, dtype=np.float32)
            score = self._cosine_similarity(query_embedding, known_embedding)
            if best_score is None or score > best_score:
                best_score = score
                best_profile = profile

        matched = best_score is not None and best_score >= DEFAULT_MATCH_THRESHOLD
        return MatchResult(
            matched=matched,
            profile=best_profile if matched else None,
            score=best_score,
            detected_faces=face_count,
        )

    def _decode_image(self, image_bytes: bytes) -> np.ndarray:
        array = np.frombuffer(image_bytes, dtype=np.uint8)
        image = cv2.imdecode(array, cv2.IMREAD_COLOR)
        if image is None:
            raise HTTPException(
                status_code=status.HTTP_400_BAD_REQUEST,
                detail="Uploaded file is not a valid image.",
            )
        return image

    def _detect_primary_face(self, image: np.ndarray) -> tuple[np.ndarray, int]:
        detector = self._get_detector()
        height, width = image.shape[:2]
        detector.setInputSize((width, height))
        _, faces = detector.detect(image)
        if faces is None or len(faces) == 0:
            raise HTTPException(
                status_code=status.HTTP_400_BAD_REQUEST,
                detail="No face detected in the image.",
            )

        best_face = max(faces, key=lambda face: float(face[2] * face[3]))
        return best_face, len(faces)

    def _extract_embedding(self, image: np.ndarray, face: np.ndarray) -> np.ndarray:
        recognizer = self._get_recognizer()
        aligned_face = recognizer.alignCrop(image, face)
        embedding = recognizer.feature(aligned_face)
        return embedding.flatten().astype(np.float32)

    def _save_upload(self, profile_id: str, filename: str | None, image_bytes: bytes) -> Path:
        UPLOADS_DIR.mkdir(parents=True, exist_ok=True)
        suffix = Path(filename or "upload.jpg").suffix or ".jpg"
        destination = UPLOADS_DIR / f"{profile_id}{suffix.lower()}"
        destination.write_bytes(image_bytes)
        return destination

    def _get_detector(self) -> cv2.FaceDetectorYN:
        if self._detector is None:
            self._ensure_models()
            self._detector = cv2.FaceDetectorYN.create(str(DETECTOR_MODEL_PATH), "", (320, 320))
        return self._detector

    def _get_recognizer(self) -> cv2.FaceRecognizerSF:
        if self._recognizer is None:
            self._ensure_models()
            self._recognizer = cv2.FaceRecognizerSF.create(str(RECOGNIZER_MODEL_PATH), "")
        return self._recognizer

    def _ensure_models(self) -> None:
        DETECTOR_MODEL_PATH.parent.mkdir(parents=True, exist_ok=True)
        if not DETECTOR_MODEL_PATH.exists():
            urllib.request.urlretrieve(DETECTOR_MODEL_URL, DETECTOR_MODEL_PATH)
        if not RECOGNIZER_MODEL_PATH.exists():
            urllib.request.urlretrieve(RECOGNIZER_MODEL_URL, RECOGNIZER_MODEL_PATH)

    @staticmethod
    def _cosine_similarity(left: np.ndarray, right: np.ndarray) -> float:
        left_norm = np.linalg.norm(left)
        right_norm = np.linalg.norm(right)
        if left_norm == 0.0 or right_norm == 0.0:
            return 0.0
        return float(np.dot(left, right) / (left_norm * right_norm))


async def read_upload_file(image: UploadFile) -> bytes:
    try:
        return await image.read()
    finally:
        await image.close()
