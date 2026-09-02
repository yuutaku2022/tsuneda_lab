"""
直交変換後の係数行列をヒートマップで可視化するスクリプト
"""

import numpy as np
import matplotlib
matplotlib.use('Agg')  # ヘッドレスモード
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from pathlib import Path
from PIL import Image
import argparse


def compute_sequency(row):
    """
    1行の交番数(sequency)を計算する（符号が変わる回数）
    """
    count = 0
    for i in range(1, len(row)):
        if row[i] != row[i - 1]:
            count += 1
    return count


def reorder_walsh_sequency(H):
    """
    ハダマール行列を交番数順に並び替え、行と列の両方を揃えて対称にする
    """
    N = H.shape[0]
    sequency_index = []
    
    for i in range(N):
        sequency = compute_sequency(H[i, :])
        sequency_index.append((sequency, i))
    
    sequency_index.sort(key=lambda x: x[0])
    
    # 行を交番数順に並べ替え
    ordered = np.zeros_like(H)
    for i in range(N):
        ordered[i, :] = H[sequency_index[i][1], :]
    
    # 列も同じ順序で並べ替え
    result = np.zeros_like(H)
    for j in range(N):
        result[:, j] = ordered[:, sequency_index[j][1]]
    
    return result


def load_debruijn_sequence(file_path):
    """
    ドブルイン系列をテキストファイルから読み込む
    """
    with open(file_path, 'r') as f:
        content = f.read()
    
    # 改行と空白を除去し、0と1のみを保持
    sequence = ''.join(c for c in content if c in '01')
    return sequence


def hadamard_transform(image_matrix):
    """
    Hadamard変換を実行（正規化版、Walsh交番数順）
    """
    def hadamard_matrix(n):
        """Hadamard行列を生成"""
        if n == 1:
            return np.array([[1.0]])
        else:
            h = hadamard_matrix(n // 2)
            return np.vstack([
                np.hstack([h, h]),
                np.hstack([h, -h])
            ])
    
    n = image_matrix.shape[0]
    # nが2のべき乗であることを確認
    if (n & (n - 1)) != 0:
        raise ValueError(f"行列のサイズは2のべき乗である必要があります: {n}")
    
    h = hadamard_matrix(n)
    # Walsh交番数順に並べ替え
    h = reorder_walsh_sequency(h)
    h = h / np.sqrt(n)  # 正規化
    
    # 係数行列 = H * image * H^T
    coefficients = h @ image_matrix @ h.T
    return coefficients


def debruijn_transform(image_matrix, debruijn_path):
    """
    ドブルイン行列を使用した直交変換
    """
    def hadamard_matrix(n):
        """Hadamard行列を生成"""
        if n == 1:
            return np.array([[1.0]])
        else:
            h = hadamard_matrix(n // 2)
            return np.vstack([
                np.hstack([h, h]),
                np.hstack([h, -h])
            ])
    
    n = image_matrix.shape[0]
    if (n & (n - 1)) != 0:
        raise ValueError(f"行列のサイズは2のべき乗である必要があります: {n}")
    
    # Walsh行列を生成して交番数順に並べ替え
    h = hadamard_matrix(n)
    walsh = reorder_walsh_sequency(h)
    walsh = walsh / np.sqrt(n)
    
    # ドブルイン系列を読み込む
    sequence = load_debruijn_sequence(debruijn_path)
    
    # ウィンドウサイズ（ログ2(n)）
    window_size = int(np.log2(n))
    
    # 系列が足りなければ繰り返す
    if len(sequence) < window_size + n - 1:
        sequence = sequence * (1 + (window_size + n - 1) // len(sequence))
    
    # ドブルイン行列を生成（Walsh行列の列を並べ替え）
    debruijn = np.zeros_like(walsh)
    for col in range(n):
        window = sequence[col:col + window_size]
        index = int(window, 2)  # 2進数から10進数に変換
        if index < 0 or index >= n:
            raise ValueError(f"不正なインデックス: {index}")
        debruijn[:, col] = walsh[:, index]
    
    # 係数行列 = D * image * D^T
    coefficients = debruijn @ image_matrix @ debruijn.T
    return coefficients


def load_image(image_path, size=256):
    """
    画像を読み込んでグレースケール行列に変換
    """
    img = Image.open(image_path).convert('L')  # グレースケール
    img_resized = img.resize((size, size), Image.LANCZOS)
    return np.array(img_resized, dtype=np.float64)


def create_heatmap_figure(coefficients, title, image_name, save_path=None, use_percentile=True, alpha=0.7):
    """
    係数行列をヒートマップで可視化
    alpha: 色の透明度（0.0-1.0、低いほど薄い）
    """
    fig, ax = plt.subplots(figsize=(10, 9))
    
    # パーセンタイル値を使用して異常値の影響を軽減
    if use_percentile:
        # パーセンタイル値の範囲を広げて、より多くの値が色の範囲に分布するようにする
        pct_low = np.percentile(coefficients, 0.5)
        pct_high = np.percentile(coefficients, 99.5)
        abs_max = max(abs(pct_low), abs(pct_high))
        min_val = np.min(coefficients)
        max_val = np.max(coefficients)
    else:
        # 最小値と最大値を決定
        min_val = np.min(coefficients)
        max_val = np.max(coefficients)
        # 絶対値の最大値を基準に調整（対称な範囲）
        abs_max = max(abs(min_val), abs(max_val))
    
    # ヒートマップを表示（RdBu_rは彩度が低めなので薄く見える）
    im = ax.imshow(coefficients, cmap='RdBu_r', aspect='auto', 
                   vmin=-abs_max, vmax=abs_max, interpolation='nearest', alpha=alpha)
    
    ax.set_xlabel('column', fontsize=12)
    ax.set_ylabel('row', fontsize=12)
    ax.set_title(title, fontsize=14, fontweight='bold')
    
    # カラーバーを追加
    cbar = plt.colorbar(im, ax=ax, pad=0.02)
    cbar.set_label('Coefficient Value', fontsize=11)
    
    # グリッドを表示（オプション）
    ax.grid(False)
    
    # 統計情報を表示
    if use_percentile:
        pct_low = np.percentile(coefficients, 0.5)
        pct_high = np.percentile(coefficients, 99.5)
        mean_val = np.mean(coefficients)
        std_val = np.std(coefficients)
        textstr = f'Mean: {mean_val:.2f}\nStd: {std_val:.2f}\nP0.5: {pct_low:.2f}\nP99.5: {pct_high:.2f}'
    else:
        mean_val = np.mean(coefficients)
        std_val = np.std(coefficients)
        textstr = f'Mean: {mean_val:.2f}\nStd: {std_val:.2f}\nMin: {min_val:.2f}\nMax: {max_val:.2f}'
    ax.text(0.02, 0.98, textstr, transform=ax.transAxes, fontsize=10,
            verticalalignment='top', bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"[OK] 保存完了: {save_path}")
    else:
        plt.show()


def main():
    parser = argparse.ArgumentParser(description='ドブルイン行列による係数行列ヒートマップ可視化')
    parser.add_argument('--image', type=str, default='../input/img/cover_image/Lenna_grayscale_256.bmp',
                        help='入力画像のパス')
    parser.add_argument('--size', type=int, default=256,
                        help='変換後のサイズ（デフォルト: 256）')
    parser.add_argument('--output', type=str, default='../output/debruijn_heatmap.png',
                        help='出力ファイルのパス')
    parser.add_argument('--no-save', action='store_true',
                        help='ファイルを保存しない')
    parser.add_argument('--no-percentile', action='store_true',
                        help='パーセンタイル値を使用しない（全値を使用）')
    parser.add_argument('--debruijn-file', type=str, default='../input/debruijn/deb256.dat',
                        help='ドブルイン系列ファイルのパス')
    parser.add_argument('--alpha', type=float, default=0.4,
                        help='色の透明度（0.0-1.0、低いほど薄い、デフォルト: 0.4）')
    parser.add_argument('--transpose', action='store_true',
                        help='直交行列を転置した形式で計算（D^T @ image @ D の形式）')
    
    args = parser.parse_args()
    
    # スクリプトの場所から相対パスを解決
    script_dir = Path(__file__).parent
    image_path = script_dir / args.image
    output_path = script_dir / args.output if not args.no_save else None
    debruijn_path = script_dir / args.debruijn_file
    
    if not image_path.exists():
        print(f"エラー: 画像ファイルが見つかりません: {image_path}")
        return
    
    if not debruijn_path.exists():
        print(f"エラー: ドブルイン系列ファイルが見つかりません: {debruijn_path}")
        return
    
    print(f"画像を読み込んでいます: {image_path}")
    image_matrix = load_image(str(image_path), args.size)
    
    print(f"ドブルイン変換を実行中（サイズ: {args.size}×{args.size}）...")
    print(f"  ドブルイン系列ファイル: {debruijn_path}")
    
    # ドブルイン行列を生成
    debruijn = debruijn_transform(image_matrix, str(debruijn_path))
    
    # 計算方式を選択
    if args.transpose:
        print("  計算方式: D^T @ image @ D（直交行列を転置）")
        coefficients = debruijn.T @ image_matrix @ debruijn
        title_suffix = '(Transposed)'
        # ファイル名に _transposed を挿入
        if output_path:
            output_path = Path(str(output_path).replace('.png', '_transposed.png'))
    else:
        print("  計算方式: D @ image @ D^T（標準）")
        coefficients = debruijn @ image_matrix @ debruijn.T
        title_suffix = ''
    
    print(f"係数行列の統計:")
    print(f"  最小値: {np.min(coefficients):.2f}")
    print(f"  最大値: {np.max(coefficients):.2f}")
    print(f"  平均値: {np.mean(coefficients):.2f}")
    print(f"  標準偏差: {np.std(coefficients):.2f}")
    
    # ヒートマップを作成
    title = f'Debruijn Transform Coefficients {title_suffix} ({args.size}×{args.size})'
    create_heatmap_figure(coefficients, title, image_path.name, 
                         str(output_path) if output_path else None,
                         use_percentile=not args.no_percentile,
                         alpha=args.alpha)


if __name__ == '__main__':
    main()
