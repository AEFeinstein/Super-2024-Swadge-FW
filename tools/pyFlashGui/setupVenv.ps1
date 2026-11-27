#Invoke-WebRequest -Uri https://github.com/pbatard/libwdi/releases/download/v1.4.1/zadig-2.7.exe -Outfile zadig-2.7.exe
# TODO: Make PowerShell wait for this to exit before continuing the script, then uncomment Remove-Item
#.\zadig-2.7.exe
#Remove-Item zadig-2.7.exe

#pip3 install pyserial esptool



if (-not (Test-Path ./.venv/Scripts/Activate.ps1)) {
    Write-Host "Creating python virtual environment"
    python -m venv .venv }
else {
    Write-Host "Updating Existing python virtual environment" }



./venv/Scripts/Activate.ps1

$output = & python -m pip install --upgrade pip 2>&1
if ($LASTEXITCODE -ne 0) {
    throw "pip update failed"
    }
$changes = $output | Where-Object {
    $_ -match '^Successfully installed' }
if ($changes) {
    $output | Out-Host }
else {
    Write-Host "Pip already at current version." }

$output = & python -m pip install -r requirements.txt 2>&1
if ($LASTEXITCODE -ne 0) {
    throw "package update failed"
    }
$changes = $output | Where-Object {
    $_ -match '^Successfully installed' }
if ($changes) {
    $output | Out-Host }
else {
    Write-Host "Packaged already at current version." }
