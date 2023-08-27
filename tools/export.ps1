# Make an esp folder only if it does not yet exist, and move into it regardless
$Filename="~/esp"
if (-not(test-path $Filename)){mkdir $FileName}

pushd ~/esp/esp-idf

# Install tools
./install.ps1

# Export paths and variables
./export.ps1

popd
