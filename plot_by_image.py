#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
各画像フォルダごとにTHとIFの組み合わせによるPSNRのプロットを生成するスクリプト
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
from pathlib import Path
from matplotlib import rcParams
import seaborn as sns

# 日本語フォントの設定
rcParams['font.family'] = 'DejaVu Sans'
rcParams['font.size'] = 10
sns.set_style("whitegrid")

def load_data(csv_path):
    """CSVファイルを読み込む"""
    if not os.path.exists(csv_path):
        print(f"エラー: {csv_path} が見つかりません。")
        return None
    try:
        df = pd.read_csv(csv_path)
        # ERROR行を除外
        df = df[df['Stego PSNR'] != 'ERROR']
        df = df[df['Extracted PSNR'] != 'ERROR']
        df['Stego PSNR'] = pd.to_numeric(df['Stego PSNR'], errors='coerce')
        df['Extracted PSNR'] = pd.to_numeric(df['Extracted PSNR'], errors='coerce')
        df = df.dropna()
        return df
    except Exception as e:
        print(f"エラー: {csv_path} の読み込みに失敗しました: {e}")
        return None

def plot_psnr_heatmap(df, matrix_type, output_dir, image_name):
    """PSNRのヒートマップを作成"""
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
    fig.suptitle(f'{image_name} - {matrix_type}', fontsize=14, fontweight='bold')
    
    # Stego PSNRのヒートマップ
    im1 = ax1.imshow(pivot_stego.values, cmap='viridis', aspect='auto', origin='lower')
    ax1.set_title(f'Stego Image PSNR (dB)', fontsize=12, fontweight='bold')
    ax1.set_xlabel('TH (Threshold)', fontsize=11)
    ax1.set_ylabel('IF (Insert Factor)', fontsize=11)
    ax1.set_xticks(range(len(pivot_stego.columns)))
    ax1.set_xticklabels(pivot_stego.columns, rotation=45)
    ax1.set_yticks(range(0, len(pivot_stego.index), max(1, len(pivot_stego.index)//10)))
    ax1.set_yticklabels([f'{pivot_stego.index[i]:.2f}' for i in range(0, len(pivot_stego.index), max(1, len(pivot_stego.index)//10))])
    plt.colorbar(im1, ax=ax1, label='PSNR (dB)')
    
    # Extracted PSNRのヒートマップ
    im2 = ax2.imshow(pivot_extracted.values, cmap='plasma', aspect='auto', origin='lower')
    ax2.set_title(f'Extracted Image PSNR (dB)', fontsize=12, fontweight='bold')
    ax2.set_xlabel('TH (Threshold)', fontsize=11)
    ax2.set_ylabel('IF (Insert Factor)', fontsize=11)
    ax2.set_xticks(range(len(pivot_extracted.columns)))
    ax2.set_xticklabels(pivot_extracted.columns, rotation=45)
    ax2.set_yticks(range(0, len(pivot_extracted.index), max(1, len(pivot_extracted.index)//10)))
    ax2.set_yticklabels([f'{pivot_extracted.index[i]:.2f}' for i in range(0, len(pivot_extracted.index), max(1, len(pivot_extracted.index)//10))])
    plt.colorbar(im2, ax=ax2, label='PSNR (dB)')
    
    plt.tight_layout()
    
    # 保存
    output_path = os.path.join(output_dir, f'{matrix_type}_heatmap.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"保存: {output_path}")
    plt.close()

def plot_psnr_line(df, matrix_type, output_dir, image_name):
    """IF固定でのTH vs PSNR、TH固定でのIF vs PSNRの線グラフ"""
    df_subset = df[df['Matrix Type'] == matrix_type]
    
    if df_subset.empty:
        return
    
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle(f'{image_name} - {matrix_type}', fontsize=14, fontweight='bold')
    
    TH_values = sorted(df_subset['TH'].unique())
    IF_values = sorted(df_subset['IF'].unique())
    
    # Stego PSNR: TH固定でIFを変えた場合
    ax1 = axes[0, 0]
    for th_val in TH_values[::max(1, len(TH_values)//5)]:  # 最大5本の線を表示
        data = df_subset[df_subset['TH'] == th_val]
        if not data.empty:
            ax1.plot(data['IF'], data['Stego PSNR'], marker='o', label=f'TH={th_val}', linewidth=2, markersize=4)
    ax1.set_xlabel('IF (Insert Factor)', fontsize=11)
    ax1.set_ylabel('Stego PSNR (dB)', fontsize=11)
    ax1.set_title('Stego PSNR: TH固定、IF変化', fontsize=11, fontweight='bold')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Extracted PSNR: TH固定でIFを変えた場合
    ax2 = axes[0, 1]
    for th_val in TH_values[::max(1, len(TH_values)//5)]:
        data = df_subset[df_subset['TH'] == th_val]
        if not data.empty:
            ax2.plot(data['IF'], data['Extracted PSNR'], marker='o', label=f'TH={th_val}', linewidth=2, markersize=4)
    ax2.set_xlabel('IF (Insert Factor)', fontsize=11)
    ax2.set_ylabel('Extracted PSNR (dB)', fontsize=11)
    ax2.set_title('Extracted PSNR: TH固定、IF変化', fontsize=11, fontweight='bold')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    # Stego PSNR: IF固定でTHを変えた場合
    ax3 = axes[1, 0]
    for if_val in IF_values[::max(1, len(IF_values)//5)]:  # 最大5本の線を表示
        data = df_subset[df_subset['IF'] == if_val]
        if not data.empty:
            ax3.plot(data['TH'], data['Stego PSNR'], marker='s', label=f'IF={if_val:.2f}', linewidth=2, markersize=4)
    ax3.set_xlabel('TH (Threshold)', fontsize=11)
    ax3.set_ylabel('Stego PSNR (dB)', fontsize=11)
    ax3.set_title('Stego PSNR: IF固定、TH変化', fontsize=11, fontweight='bold')
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    
    # Extracted PSNR: IF固定でTHを変えた場合
    ax4 = axes[1, 1]
    for if_val in IF_values[::max(1, len(IF_values)//5)]:
        data = df_subset[df_subset['IF'] == if_val]
        if not data.empty:
            ax4.plot(data['TH'], data['Extracted PSNR'], marker='s', label=f'IF={if_val:.2f}', linewidth=2, markersize=4)
    ax4.set_xlabel('TH (Threshold)', fontsize=11)
    ax4.set_ylabel('Extracted PSNR (dB)', fontsize=11)
    ax4.set_title('Extracted PSNR: IF固定、TH変化', fontsize=11, fontweight='bold')
    ax4.legend()
    ax4.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    # 保存
    output_path = os.path.join(output_dir, f'{matrix_type}_line.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"保存: {output_path}")
    plt.close()

def plot_matrix_comparison(df, output_dir, image_name):
    """3つの行列タイプを比較するプロット"""
    matrix_types = df['Matrix Type'].unique()
    
    if len(matrix_types) == 0:
        return
    
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    fig.suptitle(f'{image_name} - Matrix Comparison', fontsize=14, fontweight='bold')
    
    # Stego PSNR: TH × IF での散布図
    ax1 = axes[0, 0]
    for matrix_type in matrix_types:
        data = df[df['Matrix Type'] == matrix_type]
        ax1.scatter(data['TH'] * data['IF'], data['Stego PSNR'], 
                   label=matrix_type, alpha=0.6, s=30)
    ax1.set_xlabel('TH × IF', fontsize=11)
    ax1.set_ylabel('Stego PSNR (dB)', fontsize=11)
    ax1.set_title('Stego PSNR Comparison (TH × IF)', fontsize=12, fontweight='bold')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Extracted PSNR: TH × IF での散布図
    ax2 = axes[0, 1]
    for matrix_type in matrix_types:
        data = df[df['Matrix Type'] == matrix_type]
        ax2.scatter(data['TH'] * data['IF'], data['Extracted PSNR'], 
                   label=matrix_type, alpha=0.6, s=30)
    ax2.set_xlabel('TH × IF', fontsize=11)
    ax2.set_ylabel('Extracted PSNR (dB)', fontsize=11)
    ax2.set_title('Extracted PSNR Comparison (TH × IF)', fontsize=12, fontweight='bold')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    # Stego PSNR: 各行列タイプの平均値の比較
    ax3 = axes[1, 0]
    means_stego = df.groupby('Matrix Type')['Stego PSNR'].mean().sort_values(ascending=False)
    bars1 = ax3.bar(range(len(means_stego)), means_stego.values, alpha=0.7)
    ax3.set_xticks(range(len(means_stego)))
    ax3.set_xticklabels(means_stego.index, rotation=45, ha='right')
    ax3.set_ylabel('Average Stego PSNR (dB)', fontsize=11)
    ax3.set_title('Average Stego PSNR by Matrix Type', fontsize=12, fontweight='bold')
    ax3.grid(True, alpha=0.3, axis='y')
    # 値をバーの上に表示
    for i, (idx, val) in enumerate(means_stego.items()):
        ax3.text(i, val, f'{val:.2f}', ha='center', va='bottom', fontsize=9)
    
    # Extracted PSNR: 各行列タイプの平均値の比較
    ax4 = axes[1, 1]
    means_extracted = df.groupby('Matrix Type')['Extracted PSNR'].mean().sort_values(ascending=False)
    bars2 = ax4.bar(range(len(means_extracted)), means_extracted.values, alpha=0.7)
    ax4.set_xticks(range(len(means_extracted)))
    ax4.set_xticklabels(means_extracted.index, rotation=45, ha='right')
    ax4.set_ylabel('Average Extracted PSNR (dB)', fontsize=11)
    ax4.set_title('Average Extracted PSNR by Matrix Type', fontsize=12, fontweight='bold')
    ax4.grid(True, alpha=0.3, axis='y')
    # 値をバーの上に表示
    for i, (idx, val) in enumerate(means_extracted.items()):
        ax4.text(i, val, f'{val:.2f}', ha='center', va='bottom', fontsize=9)
    
    plt.tight_layout()
    
    # 保存
    output_path = os.path.join(output_dir, 'matrix_comparison.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"保存: {output_path}")
    plt.close()

def process_image_folder(image_folder_path, base_output_dir):
    """各画像フォルダを処理してプロットを生成"""
    image_name = os.path.basename(image_folder_path)
    csv_path = os.path.join(image_folder_path, "comparison_results.csv")
    
    if not os.path.exists(csv_path):
        print(f"スキップ: {csv_path} が見つかりません。")
        return
    
    print(f"\n{'='*60}")
    print(f"処理中: {image_name}")
    print(f"{'='*60}")
    
    # データを読み込み
    df = load_data(csv_path)
    
    if df is None or df.empty:
        print(f"警告: {image_name} のデータが空です。")
        return
    
    # 出力ディレクトリを作成
    plots_dir = os.path.join(image_folder_path, "plots")
    os.makedirs(plots_dir, exist_ok=True)
    
    print(f"読み込んだデータ数: {len(df)}")
    print(f"行列タイプ: {df['Matrix Type'].unique()}")
    
    # 各行列タイプについてプロットを生成
    matrix_types = df['Matrix Type'].unique()
    
    for matrix_type in matrix_types:
        print(f"\n  {matrix_type} のプロットを生成中...")
        plot_psnr_heatmap(df, matrix_type, plots_dir, image_name)
        plot_psnr_line(df, matrix_type, plots_dir, image_name)
    
    # 全体の比較プロット
    print(f"\n  全体比較プロットを生成中...")
    plot_matrix_comparison(df, plots_dir, image_name)
    
    print(f"\n{image_name} の処理が完了しました。")
    print(f"プロットは {plots_dir} に保存されました。")

def main():
    """メイン関数"""
    # スクリプトのディレクトリを取得
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # 出力ディレクトリのパス
    base_output_dir = os.path.join(script_dir, "output", "th_if_comparison")
    
    if not os.path.exists(base_output_dir):
        print(f"エラー: {base_output_dir} が見つかりません。")
        return
    
    print(f"出力ディレクトリ: {base_output_dir}")
    
    # 各画像フォルダを検索
    image_folders = []
    for item in os.listdir(base_output_dir):
        item_path = os.path.join(base_output_dir, item)
        if os.path.isdir(item_path) and item not in ['plots', 'Sylvester_Hadamard', 'Walsh_Hadamard', 'Debruijn']:
            # comparison_results.csvが存在するか確認
            csv_path = os.path.join(item_path, "comparison_results.csv")
            if os.path.exists(csv_path):
                image_folders.append(item_path)
    
    if not image_folders:
        print("エラー: 画像フォルダが見つかりません。")
        return
    
    print(f"\n見つかった画像フォルダ数: {len(image_folders)}")
    for folder in image_folders:
        print(f"  - {os.path.basename(folder)}")
    
    # 各画像フォルダを処理
    for image_folder in sorted(image_folders):
        process_image_folder(image_folder, base_output_dir)
    
    print(f"\n{'='*60}")
    print("すべての処理が完了しました！")
    print(f"{'='*60}")

if __name__ == "__main__":
    main()


