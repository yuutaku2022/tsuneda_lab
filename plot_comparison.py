#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
THとIFの組み合わせによるPSNRの比較プロットスクリプト
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
from matplotlib import rcParams

# 日本語フォントの設定（Windowsの場合）
rcParams['font.family'] = 'DejaVu Sans'
rcParams['font.size'] = 10

def load_data(csv_path):
    """CSVファイルを読み込む"""
    if not os.path.exists(csv_path):
        print(f"エラー: {csv_path} が見つかりません。")
        return None
    df = pd.read_csv(csv_path)
    # ERROR行を除外
    df = df[df['Stego PSNR'] != 'ERROR']
    df['Stego PSNR'] = pd.to_numeric(df['Stego PSNR'], errors='coerce')
    df['Extracted PSNR'] = pd.to_numeric(df['Extracted PSNR'], errors='coerce')
    df = df.dropna()
    return df

def plot_psnr_heatmap(df, matrix_type, output_dir):
    """PSNRのヒートマップを作成"""
    # 指定された行列タイプのデータのみを抽出
    df_subset = df[df['Matrix Type'] == matrix_type]
    
    if df_subset.empty:
        print(f"警告: {matrix_type} のデータが見つかりません。")
        return
    
    # ピボットテーブルを作成（Stego PSNR）
    pivot_stego = df_subset.pivot_table(
        values='Stego PSNR', 
        index='IF', 
        columns='TH', 
        aggfunc='mean'
    )
    
    # ピボットテーブルを作成（Extracted PSNR）
    pivot_extracted = df_subset.pivot_table(
        values='Extracted PSNR', 
        index='IF', 
        columns='TH', 
        aggfunc='mean'
    )
    
    # 2つのサブプロットを作成
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))
    
    # Stego PSNRのヒートマップ
    im1 = ax1.imshow(pivot_stego.values, cmap='viridis', aspect='auto', origin='lower')
    ax1.set_title(f'{matrix_type} - Stego Image PSNR (dB)', fontsize=12, fontweight='bold')
    ax1.set_xlabel('TH (Threshold)', fontsize=11)
    ax1.set_ylabel('IF (Insert Factor)', fontsize=11)
    ax1.set_xticks(range(len(pivot_stego.columns)))
    ax1.set_xticklabels(pivot_stego.columns)
    ax1.set_yticks(range(len(pivot_stego.index)))
    ax1.set_yticklabels([f'{x:.2f}' for x in pivot_stego.index])
    plt.colorbar(im1, ax=ax1, label='PSNR (dB)')
    
    # Extracted PSNRのヒートマップ
    im2 = ax2.imshow(pivot_extracted.values, cmap='plasma', aspect='auto', origin='lower')
    ax2.set_title(f'{matrix_type} - Extracted Image PSNR (dB)', fontsize=12, fontweight='bold')
    ax2.set_xlabel('TH (Threshold)', fontsize=11)
    ax2.set_ylabel('IF (Insert Factor)', fontsize=11)
    ax2.set_xticks(range(len(pivot_extracted.columns)))
    ax2.set_xticklabels(pivot_extracted.columns)
    ax2.set_yticks(range(len(pivot_extracted.index)))
    ax2.set_yticklabels([f'{x:.2f}' for x in pivot_extracted.index])
    plt.colorbar(im2, ax=ax2, label='PSNR (dB)')
    
    plt.tight_layout()
    
    # 保存
    output_path = os.path.join(output_dir, f'{matrix_type}_heatmap.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"保存: {output_path}")
    plt.close()


def plot_psnr_line(df, matrix_type, output_dir):
    """IF固定でのTH vs PSNR、TH固定でのIF vs PSNRの線グラフ"""
    df_subset = df[df['Matrix Type'] == matrix_type]
    
    if df_subset.empty:
        return
    
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    
    # TH固定でIFを変えた場合（Stego PSNR）
    ax3, ax4 = axes
    TH_values = sorted(df_subset['TH'].unique())
    for th_val in TH_values[::2]:  # 間引いて表示
        data = df_subset[df_subset['TH'] == th_val]
        if not data.empty:
            ax3.plot(data['IF'], data['Stego PSNR'], marker='s', label=f'TH={th_val}', linewidth=2)
    ax3.set_xlabel('IF (Insert Factor)', fontsize=11)
    ax3.set_ylabel('Stego PSNR (dB)', fontsize=11)
    ax3.set_title(f'{matrix_type} - TH固定、IF変化', fontsize=11, fontweight='bold')
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    
    # TH固定でIFを変えた場合（Extracted PSNR）
    for th_val in TH_values[::2]:
        data = df_subset[df_subset['TH'] == th_val]
        if not data.empty:
            ax4.plot(data['IF'], data['Extracted PSNR'], marker='s', label=f'TH={th_val}', linewidth=2)
    ax4.set_xlabel('IF (Insert Factor)', fontsize=11)
    ax4.set_ylabel('Extracted PSNR (dB)', fontsize=11)
    ax4.set_title(f'{matrix_type} - TH固定、IF変化', fontsize=11, fontweight='bold')
    ax4.legend()
    ax4.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    # 保存
    output_path = os.path.join(output_dir, f'{matrix_type}_line.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"保存: {output_path}")
    plt.close()

def main():
    # スクリプトのディレクトリを取得
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # CSVファイルのパス（スクリプトの場所に基づいて相対パスを計算）
    csv_path = os.path.join(script_dir, "output", "th_if_comparison", "comparison_results.csv")
    
    # 出力ディレクトリ
    output_dir = os.path.join(script_dir, "output", "th_if_comparison", "plots")
    os.makedirs(output_dir, exist_ok=True)
    
    # データを読み込み
    print(f"データを読み込んでいます: {csv_path}")
    df = load_data(csv_path)
    
    if df is None or df.empty:
        print("データが見つかりませんでした。")
        return
    
    print(f"読み込んだデータ数: {len(df)}")
    print(f"行列タイプ: {df['Matrix Type'].unique()}")
    
    # 各行列タイプについてプロットを生成
    matrix_types = df['Matrix Type'].unique()
    
    for matrix_type in matrix_types:
        print(f"\n{matrix_type} のプロットを生成中...")
        plot_psnr_heatmap(df, matrix_type, output_dir)
        plot_psnr_line(df, matrix_type, output_dir)
    
    # 全体の比較プロット
    print("\n全体比較プロットを生成中...")
    fig, axes = plt.subplots(1, 2, figsize=(16, 6))
    
    # Stego PSNRの比較
    ax1 = axes[0]
    for matrix_type in matrix_types:
        data = df[df['Matrix Type'] == matrix_type]
        ax1.scatter(data['TH'] * data['IF'], data['Stego PSNR'], 
                   label=matrix_type, alpha=0.6, s=50)
    ax1.set_xlabel('TH × IF', fontsize=11)
    ax1.set_ylabel('Stego PSNR (dB)', fontsize=11)
    ax1.set_title('Stego Image PSNR Comparison', fontsize=12, fontweight='bold')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Extracted PSNRの比較
    ax2 = axes[1]
    for matrix_type in matrix_types:
        data = df[df['Matrix Type'] == matrix_type]
        ax2.scatter(data['TH'] * data['IF'], data['Extracted PSNR'], 
                   label=matrix_type, alpha=0.6, s=50)
    ax2.set_xlabel('TH × IF', fontsize=11)
    ax2.set_ylabel('Extracted PSNR (dB)', fontsize=11)
    ax2.set_title('Extracted Image PSNR Comparison', fontsize=12, fontweight='bold')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    output_path = os.path.join(output_dir, 'overall_comparison.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"保存: {output_path}")
    plt.close()
    
    print("\nすべてのプロットが生成されました！")

if __name__ == "__main__":
    main()
