# instalar_windows.ps1 - Instalacion manual de sduino (STM8) para Arduino IDE 2.x
#
# Hace tres cosas:
#   1. Clona (o actualiza con git pull) este repositorio en <sketchbook>\sduino-repo
#   2. Crea el enlace <sketchbook>\hardware\sduino -> sduino-repo\sduino
#      (una "union de directorio": no necesita permisos de administrador)
#   3. Descarga el compilador SDCC 3.9 para STM8 en sduino-repo\sduino\tools\sdcc\3.9.1
#
# Uso (en PowerShell):
#   powershell -ExecutionPolicy Bypass -File instalar_windows.ps1
#   powershell -ExecutionPolicy Bypass -File instalar_windows.ps1 -Sketchbook "D:\MisSketches"
#
# Por defecto el sketchbook es Documentos\Arduino (el del IDE: Archivo > Preferencias).
# Necesita git (https://git-scm.com) y Windows 10 o posterior (incluye tar).

param(
	[string]$Sketchbook = (Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'Arduino'),
	[string]$Rama = 'development'
)
$ErrorActionPreference = 'Stop'

$repo = Join-Path $Sketchbook 'sduino-repo'
$hw   = Join-Path $Sketchbook 'hardware'
$link = Join-Path $hw 'sduino'
$sdcc = Join-Path $repo 'sduino\tools\sdcc\3.9.1'
$sdccUrl = 'https://github.com/tenbaht/sduino/releases/download/v0.5/sdcc-stm8-i586-mingw32msvc-20190502-11242.tar.bz2'

Write-Host "Sketchbook: $Sketchbook"

# 1. repositorio
if (Test-Path (Join-Path $repo '.git')) {
	Write-Host 'Actualizando el repositorio...'
	git -C $repo pull
} else {
	Write-Host 'Clonando el repositorio...'
	git clone -b $Rama https://github.com/CEPH81/sduino.git $repo
}
if ($LASTEXITCODE -ne 0) { throw 'git fallo' }

# 2. enlace en hardware\
New-Item -ItemType Directory -Force $hw | Out-Null
if (Test-Path $link) {
	$item = Get-Item $link -Force
	if ($item.LinkType -ne 'Junction') {
		throw "Ya existe $link y no es un enlace. Muevalo o borrelo (puede ser una instalacion anterior de sduino)."
	}
} else {
	New-Item -ItemType Junction -Path $link -Target (Join-Path $repo 'sduino') | Out-Null
	Write-Host "Enlace creado: $link"
}

# 3. compilador SDCC
if (Test-Path (Join-Path $sdcc 'bin\sdcc.exe')) {
	Write-Host 'SDCC ya esta instalado.'
} else {
	Write-Host 'Descargando SDCC 3.9 para STM8...'
	New-Item -ItemType Directory -Force $sdcc | Out-Null
	$tmp = Join-Path $env:TEMP 'sdcc-stm8-win.tar.bz2'
	[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
	Invoke-WebRequest $sdccUrl -OutFile $tmp
	tar -xjf $tmp -C $sdcc --strip-components=1
	if ($LASTEXITCODE -ne 0) { throw 'no se pudo descomprimir SDCC' }
	Remove-Item $tmp
	Write-Host "SDCC instalado en $sdcc"
}

Write-Host ''
Write-Host 'Listo. Reinicie el Arduino IDE y elija Herramientas > Placa > STM8S (sduino).'
Write-Host 'Para cargar con ST-Link: instale el controlador WinUSB con Zadig (ver README).'
