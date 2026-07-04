# Facial Detection API

FastAPI service for:

- creating face profiles from an uploaded image
- storing `id`, `name`, and a face embedding
- detecting who appears in a new uploaded image

## Endpoints

- `GET /` basic service info
- `GET /health` health check
- `GET /profiles` list stored profiles
- `POST /profiles` create a profile
- `POST /profiles/create` alias for profile creation
- `POST /detect` detect the closest known profile from an image

## Quick Start

```bash
cd /Users/nirvaank/Code/Hardware/fallout/event/project/fallout-event-proj/code/facial_detection
make install
make run
```

The first request that needs face recognition will automatically download:

- YuNet face detector
- SFace face recognizer

## Create A Profile

```bash
curl -X POST http://127.0.0.1:8000/profiles \
  -F "name=Alice" \
  -F "image=@/absolute/path/to/alice.jpg"
```

## Detect A Face

```bash
curl -X POST http://127.0.0.1:8000/detect \
  -F "image=@/absolute/path/to/unknown.jpg"
```

## Notes

- This service uses the largest detected face in the image.
- Profile data is stored in `data/profiles.json`.
- Uploaded reference images are stored in `uploads/`.
- The default cosine threshold is `0.363`. Override it with `FACE_MATCH_THRESHOLD`.
