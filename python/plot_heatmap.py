import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Plot a heat diffusion simulation from a CSV file."
    )
    parser.add_argument(
        "csv",
        nargs="?",
        default="results.csv",
        help="CSV file produced by heat_sim (default: results.csv)",
    )
    parser.add_argument(
        "--save",
        default="heatmap.png",
        help="Path for the generated image (default: heatmap.png)",
    )
    args = parser.parse_args()

    csv_path = Path(args.csv)
    if not csv_path.is_file():
        raise FileNotFoundError(f"CSV file not found: {csv_path}")

    temperature = np.loadtxt(csv_path, delimiter=",", dtype=float)

    if temperature.ndim != 2:
        raise ValueError("Expected a 2D temperature grid in the CSV file.")

    if not np.isfinite(temperature).all():
        raise ValueError("Temperature grid contains NaN or infinity.")

    fig, ax = plt.subplots(figsize=(8, 6))

    heatmap = ax.imshow(
        temperature,
        origin="lower",
        cmap="inferno",
        interpolation="nearest",
        aspect="equal",
    )

    fig.colorbar(heatmap, ax=ax, label="Temperature")
    ax.set_title("Heat Diffusion")
    ax.set_xlabel("X position")
    ax.set_ylabel("Y position")
    fig.tight_layout()

    fig.savefig(args.save, dpi=150)
    print(f"Heatmap saved to: {args.save}")
    print(f"Grid dimensions: {temperature.shape[1]} x {temperature.shape[0]}")
    print(f"Minimum temperature: {temperature.min():.6g}")
    print(f"Maximum temperature: {temperature.max():.6g}")

    plt.show()


if __name__ == "__main__":
    main()