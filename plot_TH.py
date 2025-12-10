import pandas as pd
import matplotlib.pyplot as plt
import os
import numpy as np

# --- 設定 ---
INPUT_CSV_FILE = 'comparison_results_all.csv'  # C++コード出力CSV
OUTPUT_DIR = 'analysis_graphs'
# -----------

def analyze_psnr_data(csv_path, output_dir):
    print(f"入力ファイル {csv_path} を読み込んでいます...")
    
    # データ読み込み
    try:
        df = pd.read_csv(csv_path)
    except FileNotFoundError:
        print(f"エラー: {csv_path} が見つかりません。")
        return
    
    # 'ERROR'をNaNに変換
    df['Stego PSNR'] = pd.to_numeric(df['Stego PSNR'], errors='coerce')
    df['Extracted PSNR'] = pd.to_numeric(df['Extracted PSNR'], errors='coerce')
    
    # NaNを含む行を削除
    df = df.dropna(subset=['Stego PSNR', 'Extracted PSNR'])
    if df.empty:
        print("有効なデータがありません。")
        return

    os.makedirs(output_dir, exist_ok=True)

    # PSNR差の計算
    df['PSNR_Diff'] = (df['Stego PSNR'] - df['Extracted PSNR']).abs()

    # 最適均衡点
    group_keys = ['Cover Image', 'Secret Image', 'Matrix Type', 'TargetLine']
    optimal_indices = df.groupby(group_keys)['PSNR_Diff'].idxmin()
    optimal_points_df = df.loc[optimal_indices].reset_index(drop=True)

    print("\n--- Optimal Equilibrium Points (最適均衡点) ---")
    for _, row in optimal_points_df.iterrows():
        print(f"[Cover: {row['Cover Image']}, Secret: {row['Secret Image']}, Matrix: {row['Matrix Type']}, TL: {row['TargetLine']}]")
        print(f"  Optimal TH: {row['TH']}, IF: {row['IF']:.4f}, Stego PSNR: {row['Stego PSNR']:.2f}, Extracted PSNR: {row['Extracted PSNR']:.2f} (Diff: {row['PSNR_Diff']:.2f})")

    # --- グラフA: PSNRトレードオフ（Cover×SecretごとにMatrixType×TargetLineを比較） ---
    print("\nグラフAを生成中...")
    unique_combinations = df[['Cover Image', 'Secret Image']].drop_duplicates().values
    matrix_types = df['Matrix Type'].unique()
    target_lines = df['TargetLine'].unique()
    
    for cover_img, secret_img in unique_combinations:
        fig, ax = plt.subplots(figsize=(12, 6))
        fig.suptitle(f'PSNR Trade-off Analysis\nCover: {cover_img} | Secret: {secret_img}', fontsize=16, y=1.03)

        for matrix_type in matrix_types:
            for tl in target_lines:
                plot_data = df[
                    (df['Cover Image'] == cover_img) &
                    (df['Secret Image'] == secret_img) &
                    (df['Matrix Type'] == matrix_type) &
                    (df['TargetLine'] == tl)
                ].sort_values('TH')
                
                if plot_data.empty:
                    continue

                label_stego = f'Stego ({matrix_type}, TL={tl})'
                label_extr = f'Extracted ({matrix_type}, TL={tl})'
                ax.plot(plot_data['TH'], plot_data['Stego PSNR'], marker='o', linestyle='-',
                        label=label_stego)
                ax.plot(plot_data['TH'], plot_data['Extracted PSNR'], marker='s', linestyle='--',
                        label=label_extr)
                
                # 最適TH縦線
                opt_row = optimal_points_df[
                    (optimal_points_df['Cover Image'] == cover_img) &
                    (optimal_points_df['Secret Image'] == secret_img) &
                    (optimal_points_df['Matrix Type'] == matrix_type) &
                    (optimal_points_df['TargetLine'] == tl)
                ]
                if not opt_row.empty:
                    optimal_th = opt_row['TH'].values[0]
                    ax.axvline(x=optimal_th, linestyle=':', color='green', linewidth=1.5)

        ax.set_xlabel('Threshold (TH)')
        ax.set_ylabel('PSNR (dB)')
        ax.grid(True, linestyle=':', alpha=0.7)
        ax.legend(fontsize=8, loc='best')

        safe_filename = f"GraphA_Cover_{cover_img}_Secret_{secret_img}_MatrixTL.png"
        save_path = os.path.join(output_dir, safe_filename)
        plt.tight_layout()
        plt.savefig(save_path, bbox_inches='tight')
        plt.close(fig)

    print("グラフA生成完了。")

    # --- グラフB: SecretごとのCover別 最適TH比較 ---
    print("\nグラフBを生成中...")
    unique_secret_images = optimal_points_df['Secret Image'].unique()
    
    for secret_img in unique_secret_images:
        fig, axes = plt.subplots(len(matrix_types), 1, figsize=(12, 6 * len(matrix_types)), sharey=True)
        if len(matrix_types) == 1:
            axes = [axes]

        fig.suptitle(f'Optimal TH Comparison (Secret Image: {secret_img})', fontsize=16, y=1.03)

        for i, matrix_type in enumerate(matrix_types):
            ax = axes[i]
            plot_data = optimal_points_df[
                (optimal_points_df['Secret Image'] == secret_img) &
                (optimal_points_df['Matrix Type'] == matrix_type)
            ].sort_values('Cover Image')
            
            if not plot_data.empty:
                bars = ax.bar(plot_data['Cover Image'], plot_data['TH'])
                ax.bar_label(bars, fmt='%d')
            ax.set_title(f'Matrix: {matrix_type}')
            ax.set_ylabel('Optimal TH')
            ax.set_xlabel('Cover Image')

        if not optimal_points_df.empty:
            max_th = optimal_points_df['TH'].max()
            plt.setp(axes, ylim=(0, max_th + 2))

        plt.tight_layout(rect=[0, 0, 1, 1])
        safe_filename = f"GraphB_Optimal_TH_Comparison_Secret_{secret_img}.png"
        save_path = os.path.join(output_dir, safe_filename)
        plt.savefig(save_path, bbox_inches='tight')
        plt.close(fig)

    print("グラフB生成完了。")
    print(f"\nすべての分析完了。結果は {output_dir} を確認してください。")

# --- 実行 ---
if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    csv_path = os.path.join(script_dir, "output", "cover_image_comparison", "comparison_results_all.csv")
    output_dir = os.path.join(script_dir, "output", "cover_image_comparison", "plots")
    os.makedirs(output_dir, exist_ok=True)
    analyze_psnr_data(csv_path, output_dir)
