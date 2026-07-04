from __future__ import annotations

from fastapi import FastAPI, File, Form, UploadFile

from app.config import DEFAULT_MATCH_THRESHOLD, PROFILES_PATH
from app.face_service import FaceService, read_upload_file
from app.schemas import CreateProfileResponse, DetectionResponse, ProfileSummary
from app.storage import ProfileStore


app = FastAPI(title="Facial Detection API", version="0.1.0")

store = ProfileStore(PROFILES_PATH)
face_service = FaceService(store)


@app.get("/")
def root() -> dict[str, str]:
    return {
        "message": "Facial detection API is running.",
        "docs": "/docs",
    }


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}


@app.get("/profiles", response_model=list[ProfileSummary])
def list_profiles() -> list[ProfileSummary]:
    profiles = store.list_profiles()
    return [
        ProfileSummary(
            id=profile.id,
            name=profile.name,
            image_path=profile.image_path,
            created_at=profile.created_at,
        )
        for profile in profiles
    ]


@app.post("/profiles", response_model=CreateProfileResponse)
async def create_profile(
    name: str = Form(...),
    image: UploadFile = File(...),
) -> CreateProfileResponse:
    payload = await read_upload_file(image)
    profile = face_service.create_profile(name=name, image_bytes=payload, filename=image.filename)
    return CreateProfileResponse(
        id=profile.id,
        name=profile.name,
        created_at=profile.created_at,
        image_path=profile.image_path,
    )


@app.post("/profiles/create", response_model=CreateProfileResponse)
async def create_profile_alias(
    name: str = Form(...),
    image: UploadFile = File(...),
) -> CreateProfileResponse:
    return await create_profile(name=name, image=image)


@app.post("/detect", response_model=DetectionResponse)
async def detect_face(image: UploadFile = File(...)) -> DetectionResponse:
    payload = await read_upload_file(image)
    result = face_service.detect(image_bytes=payload)
    return DetectionResponse(
        matched=result.matched,
        profile_id=result.profile.id if result.profile else None,
        name=result.profile.name if result.profile else None,
        score=result.score,
        threshold=DEFAULT_MATCH_THRESHOLD,
        detected_faces=result.detected_faces,
    )
