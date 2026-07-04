from __future__ import annotations

from datetime import datetime

from pydantic import BaseModel, Field


class ProfileRecord(BaseModel):
    id: str
    name: str
    embedding: list[float]
    image_path: str
    created_at: datetime


class ProfileSummary(BaseModel):
    id: str
    name: str
    image_path: str
    created_at: datetime


class CreateProfileResponse(BaseModel):
    id: str
    name: str
    created_at: datetime
    image_path: str


class DetectionResponse(BaseModel):
    matched: bool
    profile_id: str | None = None
    name: str | None = None
    score: float | None = None
    threshold: float = Field(..., description="Minimum similarity required")
    detected_faces: int
