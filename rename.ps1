# Get all files in the current directory
$files = Get-ChildItem -File

foreach ($file in $files) {
    $newName = $file.Name + ".pcap"
    Rename-Item -Path $file.FullName -NewName $newName
    Write-Host "Renamed '$($file.Name)' to '$newName'"
}