#!/usr/bin/env python3
"""Prepare app folder descriptors. Does not modify anything without --aplicar.
The Edition also accepts loose .ELF files directly in APPS.
"""
from pathlib import Path
import argparse


def candidates(folder: Path):
    if not folder.is_dir():
        raise ValueError(f'Pasta APPS nao encontrada: {folder}')
    for sub in sorted(folder.iterdir(), key=lambda p: p.name.lower()):
        if not sub.is_dir() or sub.is_symlink() or (sub/'title.cfg').exists():
            continue
        files = [f for f in sub.iterdir() if f.is_file() and not f.is_symlink() and f.suffix.lower() == '.elf']
        if len(files) == 1:
            yield sub, files[0]
        elif len(files) > 1:
            print(f'IGNORADO (mais de um ELF): {sub.name}')


def main():
    ap=argparse.ArgumentParser(description='Gera arquivos title.cfg para aplicativos PS2 organizados em subpastas.')
    ap.add_argument('apps', type=Path, help='Caminho do APPS no pendrive')
    ap.add_argument('--aplicar', action='store_true', help='Gravar title.cfg, sem substituir arquivos existentes')
    a=ap.parse_args()
    found=0
    for sub,elf in candidates(a.apps):
        title=sub.name.replace('_',' ').strip()
        if '\n' in title or '\r' in title or '=' in title:
            print('IGNORADO (nome invalido):',sub.name)
            continue
        payload=f'title={title}\nboot={elf.name}\n'
        out=sub/'title.cfg'
        if a.aplicar:
            try:
                with out.open('x',encoding='utf-8',newline='\n') as f:
                    f.write(payload)
                print('CRIADO:',out)
            except FileExistsError:
                print('JA EXISTE:',out)
        else:
            print('PREVISTO:',out, '→',repr(payload))
        found += 1
    print(f'{found} descricao(oes) {"criadas" if a.aplicar else "previstas"}.')
    if not a.aplicar:
        print('Nenhum arquivo alterado. Para gravar acrescente --aplicar.')

if __name__ == '__main__':
    main()
