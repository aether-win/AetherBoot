<#
.SYNOPSIS
  Comprueba la integridad de las imágenes copiadas a una USB Ventoy (Windows).

.DESCRIPTION
  Para cada imagen (.iso .wim .img .vhd .vhdx .efi .dat .vtoy) busca una suma de
  verificación de referencia y la compara con la del archivo:
    * un archivo hermano: imagen.iso.sha256 / .sha256sum / .sha512 / .sha1 / .md5
    * o una lista en la misma carpeta: SHA256SUMS, sha256sum.txt, CHECKSUM, *.sha256 ...
  Formatos admitidos: "hash  archivo", "hash *archivo", "SHA256 (archivo) = hash"
  o un archivo con solo el hash.

.PARAMETER Carpeta
  Raíz de la partición de datos del USB. Si se omite, se busca la unidad con etiqueta "Aether" (o "Ventoy" en USB antiguos).

.PARAMETER Generar
  Crea "imagen.sha256" para las imágenes sin referencia.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File VerificarISOs.ps1
  powershell -ExecutionPolicy Bypass -File VerificarISOs.ps1 -Carpeta E:\ -Generar
#>
param(
    [string]$Carpeta = "",
    [switch]$Generar
)

$ErrorActionPreference = "Stop"

if (-not $Carpeta) {
    $vol = Get-Volume -ErrorAction SilentlyContinue | Where-Object { $_.FileSystemLabel -in @("Aether", "Ventoy") -and $_.DriveLetter } | Sort-Object { $_.FileSystemLabel -ne "Aether" } | Select-Object -First 1
    if ($vol) { $Carpeta = "$($vol.DriveLetter):\" } else { $Carpeta = (Get-Location).Path }
}
if (-not (Test-Path -LiteralPath $Carpeta -PathType Container)) {
    Write-Host "No existe la carpeta: $Carpeta" -ForegroundColor Red
    exit 2
}

$algPorLongitud = @{ 32 = "MD5"; 40 = "SHA1"; 64 = "SHA256"; 128 = "SHA512" }
$regexHash = '(?<![0-9a-fA-F])([0-9a-fA-F]{128}|[0-9a-fA-F]{64}|[0-9a-fA-F]{40}|[0-9a-fA-F]{32})(?![0-9a-fA-F])'

function Get-PrimerHash([string]$texto) {
    $m = [regex]::Match($texto, $regexHash)
    if ($m.Success) { return $m.Groups[1].Value } else { return $null }
}

function Find-Referencia([System.IO.FileInfo]$img) {
    $base = $img.Name
    foreach ($ext in "sha256","sha256sum","sha512","sha512sum","sha1","sha1sum","md5","md5sum") {
        $cand = "$($img.FullName).$ext"
        if (Test-Path -LiteralPath $cand -PathType Leaf) {
            $lineas = Get-Content -LiteralPath $cand -ErrorAction SilentlyContinue
            $linea = $lineas | Where-Object { $_ -like "*$base*" } | Select-Object -First 1
            if (-not $linea) { $linea = $lineas | Select-Object -First 1 }
            $h = Get-PrimerHash "$linea"
            if ($h) { return $h }
        }
    }
    $patrones = "SHA256SUMS*","SHA512SUMS*","SHA1SUMS*","MD5SUMS*","sha256sum*","sha512sum*","md5sum*","*CHECKSUM*","*.sha256","*.sha512"
    foreach ($p in $patrones) {
        foreach ($lista in Get-ChildItem -LiteralPath $img.DirectoryName -Filter $p -File -ErrorAction SilentlyContinue) {
            foreach ($linea in Get-Content -LiteralPath $lista.FullName -ErrorAction SilentlyContinue) {
                if ($linea.Contains($base) -and -not $linea.Contains("$base.")) {
                    $h = Get-PrimerHash $linea
                    if ($h) { return $h }
                }
            }
        }
    }
    return $null
}

$extensiones = ".iso",".wim",".img",".vhd",".vhdx",".efi",".dat",".vtoy"
$imagenes = Get-ChildItem -LiteralPath $Carpeta -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $extensiones -contains $_.Extension.ToLower() -and
                   $_.FullName -notmatch '\\ventoy\\' -and $_.FullName -notmatch '\\\$RECYCLE\.BIN\\' } |
    Sort-Object FullName

Write-Host "Verificando imágenes en: $Carpeta`n"
$ok = 0; $mal = 0; $sinRef = 0; $gen = 0

foreach ($img in $imagenes) {
    $rel = $img.FullName.Substring($Carpeta.TrimEnd('\').Length).TrimStart('\')
    $ref = Find-Referencia $img

    if (-not $ref) {
        if ($Generar) {
            Write-Host "  $rel ... " -NoNewline
            $h = (Get-FileHash -LiteralPath $img.FullName -Algorithm SHA256).Hash.ToLower()
            Set-Content -LiteralPath "$($img.FullName).sha256" -Value "$h  $($img.Name)" -Encoding Ascii
            Write-Host "GENERADA" -ForegroundColor Yellow
            $gen++
        } else {
            Write-Host "  SIN REFERENCIA  $rel" -ForegroundColor Yellow
            $sinRef++
        }
        continue
    }

    $alg = $algPorLongitud[$ref.Length]
    Write-Host "  $rel [$alg] ... " -NoNewline
    $calc = (Get-FileHash -LiteralPath $img.FullName -Algorithm $alg).Hash
    if ($calc -ieq $ref) {
        Write-Host "OK" -ForegroundColor Green
        $ok++
    } else {
        Write-Host "NO COINCIDE" -ForegroundColor Red
        Write-Host "      esperado: $($ref.ToLower())"
        Write-Host "      obtenido: $($calc.ToLower())"
        $mal++
    }
}

Write-Host "`nTotal: $($imagenes.Count)   Correctas: $ok   Dañadas: $mal   Sin referencia: $sinRef   Generadas: $gen"
if ($imagenes.Count -eq 0) { Write-Host "No se encontraron imágenes." }
if ($mal -gt 0) {
    Write-Host "Hay imágenes dañadas: vuelva a descargarlas o copiarlas." -ForegroundColor Red
    exit 1
}
exit 0
