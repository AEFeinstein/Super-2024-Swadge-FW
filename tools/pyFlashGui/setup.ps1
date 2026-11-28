Invoke-WebRequest -Uri https://github.com/pbatard/libwdi/releases/download/v1.5.1/zadig-2.9.exe -Outfile zadig-2.9.exe
# TODO: Make PowerShell wait for this to exit before continuing the script, then uncomment Remove-Item
.\zadig-2.9.exe
#Remove-Item zadig-2.9.exe

pip3 install -r requirements.txt
