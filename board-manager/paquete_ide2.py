#!/usr/bin/env python3
"""
paquete_ide2.py - Crea el paquete del Gestor de placas (Arduino IDE 2.x).

Genera, en board-manager/dist/:
  - sduino-stm8-<version>.tar.bz2   (la plataforma: sduino/stm8)
  - package_sduino_ceph81_index.json (indice con esa plataforma y el SDCC)

El archivo se crea de forma reproducible (orden fijo, fecha fija, sin
usuario), asi que la suma SHA-256 solo cambia si cambia el contenido.

Publicacion:
  1. Ejecute:  python3 board-manager/paquete_ide2.py
  2. En GitHub cree la release "v<version>" de CEPH81/sduino y suba el .tar.bz2.
  3. Copie el .json a la raiz del repositorio (rama development) y haga push.
  4. En el IDE: Archivo > Preferencias > URLs adicionales del gestor de placas:
       https://raw.githubusercontent.com/CEPH81/sduino/development/package_sduino_ceph81_index.json

Parte de sduino. LGPL 2.1.
"""
import hashlib, io, json, os, re, sys, tarfile, bz2

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLATAFORMA = os.path.join(RAIZ, 'sduino', 'stm8')
DIST = os.path.join(RAIZ, 'board-manager', 'dist')
REPO = 'CEPH81/sduino'
FECHA = 1767225600          # 2026-01-01: fecha fija de los archivos

# Herramienta SDCC: la misma compilacion que usaba sduino 0.5 (publicada por tenbaht)
SDCC = {
    "name": "sdcc", "version": "build.11242",
    "systems": [
        {"host": "x86_64-pc-linux-gnu",
         "url": "https://github.com/tenbaht/sduino/releases/download/v0.5/sdcc-stm8-amd64-unknown-linux2.5-20190502-11242.tar.bz2",
         "archiveFileName": "sdcc-stm8-amd64-unknown-linux2.5-20190502-11242.tar.bz2"},
        {"host": "i686-mingw32",
         "url": "https://github.com/tenbaht/sduino/releases/download/v0.5/sdcc-stm8-i586-mingw32msvc-20190502-11242.tar.bz2",
         "archiveFileName": "sdcc-stm8-i586-mingw32msvc-20190502-11242.tar.bz2"},
        {"host": "x86_64-apple-darwin",
         "url": "https://github.com/tenbaht/sduino/releases/download/v0.5/sdcc-stm8-x86_64-apple-macosx-20190502-11242.tar.bz2",
         "archiveFileName": "sdcc-stm8-x86_64-apple-macosx-20190502-11242.tar.bz2"},
    ],
}

EXCLUIR = re.compile(r'(^|/)(platform\.local\.txt|build-[^/]*|\.git[^/]*|[^/]*\.(o|rel|lst|sym|asm|rst|ihx|map|cdb|lk))(/|$)')


def leer_version():
    for linea in open(os.path.join(PLATAFORMA, 'platform.txt'), encoding='utf-8'):
        if linea.startswith('version='):
            return linea.split('=', 1)[1].strip()
    sys.exit('platform.txt no tiene version=')


def placas():
    nombres = []
    for linea in open(os.path.join(PLATAFORMA, 'boards.txt'), encoding='utf-8'):
        m = re.match(r'^([A-Za-z0-9_]+)\.name=(.+)$', linea.strip())
        if m:
            nombres.append({"name": m.group(2)})
    return nombres


def crear_tar(version):
    nombre = 'sduino-stm8-%s.tar.bz2' % version
    tope = 'sduino-stm8-%s' % version
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode='w', format=tarfile.GNU_FORMAT) as tar:
        for raiz, dirs, archivos in os.walk(PLATAFORMA):
            dirs.sort()
            rel_dir = os.path.relpath(raiz, PLATAFORMA)
            for a in sorted(archivos):
                rel = os.path.normpath(os.path.join(rel_dir, a)).replace(os.sep, '/')
                if EXCLUIR.search(rel):
                    continue
                ruta = os.path.join(raiz, a)
                info = tar.gettarinfo(ruta, arcname=tope + '/' + rel)
                info.uid = info.gid = 0
                info.uname = info.gname = ''
                info.mtime = FECHA
                info.mode = 0o755 if (os.access(ruta, os.X_OK) or rel.endswith('.sh')) else 0o644
                with open(ruta, 'rb') as f:
                    tar.addfile(info, f)
    datos = bz2.compress(buf.getvalue(), 9)
    os.makedirs(DIST, exist_ok=True)
    with open(os.path.join(DIST, nombre), 'wb') as f:
        f.write(datos)
    return nombre, len(datos), hashlib.sha256(datos).hexdigest()


def main():
    version = leer_version()
    nombre, tam, sha = crear_tar(version)
    indice = {"packages": [{
        "name": "sduino",
        "maintainer": "CEPH81 (basado en sduino de Michael Mayer)",
        "websiteURL": "https://github.com/%s" % REPO,
        "email": "",
        "help": {"online": "https://github.com/%s" % REPO},
        "platforms": [{
            "name": "STM8S (sduino para Arduino IDE 2.x)",
            "architecture": "stm8",
            "version": version,
            "category": "Contributed",
            "url": "https://github.com/%s/releases/download/v%s/%s" % (REPO, version, nombre),
            "archiveFileName": nombre,
            "checksum": "SHA-256:" + sha,
            "size": str(tam),
            "help": {"online": "https://github.com/%s" % REPO},
            "boards": placas(),
            "toolsDependencies": [{"packager": "sduino", "name": "sdcc", "version": SDCC["version"]}],
        }],
        "tools": [SDCC],
    }]}
    # las sumas del SDCC se toman del indice original de sduino
    orig = json.load(open(os.path.join(RAIZ, 'package_sduino_stm8_index.json'), encoding='utf-8'))
    sumas = {}
    for t in orig['packages'][0]['tools']:
        if t['name'] == 'sdcc' and t['version'] == SDCC['version']:
            for s in t['systems']:
                sumas[s['archiveFileName']] = (s['checksum'], s['size'])
    for s in SDCC['systems']:
        s['checksum'], s['size'] = sumas[s['archiveFileName']]
    ruta_json = os.path.join(DIST, 'package_sduino_ceph81_index.json')
    with open(ruta_json, 'w', encoding='utf-8') as f:
        json.dump(indice, f, indent=2, ensure_ascii=False)
        f.write('\n')
    print('Plataforma %s: %s (%d bytes, SHA-256 %s)' % (version, nombre, tam, sha))
    print('Indice: %s' % ruta_json)


if __name__ == '__main__':
    main()
