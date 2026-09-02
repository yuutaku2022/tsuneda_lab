import pandas as pd
import matplotlib.pyplot as plt
import os

csv_path = "./output/cover_image_comparison/comparison_results_all.csv"
df = pd.read_csv(csv_path)

df = df[df['Stego PSNR'] != 'ERROR']
df['Stego PSNR'] = df['Stego PSNR'].astype(float)
df['Extracted PSNR'] = df['Extracted PSNR'].astype(float)
df['TH'] = df['TH'].astype(int)
df['TargetLine'] = df['TargetLine'].astype(int)

plot_dir = "./output/cover_image_comparison/plots_all_targetline"
os.makedirs(plot_dir, exist_ok=True)

covers = df['Cover Image'].unique()
secrets = df['Secret Image'].unique()

for cover in covers:
    for secret in secrets:

        df_f = df[
            (df['Cover Image'] == cover) &
            (df['Secret Image'] == secret) &
            (df['Matrix Type'].str.startswith('Debruijn'))
        ]

        if df_f.empty:
            continue

        plt.figure(figsize=(9, 6))

        for tl in sorted(df_f['TargetLine'].unique()):
            df_tl = df_f[df_f['TargetLine'] == tl]

            plt.plot(
                df_tl['TH'],
                df_tl['Stego PSNR'],
                marker='o',
                linestyle='-',
                label=f'TL={tl} Stego'
            )
            plt.plot(
                df_tl['TH'],
                df_tl['Extracted PSNR'],
                marker='x',
                linestyle='--',
                label=f'TL={tl} Extracted'
            )

        plt.xlabel('Threshold TH')
        plt.ylabel('PSNR [dB]')
        plt.title(f'{cover} / {secret} (Debruijn)')
        plt.legend(fontsize=8, ncol=2)
        plt.grid(True)
        plt.tight_layout()

        save_path = os.path.join(
            plot_dir,
            f'{cover}_{secret}_Debruijn_all_targetline_psnr.jpg'
        )
        plt.savefig(save_path)
        plt.close()

        print(f'Saved: {save_path}')
