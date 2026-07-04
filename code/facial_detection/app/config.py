from __future__ import annotations

import os
from pathlib import Path


BASE_DIR = Path(__file__).resolve().parent.parent
DATA_DIR = BASE_DIR / "data"
UPLOADS_DIR = BASE_DIR / "uploads"
MODELS_DIR = BASE_DIR / "models"
PROFILES_PATH = DATA_DIR / "profiles.json"

DETECTOR_MODEL_URL = (
    "https://raw.githubusercontent.com/opencv/opencv_zoo/main/models/"
    "face_detection_yunet/face_detection_yunet_2023mar.onnx"
)
RECOGNIZER_MODEL_URL = (
    "https://raw.githubusercontent.com/opencv/opencv_zoo/main/models/"
    "face_recognition_sface/face_recognition_sface_2021dec.onnx"
)

DETECTOR_MODEL_PATH = MODELS_DIR / "face_detection_yunet_2023mar.onnx"
RECOGNIZER_MODEL_PATH = MODELS_DIR / "face_recognition_sface_2021dec.onnx"

DEFAULT_MATCH_THRESHOLD = float(os.getenv("FACE_MATCH_THRESHOLD", "0.363"))
