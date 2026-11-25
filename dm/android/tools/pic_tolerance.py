#!/usr/bin/env python3





import os
import sys

import numpy as np
from PIL import Image

WITHIN_LEVELS = 16
WITHIN_SHARE = 0.99
MEAN_LEVELS = 4.0


def pictures(appart):

    root = os.path.join(appart, 'mon')
    return sorted(os.path.relpath(os.path.join(d, f), appart) for d, _, files in os.walk(root) for f in files if f.endswith('.webp'))


def compare_picture(ref_path, new_path):

    with Image.open(ref_path) as a, Image.open(new_path) as b:
        ra, rb = np.asarray(a.convert('RGBA'), np.int16), np.asarray(b.convert('RGBA'), np.int16)
    if ra.shape != rb.shape:
        return 0.0, float('inf'), 'size %s, reference %s' % (rb.shape[:2], ra.shape[:2])
    if not np.array_equal(ra[:, :, 3], rb[:, :, 3]):
        return 0.0, float('inf'), '%d alpha values differ' % int(np.count_nonzero(ra[:, :, 3] != rb[:, :, 3]))
    visible = ra[:, :, 3] > 0
    diff = np.abs(ra[:, :, :3] - rb[:, :, :3])[visible]
    if not diff.size:
        return 1.0, 0.0, None
    within, mean = float(np.mean(diff <= WITHIN_LEVELS)), float(np.mean(diff))
    problem = None
    if within < WITHIN_SHARE or mean > MEAN_LEVELS:
        problem = '%.2f %% of the colour values within %d levels, mean difference %.2f' % (100 * within, WITHIN_LEVELS, mean)
    return within, mean, problem


def compare(reference, candidate):

    problems, stats = [], []
    with open(os.path.join(reference, 'mon.json'), 'rb') as f, open(os.path.join(candidate, 'mon.json'), 'rb') as g:
        if f.read() != g.read():
            problems.append(('mon.json', 'differs'))
    ref, new = pictures(reference), pictures(candidate)
    for rel in sorted(set(ref) ^ set(new)):
        problems.append((rel, 'only in the reference' if rel in ref else 'only in the candidate'))
    for rel in sorted(set(ref) & set(new)):
        within, mean, problem = compare_picture(os.path.join(reference, rel), os.path.join(candidate, rel))
        stats.append((rel, within, mean))
        if problem:
            problems.append((rel, problem))
    return problems, stats


def main(argv):
    if len(argv) != 3:
        raise SystemExit(__doc__)
    problems, stats = compare(argv[1], argv[2])
    for rel, what in problems:
        print('%s: %s' % (rel, what))
    if stats:
        means = sorted(m for _, _, m in stats)
        withins = sorted(w for _, w, _ in stats)
        print('pictures %d, outside the tolerance %d; mean difference median %.2f max %.2f; share within %d levels min %.4f' % (
            len(stats), len(problems), means[len(means) // 2], means[-1], WITHIN_LEVELS, withins[0]))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
