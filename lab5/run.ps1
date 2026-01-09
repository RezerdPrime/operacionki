# run.ps1
Write-Host "Starting temperature monitoring system..." -ForegroundColor Green

# Start listener
Start-Process -FilePath ".\build\listener.exe" -ArgumentList "COM4" -WindowStyle Normal
Write-Host "Listener started (COM4)" -ForegroundColor Cyan


# Start emulator
Start-Process -FilePath ".\build\emulator.exe" -ArgumentList "COM3" -WindowStyle Normal
Write-Host "Emulator started (COM3)" -ForegroundColor Cyan