from __future__ import annotations

import csv
import random
import argparse
from pathlib import Path
from typing import List, Sequence, Tuple

from embed_by_threshold_adjust import embed_by_threshold_adjust, extract_by_threshold_adjust
from matrix_transform_utils import create_directory, generate_debruijn_matrix

REPO_ROOT = Path(__file__).resolve().parents[1]
INPUT_DIR = REPO_ROOT / "input"
OUTPUT_DIR = REPO_ROOT / "output" / "cover_image_comparison_python"


def apply_random_sign_flip(matrix: List[List[int]], flip_count: int) -> None:
    n = len(matrix)
    if flip_count > n * n:
        flip_count = n * n
    coords = [(i, j) for i in range(n) for j in range(n)]
    random.shuffle(coords)
    for idx in range(flip_count):
        i, j = coords[idx]
        matrix[i][j] = -matrix[i][j]


def apply_random_row_shuffle(matrix: List[List[int]], shuffle_count: int) -> None:
    n = len(matrix)
    if shuffle_count > n:
        shuffle_count = n
    row_indices = list(range(n))
    random.shuffle(row_indices)
    for k in range(shuffle_count - 1):
        matrix[row_indices[k]], matrix[row_indices[k + 1]] = matrix[row_indices[k + 1]], matrix[row_indices[k]]


def apply_random_col_shuffle(matrix: List[List[int]], shuffle_count: int) -> None:
    n = len(matrix)
    if shuffle_count > n:
        shuffle_count = n
    col_indices = list(range(n))
    random.shuffle(col_indices)
    for k in range(shuffle_count - 1):
        for i in range(n):
            matrix[i][col_indices[k]], matrix[i][col_indices[k + 1]] = matrix[i][col_indices[k + 1]], matrix[i][col_indices[k]]


def run_experiment(quick: bool = False) -> None:
    n = 256
    cover_images = [
        (INPUT_DIR / "img" / "cover_image" / "Lenna_grayscale_256.bmp", "Lenna_256", "Lenna_256"),
        (INPUT_DIR / "img" / "cover_image" / "barbara256.bmp", "barbara256", "barbara256"),
    ]
    secret_images = [
        (INPUT_DIR / "img" / "secret_image" / "secret_grayscale_64x64.bmp", "secret_64x64", "secret_64x64"),
        (INPUT_DIR / "img" / "secret_image" / "secret_grayscale_128x128.bmp", "secret_128x128", "secret_128x128"),
    ]

    if quick:
        cover_images = cover_images[:1]
        secret_images = secret_images[:1]

    match n:
        case 64:
            debruijn_seq_file = INPUT_DIR / "debruijn" / "deb64.dat"
        case 256:
            debruijn_seq_file = INPUT_DIR / "debruijn" / "deb256.dat"
        case _:
            raise ValueError(f"Unsupported size: {n}")

    line_start_index = 0
    target_line_value = 5
    th_min, th_max, th_step = 5, 10, 1
    if_ratio_of_theory = 0.98
    flip_counts = [5, 10]

    create_directory(OUTPUT_DIR)
    csv_path = OUTPUT_DIR / "comparison_results_all.csv"
    with csv_path.open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(["Cover Image", "Secret Image", "Matrix Type", "FlipCount", "TH", "IF", "Stego PSNR", "Extracted PSNR"])

        debruijn_matrix = generate_debruijn_matrix(n, debruijn_seq_file, target_line_value, line_start_index)
        matrix_types = [(f"Debruijn_t{target_line_value}", debruijn_matrix)]

        for cover_path, cover_name, cover_disp in cover_images:
            for secret_path, secret_name, secret_disp in secret_images:
                image_output_dir = OUTPUT_DIR / cover_name / secret_name
                create_directory(image_output_dir)
                image_csv_path = image_output_dir / "comparison_results.csv"
                with image_csv_path.open("w", newline="", encoding="utf-8") as image_csv_file:
                    image_writer = csv.writer(image_csv_file)
                    image_writer.writerow(["Matrix Type", "FlipCount", "TH", "IF", "Stego PSNR", "Extracted PSNR"])
                    for matrix_name_base, orth_matrix in matrix_types:
                        matrix_output_dir_base = image_output_dir / matrix_name_base
                        create_directory(matrix_output_dir_base)
                        for th in range(th_min, th_max + 1, th_step):
                            if_value = (th / 255.0) * if_ratio_of_theory
                            stego_path, stego_psnr = embed_by_threshold_adjust(
                                cover_path,
                                secret_path,
                                orth_matrix,
                                th,
                                if_value,
                                str(matrix_output_dir_base),
                            )
                            for flip_count in flip_counts:
                                modified_matrix = [row[:] for row in orth_matrix]
                                apply_random_col_shuffle(modified_matrix, flip_count)
                                matrix_name_flipped = f"{matrix_name_base}_flip_{flip_count}"
                                matrix_output_dir = image_output_dir / matrix_name_flipped
                                create_directory(matrix_output_dir)
                                extracted_path, extracted_psnr = extract_by_threshold_adjust(
                                    stego_path,
                                    secret_path,
                                    modified_matrix,
                                    th,
                                    if_value,
                                    str(matrix_output_dir),
                                )
                                writer.writerow([cover_disp, secret_disp, matrix_name_flipped, flip_count, th, f"{if_value:.4f}", f"{stego_psnr:.2f}", f"{extracted_psnr:.2f}"])
                                image_writer.writerow([matrix_name_flipped, flip_count, th, f"{if_value:.4f}", f"{stego_psnr:.2f}", f"{extracted_psnr:.2f}"])

    print(f"Completed. Results saved to {csv_path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Ported Python version of compare_th_if.cpp")
    parser.add_argument("--quick", action="store_true", help="Run a smaller smoke-test subset")
    args = parser.parse_args()
    run_experiment(quick=args.quick)
