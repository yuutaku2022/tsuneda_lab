from __future__ import annotations

import math
from pathlib import Path
from typing import Sequence, Tuple, Union

import cv2
import numpy as np

from matrix_transform_utils import (
    calculate_psnr,
    create_directory,
    inverse_orthogonal_transform,
    load_gray_image_as_matrix,
    orthogonal_transform,
)

PathLike = Union[str, Path]


def embed_by_threshold_adjust(
    cover_image_file_path: PathLike,
    secret_image_file_path: PathLike,
    orthogonal_matrix: Sequence[Sequence[int]],
    th: int,
    if_ratio: float,
    output_path: PathLike,
) -> Tuple[str, float]:
    cover_image = load_gray_image_as_matrix(cover_image_file_path)
    secret_image = load_gray_image_as_matrix(secret_image_file_path)

    freq_matrix = orthogonal_transform(cover_image, orthogonal_matrix)
    embedded_freq_matrix = freq_matrix.copy()

    secret_height, secret_width = secret_image.shape
    for i in range(secret_height):
        for j in range(secret_width):
            s = secret_image[i, j] * if_ratio
            if embedded_freq_matrix[i, j] > 0:
                if embedded_freq_matrix[i, j] <= th:
                    embedded_freq_matrix[i, j] = 0.0
                else:
                    embedded_freq_matrix[i, j] = embedded_freq_matrix[i, j] - math.fmod(embedded_freq_matrix[i, j], th)
                embedded_freq_matrix[i, j] += s
            else:
                if embedded_freq_matrix[i, j] >= -th:
                    embedded_freq_matrix[i, j] = 0.0
                else:
                    embedded_freq_matrix[i, j] = embedded_freq_matrix[i, j] - math.fmod(embedded_freq_matrix[i, j], th)
                embedded_freq_matrix[i, j] -= s

    stego_image = inverse_orthogonal_transform(embedded_freq_matrix, orthogonal_matrix)
    stego_psnr = calculate_psnr(cover_image, stego_image)

    create_directory(output_path)
    output_dir = Path(output_path)
    stego_file_path = output_dir / f"{secret_width}x{secret_height}_stego_TH={th}_IF={if_ratio:.3f}_{stego_psnr:.1f}dB.bmp"
    cv2.imwrite(str(stego_file_path), stego_image)
    return str(stego_file_path), float(stego_psnr)


def extract_by_threshold_adjust(
    stego_image_file_path: PathLike,
    secret_image_file_path: PathLike,
    orthogonal_matrix: Sequence[Sequence[int]],
    th: int,
    if_ratio: float,
    output_path: PathLike,
) -> Tuple[str, float]:
    stego_image = load_gray_image_as_matrix(stego_image_file_path)
    secret_image = load_gray_image_as_matrix(secret_image_file_path)

    stego_freq_matrix = orthogonal_transform(stego_image, orthogonal_matrix)
    secret_height, secret_width = secret_image.shape
    extracted_secret = np.zeros((secret_height, secret_width), dtype=np.uint8)

    for i in range(secret_height):
        for j in range(secret_width):
            stego_freq_element_abs = abs(stego_freq_matrix[i, j])
            if stego_freq_element_abs <= th:
                es = stego_freq_element_abs
            else:
                integer_part = int(stego_freq_element_abs)
                remainder = integer_part - math.fmod(integer_part, th)
                es = stego_freq_element_abs - remainder
            es /= if_ratio
            extracted_secret[i, j] = int(np.clip(es, 0, 255))

    extracted_psnr = calculate_psnr(secret_image, extracted_secret)
    create_directory(output_path)
    output_dir = Path(output_path)
    extracted_file_path = output_dir / f"{secret_width}x{secret_height}_secret_TH={th}_IF={if_ratio:.3f}_{extracted_psnr:.1f}dB.bmp"
    cv2.imwrite(str(extracted_file_path), extracted_secret)
    return str(extracted_file_path), float(extracted_psnr)
