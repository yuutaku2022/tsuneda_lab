from __future__ import annotations

import math
from pathlib import Path
from typing import Iterable, List, Sequence, Union

import cv2
import numpy as np

PathLike = Union[str, Path]


def generate_sylvester_hadamard_matrix(n: int) -> List[List[int]]:
    matrix = [[0] * n for _ in range(n)]
    matrix[0][0] = 1

    size = 2
    while size <= n:
        half = size // 2
        for i in range(half):
            for j in range(half):
                matrix[i + half][j] = matrix[i][j]
                matrix[i][j + half] = matrix[i][j]
                matrix[i + half][j + half] = -matrix[i][j]
        size *= 2

    return matrix


def generate_walsh_hadamard_matrix(n: int) -> List[List[int]]:
    sylvester = generate_sylvester_hadamard_matrix(n)
    walsh = [[0] * n for _ in range(n)]

    for i in range(n):
        sign_swap_count = 0
        for j in range(n - 1):
            if sylvester[i][j] != sylvester[i][j + 1]:
                sign_swap_count += 1
        for j in range(n):
            walsh[sign_swap_count][j] = sylvester[i][j]

    return walsh


def generate_debruijn_matrix(
    n: int,
    debruijn_seq_file_path: PathLike,
    target_line: int,
    line_start_index: int,
) -> List[List[int]]:
    seq_path = Path(debruijn_seq_file_path)
    if not seq_path.exists():
        raise FileNotFoundError(f"Debruijn sequence file not found: {seq_path}")

    debruijn_seq = ""
    for line_number, line in enumerate(seq_path.read_text(encoding="utf-8").splitlines(), start=1):
        if line_number == target_line:
            debruijn_seq = line.strip()
            break

    if not debruijn_seq:
        raise ValueError("Target line was not found in Debruijn sequence file.")

    seq_length = len(debruijn_seq)
    if seq_length == 256 and seq_length >= 8 and debruijn_seq.startswith("00000000"):
        window_size = 8
    elif seq_length == 64:
        window_size = 6
    elif seq_length == 256:
        window_size = 8
    elif seq_length == 248:
        debruijn_seq = "00000000" + debruijn_seq
        seq_length = 256
        window_size = 8
    else:
        raise ValueError(f"Unsupported Debruijn sequence length: {seq_length}")

    if n != seq_length:
        print(f"Warning: matrix size {n} and Debruijn length {seq_length} do not match.")

    windowed_values = []
    extended_seq = debruijn_seq + debruijn_seq
    for i in range(n):
        window = extended_seq[line_start_index + i: line_start_index + i + window_size]
        windowed_values.append(int(window, 2))

    walsh = generate_walsh_hadamard_matrix(n)
    debruijn_matrix = [[0] * n for _ in range(n)]
    for i in range(n):
        for j in range(n):
            debruijn_matrix[j][i] = walsh[j][windowed_values[i]]

    return debruijn_matrix


def load_gray_image_as_matrix(image_path: PathLike) -> np.ndarray:
    image = cv2.imread(str(image_path), cv2.IMREAD_GRAYSCALE)
    if image is None or image.size == 0:
        raise FileNotFoundError(f"Could not load grayscale image: {image_path}")
    return image


def orthogonal_transform(input_image: np.ndarray, orthogonal_matrix: Sequence[Sequence[int]]) -> np.ndarray:
    matrix = np.asarray(orthogonal_matrix, dtype=float)
    image = np.asarray(input_image, dtype=float)
    n = matrix.shape[0]
    return (matrix @ image @ matrix.T) / n


def inverse_orthogonal_transform(embedded_freq_matrix: np.ndarray, orthogonal_matrix: Sequence[Sequence[int]]) -> np.ndarray:
    matrix = np.asarray(orthogonal_matrix, dtype=float)
    freq = np.asarray(embedded_freq_matrix, dtype=float)
    n = matrix.shape[0]
    stego = (matrix.T @ freq @ matrix) / n
    stego = np.rint(stego).astype(np.int32)
    return np.clip(stego, 0, 255).astype(np.uint8)


def create_directory(path: PathLike) -> None:
    Path(path).mkdir(parents=True, exist_ok=True)


def calculate_psnr(img1: np.ndarray, img2: np.ndarray) -> float:
    diff = cv2.absdiff(img1, img2)
    diff = diff.astype(np.float32)
    mse = np.mean(diff * diff)
    if mse <= 1e-10:
        return 100.0
    return round(10.0 * math.log10((255.0 * 255.0) / mse), 3)
