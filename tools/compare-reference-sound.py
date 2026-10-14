#!/usr/bin/env python3
"""Rebuild one identical offline fixture against the approved commit and this tree."""
import argparse
import io
from pathlib import Path
import subprocess
import tarfile
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--reference', default='341fc8b')
parser.add_argument('--compiler', default='g++')
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='voicemorph-reference-') as directory:
    temp = Path(directory)
    archive = subprocess.check_output(['git', 'archive', args.reference, 'src'], cwd=root)
    with tarfile.open(fileobj=io.BytesIO(archive)) as files:
        files.extractall(temp / 'reference', filter='data')
    binaries = []
    for name, source in [('reference', temp / 'reference/src'), ('repaired', root / 'src')]:
        binary = temp / (name + '-render')
        subprocess.run([args.compiler, '-std=c++17', '-O2', '-I' + str(source),
                        str(root / 'tests/render_reference_character.cpp'), '-o', str(binary)], check=True)
        binaries.append(binary)
    for rate in (44100, 48000):
        outputs = []
        for index, binary in enumerate(binaries):
            path = temp / f'{rate}-{index}.raw'
            subprocess.run([str(binary), str(rate), str(path)], check=True)
            outputs.append(path.read_bytes())
        if outputs[0] != outputs[1]:
            raise SystemExit(f'{rate} Hz: reference fixture differs')
        print(f'{rate} Hz: {rate * 2} stereo frames byte-identical to {args.reference}')
