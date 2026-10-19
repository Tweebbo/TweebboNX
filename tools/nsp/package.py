#!/usr/bin/env python3
"""Build a standalone application NSP, keeping key material outside the project."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
TITLE = '0100F7EEBB000000'
IMAGE = 'docker.io/devkitpro/devkita64:latest'


def run(args, **kwargs):
    subprocess.run(args, check=True, **kwargs)


def keyset(path):
    required = {'header_key': 64, 'key_area_key_application_00': 32}
    found = {}
    for line in path.read_text().splitlines():
        name, sep, value = line.partition('=')
        name, value = name.strip(), value.strip()
        if sep and name in required:
            if not re.fullmatch(r'[0-9a-fA-F]{%d}' % required[name], value) or not int(value, 16):
                raise SystemExit('Invalid required key: ' + name)
            found[name] = value
    if found.keys() != required.keys():
        raise SystemExit('Missing required keys: ' + ', '.join(required.keys() - found.keys()))
    return ''.join(f'{name} = {value}\n' for name, value in found.items())


def check_pfs0(path):
    data = path.read_bytes()
    magic, count, strings, _ = struct.unpack_from('<4sIII', data)
    if magic != b'PFS0' or count != 3:
        raise SystemExit('Expected an installable NSP with program, control and meta NCAs.')
    start = 16 + 24 * count
    payload = start + strings
    files = []
    for i in range(count):
        offset, size, name_at, _ = struct.unpack_from('<QQII', data, 16 + 24 * i)
        name = data[start + name_at:data.index(b'\0', start + name_at)].decode()
        content = data[payload + offset:payload + offset + size]
        if len(content) != size or name.split('.')[0] != hashlib.sha256(content).hexdigest()[:32]:
            raise SystemExit('NCA content hash mismatch.')
        files.append({'name': name, 'bytes': size})
    if sum(f['name'].endswith('.cnmt.nca') for f in files) != 1:
        raise SystemExit('Missing content metadata NCA.')
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--keys', type=Path, default=os.environ.get('SWITCH_KEYS'))
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    stage = ROOT / 'build/nsp-stage'
    if stage.exists():
        shutil.rmtree(stage)
    (stage / 'exefs').mkdir(parents=True)
    (stage / 'control').mkdir()
    run(['podman', 'run', '--rm', '-v', f'{ROOT}:/work:z', '-w', '/work', IMAGE,
         'bash', '-lc', 'set -e; elf2nso tweebbo.elf build/nsp-stage/exefs/main; '
         'npdmtool tools/nsp/application.json build/nsp-stage/exefs/main.npdm; '
         'nacptool --create Tweebbo Tweebbo "$(cat VERSION)" build/nsp-stage/control/control.nacp '
         f'--titleid={TITLE}'])
    shutil.copyfile(ROOT / 'assets/icon.jpg', stage / 'control/icon_AmericanEnglish.dat')
    nso = (stage / 'exefs/main').read_bytes()
    npdm = (stage / 'exefs/main.npdm').read_bytes()
    nacp = (stage / 'control/control.nacp').read_bytes()
    aci = struct.unpack_from('<I', npdm, 0x70)[0]
    if nso[:4] != b'NSO0' or npdm[:4] != b'META' or len(nacp) != 0x4000:
        raise SystemExit('Invalid application payload.')
    if struct.unpack_from('<Q', npdm, aci + 0x10)[0] != int(TITLE, 16):
        raise SystemExit('NPDM title ID mismatch.')
    if struct.unpack_from('<Q', nacp, 0x3038)[0] != int(TITLE, 16):
        raise SystemExit('NACP title ID mismatch.')
    print('Standalone NSO, application NPDM, HOME metadata and icon prepared.')
    if args.prepare_only:
        return
    if not args.keys or not args.keys.is_file():
        raise SystemExit('Supply --keys /private/path/prod.keys or SWITCH_KEYS to create the NSP.')
    keys = keyset(args.keys)
    packer = ROOT / 'build/hacbrewpack/hacbrewpack'
    if not packer.is_file():
        run(['bash', str(ROOT / 'tools/nsp/install-packer.sh')])
    with tempfile.TemporaryDirectory(prefix='tweebbo-nsp-') as scratch:
        work = Path(scratch)
        private = work / 'keys.dat'
        private.write_text(keys)
        private.chmod(0o600)
        # Pass only the two understood keys: old packer versions echo unknown key values.
        result = subprocess.run([str(packer), '-k', str(private), '--nologo',
            '--exefsdir', str(stage / 'exefs'), '--romfsdir', str(ROOT / 'romfs'),
            '--controldir', str(stage / 'control')], cwd=work, capture_output=True)
        if result.returncode:
            raise SystemExit('NSP packer failed; its output was withheld to protect key material.')
        candidates = list((work / 'hacbrewpack_nsp').glob('*.nsp'))
        if len(candidates) != 1:
            raise SystemExit('NSP packer did not produce exactly one package.')
        files = check_pfs0(candidates[0])
        verify = work / 'verified'
        verify.mkdir()
        command = ['podman', 'run', '--rm', '-v', f'{work}:/check:z', IMAGE,
                   'bash', '-lc', 'set -e; for nca in /check/hacbrewpack_nca/*.nca; do '
                   'name=$(basename "$nca"); mkdir -p "/check/verified/$name"; '
                   'hactool --suppresskeys --disablekeywarns -k /check/keys.dat -x -y '
                   '--header="/check/verified/$name/header" '
                   '--exefsdir="/check/verified/$name/exefs" '
                   '--romfsdir="/check/verified/$name/romfs" "$nca"; done']
        checked = subprocess.run(command, capture_output=True)
        if checked.returncode:
            raise SystemExit('Independent NCA extraction failed.')
        failures = [line for line in checked.stdout.decode(errors='replace').splitlines()
                    if 'FAIL' in line and 'Signature' not in line]
        if failures:
            raise SystemExit('Independent NCA hash verification failed.')
        types = {}
        for directory in verify.iterdir():
            header = (directory / 'header').read_bytes()
            if header[0x200:0x204] != b'NCA3' or struct.unpack_from('<Q', header, 0x210)[0] != int(TITLE, 16):
                raise SystemExit('NCA header or title ID mismatch.')
            types[header[0x205]] = directory
        if set(types) != {0, 1, 2}:
            raise SystemExit('Expected Program, Meta and Control NCAs.')
        for name in ('main', 'main.npdm'):
            if (types[0] / 'exefs' / name).read_bytes() != (stage / 'exefs' / name).read_bytes():
                raise SystemExit('Extracted executable differs from build.')
        assets = 0
        for source in (ROOT / 'romfs').rglob('*'):
            if source.is_file():
                if (types[0] / 'romfs' / source.relative_to(ROOT / 'romfs')).read_bytes() != source.read_bytes():
                    raise SystemExit('Extracted RomFS differs from build.')
                assets += 1
        for source in (stage / 'control').iterdir():
            if (types[2] / 'romfs' / source.name).read_bytes() != source.read_bytes():
                raise SystemExit('Extracted HOME metadata differs from build.')
        output = ROOT / 'out/tweebbo.nsp'
        shutil.copyfile(candidates[0], output)
        manifest = {'title_id': TITLE, 'title': 'Tweebbo', 'version': (ROOT / 'VERSION').read_text().strip(),
                    'kind': 'standalone application', 'bytes': output.stat().st_size,
                    'sha256': hashlib.sha256(output.read_bytes()).hexdigest(), 'contents': files,
                    'romfs_files_verified': assets, 'nca_extraction_verified': True,
                    'hardware_verified': False}
        (ROOT / 'out/tweebbo-nsp.json').write_text(json.dumps(manifest, indent=2) + '\n')
        print(f'Created {output}: {manifest["bytes"]} bytes; three NCA hashes verified.')


if __name__ == '__main__':
    main()
