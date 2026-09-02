import pandas as pd
import matplotlib.pyplot as plt
import os
import re

# 比較元 CSV とランダム変換なし CSV
csv_path_random = "./output/cover_image_comparison5/comparison_results_all.csv"
csv_path_original = "./output/cover_image_comparison/comparison_results_all.csv"

df_random = pd.read_csv(csv_path_random)
df_original = pd.read_csv(csv_path_original)

# エラー除外
df_random = df_random[df_random['Extracted PSNR'] != 'ERROR']
df_original = df_original[df_original['Extracted PSNR'] != 'ERROR']

df_random['Extracted PSNR'] = df_random['Extracted PSNR'].astype(float)
df_original['Extracted PSNR'] = df_original['Extracted PSNR'].astype(float)
df_random['TH'] = df_random['TH'].astype(int)
df_original['TH'] = df_original['TH'].astype(int)

# Matrix Type から flipCount を抽出
def extract_flip(matrix_type):
    m = re.search(r'flip_(\d+)', matrix_type)
    return int(m.group(1)) if m else 0

df_random['FlipCount'] = df_random['Matrix Type'].apply(extract_flip)
df_original['FlipCount'] = 0  # 元の行列は FlipCount=0 と扱う

# CSV を結合
df_all = pd.concat([df_random, df_original], ignore_index=True)

# プロット用ディレクトリ
plot_dir = "./output/cover_image_comparison5/plots/by_flip_extracted_with_original"
os.makedirs(plot_dir, exist_ok=True)

for cover in df_all['Cover Image'].unique():
    for secret in df_all['Secret Image'].unique():
        df_cs = df_all[(df_all['Cover Image'] == cover) & (df_all['Secret Image'] == secret)]
        if df_cs.empty:
            continue

        plt.figure(figsize=(10,6))

        for flip in sorted(df_cs['FlipCount'].unique()):
            df_flip = df_cs[df_cs['FlipCount'] == flip]
            label = 'original' if flip == 0 else f'flip={flip}'
            plt.plot(df_flip['TH'], df_flip['Extracted PSNR'],
                     'o-' if flip != 0 else 's-', label=label)

        plt.xlabel('Threshold TH')
        plt.ylabel('Extracted PSNR [dB]')
        plt.title(f'{cover} / {secret} (all flipCounts + original)')
        plt.legend()
        plt.grid(True)
        plt.tight_layout()

        save_path = os.path.join(plot_dir, f'{cover}_{secret}_all_flip_plus_original_extracted_psnr.jpg')
        plt.savefig(save_path)
        plt.close()
        print(f'Saved: {save_path}')
