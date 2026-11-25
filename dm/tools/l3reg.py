#!/usr/bin/env python3








import sys


def fit(rows, index, target):
    x = [row[0] for row in rows]
    y = [row[index] for row in rows]
    mean_x = sum(x) / len(x)
    mean_y = sum(y) / len(y)
    denominator = sum((value - mean_x) ** 2 for value in x)
    slope_per_tick = (
        sum((a - mean_x) * (b - mean_y) for a, b in zip(x, y)) / denominator
        if denominator else 0
    )
    slope_per_1000 = slope_per_tick * 1000
    projected_growth = (
        slope_per_1000 * (target - x[-1]) / 1000 + y[-1] - y[0]
    )
    return slope_per_1000, projected_growth


def main(path, target):
    rows = []
    for line in open(path):
        try:
            _, tick, heap, metal, handles, phys, rss, footprint = line.split()
            rows.append((int(tick), int(heap), int(metal), int(handles), int(phys), float(rss), float(footprint)))
        except ValueError:
            pass
    if len(rows) < 3:
        print('REGRESSION samples=%d insufficient' % len(rows))
        return 4
    cut = int(len(rows) * .30)
    kept = rows[cut:] or rows[-1:]
    heap_slope, heap_growth = fit(kept, 1, target)
    metal_slope, metal_growth = fit(kept, 2, target)
    handle_slope, handle_growth = fit(kept, 3, target)
    phys_slope, phys_growth = fit(kept, 4, target)
    rss_slope, rss_growth = fit(kept, 5, target)
    footprint_slope, footprint_growth = fit(kept, 6, target)
    phys_projected = kept[-1][4] + phys_growth
    print(
        'REGRESSION samples=%d kept=%d heap_slope_MB_1000=%.3f '
        'metal_slope_MB_1000=%.3f handle_slope_1000=%.3f '
        'heap_projected_MB=%.3f metal_projected_MB=%.3f handle_projected=%.3f '
        'phys_projected_GB=%.3f phys_growth_MB=%.3f '
        'rss_reported_MB=%.3f footprint_reported_MB=%.3f'
        % (
            len(rows),
            len(kept),
            heap_slope / 1048576,
            metal_slope / 1048576,
            handle_slope,
            heap_growth / 1048576,
            metal_growth / 1048576,
            handle_growth,
            phys_projected / 1073741824,
            phys_growth / 1048576,
            rss_growth / 1024,
            footprint_growth / 1048576,
        )
    )
    red = (heap_growth > 256 * 1048576 or metal_growth > 256 * 1048576
           or phys_projected > 4.5 * 1073741824 or handle_growth > 16)
    print('METRIC_RED=%d' % red)
    return 5 if red else 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv[1], int(sys.argv[2])))
