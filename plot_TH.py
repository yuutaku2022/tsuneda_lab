import pandas as pd
import matplotlib.pyplot as plt
import os
import numpy as np

# --- 設定 ---
INPUT_CSV_FILE = 'comparison_results_all.csv'
OUTPUT_DIR = 'analysis_graphs'
# -----------

def analyze_psnr_data(csv_path, output_dir):
    """
    実験結果のCSVを読み込み、PSNRの最適均衡点を分析・可視化する
    """
    print(f"入力ファイル {csv_path} を読み込んでいます...")

    # 1. データ読み込み & 2. 前処理 (エラー行の除外)
    try:
        df = pd.read_csv(csv_path)
    except FileNotFoundError:
        print(f"エラー: {csv_path} が見つかりません。")
        print("C++の実験コードを実行してCSVファイルを生成してください。")
        return

    # 'ERROR'などの非数値データをNaNに変換
    df['Stego PSNR'] = pd.to_numeric(df['Stego PSNR'], errors='coerce')
    df['Extracted PSNR'] = pd.to_numeric(df['Extracted PSNR'], errors='coerce')

    # エラー（NaN）になった行を削除
    original_rows = len(df)
    df = df.dropna(subset=['Stego PSNR', 'Extracted PSNR'])
    print(f"読み込み完了。{original_rows}行中、{len(df)}行の有効なデータを処理します。")

    if df.empty:
        print("エラー: 有効なデータが見つかりませんでした。CSVファイルの中身を確認してください。")
        return

    # グラフ保存用ディレクトリを作成
    os.makedirs(output_dir, exist_ok=True)
    print(f"グラフは {output_dir} フォルダに保存されます。")

    # 3. 最適均衡点の計算
    # PSNRの差の絶対値を計算
    df['PSNR_Diff'] = (df['Stego PSNR'] - df['Extracted PSNR']).abs()

    # 差が最小となる行のインデックスを取得
    group_keys = ['Cover Image', 'Secret Image', 'Matrix Type']
    optimal_indices = df.groupby(group_keys)['PSNR_Diff'].idxmin()
    optimal_points_df = df.loc[optimal_indices].reset_index(drop=True)

    # 4. コンソールへの出力
    print("\n--- Optimal Equilibrium Points (最適均衡点) ---")
    for _, row in optimal_points_df.iterrows():
        print(f"  [Cover: {row['Cover Image']}, Secret: {row['Secret Image']}, Matrix: {row['Matrix Type']}]")
        print(f"    Optimal TH: {row['TH']}, IF: {row['IF']:.4f}")
        print(f"    Stego PSNR: {row['Stego PSNR']:.2f}, Extracted PSNR: {row['Extracted PSNR']:.2f} (Diff: {row['PSNR_Diff']:.2f})")
        print("    -----------------------------------")


    # 5. グラフ生成A：PSNRトレードオフ可視化
    print("\nグラフA (PSNRトレードオフ) を生成中...")
    
    # (Cover, Secret) のユニークな組み合わせでループ
    unique_combinations = df[['Cover Image', 'Secret Image']].drop_duplicates().values
    matrix_types = df['Matrix Type'].unique()
    
    for cover_img, secret_img in unique_combinations:
        fig, axes = plt.subplots(len(matrix_types), 1, 
                                 figsize=(10, 5 * len(matrix_types)), 
                                 sharex=True)
        
        if len(matrix_types) == 1: # サブプロットが1つの場合 axes は配列にならない
            axes = [axes]

        fig.suptitle(f'PSNR Trade-off Analysis\nCover: {cover_img} | Secret: {secret_img}', 
                     fontsize=16, y=1.03)

        for i, matrix_type in enumerate(matrix_types):
            ax = axes[i]
            
            # 元データのフィルタリング
            plot_data = df[
                (df['Cover Image'] == cover_img) &
                (df['Secret Image'] == secret_img) &
                (df['Matrix Type'] == matrix_type)
            ].sort_values('TH')
            
            # 最適THの取得
            optimal_row = optimal_points_df[
                (optimal_points_df['Cover Image'] == cover_img) &
                (optimal_points_df['Secret Image'] == secret_img) &
                (optimal_points_df['Matrix Type'] == matrix_type)
            ]
            
            if not plot_data.empty:
                # 2つのPSNRをプロット
                ax.plot(plot_data['TH'], plot_data['Stego PSNR'], 
                        marker='o', linestyle='-', label='Stego PSNR (非可視性)')
                ax.plot(plot_data['TH'], plot_data['Extracted PSNR'], 
                        marker='s', linestyle='--', label='Extracted PSNR (復元性)')
                
                # 最適TH（均衡点）をハイライト
                if not optimal_row.empty:
                    optimal_th = optimal_row['TH'].values[0]
                    ax.axvline(x=optimal_th, color='green', linestyle=':', 
                               linewidth=2, label=f'Optimal TH = {optimal_th}')
                
                ax.set_title(f'Matrix: {matrix_type}')
                ax.set_ylabel('PSNR (dB)')
                ax.legend()
                ax.grid(True, linestyle=':', alpha=0.7)

        axes[-1].set_xlabel('Threshold (TH)')
        plt.tight_layout(rect=[0, 0, 1, 1]) # suptitleが重ならないように調整
        
        # ファイル名を安全な形式に
        safe_filename = f"GraphA_Cover_{cover_img}_Secret_{secret_img}.png"
        save_path = os.path.join(output_dir, safe_filename)
        plt.savefig(save_path, bbox_inches='tight')
        plt.close(fig)

    print("グラフA の生成完了。")

    # 6. グラフ生成B：カバー画像別 最適TH比較
    print("\nグラフB (カバー画像別 最適TH比較) を生成中...")

    unique_secret_images = optimal_points_df['Secret Image'].unique()

    for secret_img in unique_secret_images:
        fig, axes = plt.subplots(len(matrix_types), 1, 
                                 figsize=(12, 6 * len(matrix_types)), 
                                 sharey=True) # Y軸（TH）を共有

        if len(matrix_types) == 1:
            axes = [axes]
            
        fig.suptitle(f'Optimal TH Comparison (Secret Image: {secret_img})\n', 
                     fontsize=16, y=1.03)

        for i, matrix_type in enumerate(matrix_types):
            ax = axes[i]
            
            # 最適化データからフィルタリング
            plot_data = optimal_points_df[
                (optimal_points_df['Secret Image'] == secret_img) &
                (optimal_points_df['Matrix Type'] == matrix_type)
            ].sort_values('Cover Image')
            
            if not plot_data.empty:
                bars = ax.bar(plot_data['Cover Image'], plot_data['TH'])
                
                # 棒グラフの上にTHの値を表示
                ax.bar_label(bars, fmt='%d')
                
                ax.set_title(f'Matrix: {matrix_type}')
                ax.set_ylabel('Optimal TH')
                ax.set_xlabel('Cover Image')
            else:
                ax.set_title(f'Matrix: {matrix_type} (No Data)')

        # Y軸の範囲を調整（例: 0から最大TH+2まで）
        if not optimal_points_df.empty:
            max_th = optimal_points_df['TH'].max()
            plt.setp(axes, ylim=(0, max_th + 2))

        plt.tight_layout(rect=[0, 0, 1, 1])
        
        safe_filename = f"GraphB_Optimal_TH_Comparison_Secret_{secret_img}.png"
        save_path = os.path.join(output_dir, safe_filename)
        plt.savefig(save_path, bbox_inches='tight')
        plt.close(fig)

    print("グラフB の生成完了。")
    print(f"\nすべての分析が完了しました。結果は {output_dir} を確認してください。")

# --- スクリプトの実行 ---
if __name__ == "__main__":
    # スクリプトのディレクトリを取得
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # CSVファイルのパス（スクリプトの場所に基づいて相対パスを計算）
    # 修正点：ファイル名を "comparison_results_all.csv" に変更
    csv_path = os.path.join(script_dir, "output", "cover_image_comparison", "comparison_results_all.csv")
    
    # 出力ディレクトリ
    output_dir = os.path.join(script_dir, "output", "cover_image_comparison", "plots")
    
    # 出力ディレクトリが存在しない場合は作成
    os.makedirs(output_dir, exist_ok=True)
    
    # 修正されたパス変数を使用して関数を実行
    analyze_psnr_data(csv_path, output_dir)