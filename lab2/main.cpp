
#include "back.hpp"
#include <iostream>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif


void createTestProgram() {
#ifdef _WIN32

    FILE* bat = fopen("test_program.bat", "w");
    if (bat) {
        fprintf(bat, "@echo off\n");
        fprintf(bat, "echo Test program running on Windows\n");
        fprintf(bat, "timeout /t 2 /nobreak >nul\n");
        fprintf(bat, "echo Exiting with code %%1\n");
        fprintf(bat, "exit /b %%1\n");
        fclose(bat);
    }
    
    FILE* ps1 = fopen("test_ps.ps1", "w");
    if (ps1) {
        fprintf(ps1, "Write-Host 'PowerShell test script running'\n");
        fprintf(ps1, "Start-Sleep -Seconds 1\n");
        fprintf(ps1, "Write-Host 'Exiting with code 42'\n");
        fprintf(ps1, "exit 42\n");
        fclose(ps1);
    }
#else

    FILE* sh = fopen("test_program.sh", "w");
    if (sh) {
        fprintf(sh, "#!/bin/bash\n");
        fprintf(sh, "echo \"Test program running on $(uname -s)\"\n");
        fprintf(sh, "sleep 2\n");
        fprintf(sh, "echo \"Exiting with code $1\"\n");
        fprintf(sh, "exit $1\n");
        fclose(sh);
        
        chmod("test_program.sh", 0755); //люблю права на юнихе
    }
    
    FILE* py = fopen("test_python.py", "w");
    if (py) {
        fprintf(py, "#!/usr/bin/env python3\n");
        fprintf(py, "import sys\n");
        fprintf(py, "import time\n");
        fprintf(py, "print('Python test script running')\n");
        fprintf(py, "time.sleep(1)\n");
        fprintf(py, "print('Exiting with code 99')\n");
        fprintf(py, "sys.exit(99)\n");
        fclose(py);
        
        chmod("test_python.py", 0755);
    }
#endif
}

void testLaunchAndWait() {
    std::cout << "\n=== Testing launchAndWait ===\n";
    
#ifdef _WIN32

    std::cout << "Testing batch file...\n";
    int exitCode = BackgroundLauncher::launchAndWait(
        "test_program.bat", 
        {"5"}
    );
    std::cout << "Batch file exited with code: " << exitCode << std::endl;
    
    std::cout << "\nTesting PowerShell script...\n";
    exitCode = BackgroundLauncher::launchAndWait(
        "powershell.exe",
        {"-ExecutionPolicy", "Bypass", "-File", "test_ps.ps1"}
    );
    std::cout << "PowerShell script exited with code: " << exitCode << std::endl;
    
    std::cout << "\nTesting non-existent program...\n";
    exitCode = BackgroundLauncher::launchAndWait("nonexistent.exe");
    std::cout << "Non-existent program result: " << exitCode << " (should be -1)" << std::endl;
#else

    std::cout << "Testing bash script...\n";
    int exitCode = BackgroundLauncher::launchAndWait(
        "./test_program.sh",
        {"7"}
    );
    std::cout << "Bash script exited with code: " << exitCode << std::endl;
    
    std::cout << "\nTesting Python script...\n";
    exitCode = BackgroundLauncher::launchAndWait(
        "./test_python.py",
        {}
    );
    std::cout << "Python script exited with code: " << exitCode << std::endl;
    
    std::cout << "\nTesting system utility (ls)...\n";
    exitCode = BackgroundLauncher::launchAndWait("ls", {"-la"});
    std::cout << "ls command exited with code: " << exitCode << std::endl;
    
    std::cout << "\nTesting non-existent program...\n";
    exitCode = BackgroundLauncher::launchAndWait("./nonexistent.sh");
    std::cout << "Non-existent program result: " << exitCode << " (should be -1)" << std::endl;
#endif
}

void testBackgroundLaunch() {
    std::cout << "\n=== Testing background launch ===\n";
    
#ifdef _WIN32
    bool success1 = BackgroundLauncher::launch("test_program.bat", {"0"});
    bool success2 = BackgroundLauncher::launch("test_program.bat", {"1"});
    bool success3 = BackgroundLauncher::launch("powershell.exe", 
        {"-ExecutionPolicy", "Bypass", "-Command", "Start-Sleep -Seconds 3; Write-Host 'PS done'"});
#else
    bool success1 = BackgroundLauncher::launch("./test_program.sh", {"0"});
    bool success2 = BackgroundLauncher::launch("./test_program.sh", {"1"});
    bool success3 = BackgroundLauncher::launch("sleep", {"3"});
#endif
    
    std::cout << "Launch results: " 
              << (success1 ? "Success" : "Failed") << ", "
              << (success2 ? "Success" : "Failed") << ", "
              << (success3 ? "Success" : "Failed") << std::endl;
    
    std::cout << "Currently running " << BackgroundLauncher::getRunningCount() 
              << " processes" << std::endl;
    
    auto runningPrograms = BackgroundLauncher::getRunningPrograms();
    std::cout << "Running programs:\n";
    for (const auto& prog : runningPrograms) {
        std::cout << "  - " << prog << std::endl;
    }
    
#ifdef _WIN32
    bool isBatRunning = BackgroundLauncher::isProgramRunning("test_program");
    bool isPsRunning = BackgroundLauncher::isProgramRunning("powershell");
    std::cout << "Batch program running: " << (isBatRunning ? "Yes" : "No") << std::endl;
    std::cout << "PowerShell running: " << (isPsRunning ? "Yes" : "No") << std::endl;
#else
    bool isShRunning = BackgroundLauncher::isProgramRunning("test_program");
    bool isSleepRunning = BackgroundLauncher::isProgramRunning("sleep");
    std::cout << "Shell script running: " << (isShRunning ? "Yes" : "No") << std::endl;
    std::cout << "Sleep command running: " << (isSleepRunning ? "Yes" : "No") << std::endl;
#endif
    
    std::cout << "\nWaiting for all processes to complete...\n";
    size_t completed = BackgroundLauncher::waitForAll();
    std::cout << "Completed " << completed << " processes" << std::endl;
    std::cout << "Remaining processes: " << BackgroundLauncher::getRunningCount() << std::endl;
}



int main() {
    std::cout << "=== Cross-Platform Background Launcher Test ===\n";
    
#ifdef _WIN32
    std::cout << "Running on Windows\n";
#else
    std::cout << "Running on UNIX system\n";
#endif
    
    createTestProgram();
    
    testLaunchAndWait();
    testBackgroundLaunch();
    
    std::cout << "\n=== Final check ===\n";
    std::cout << "Processes still running: " << BackgroundLauncher::getRunningCount() << std::endl;
    
#ifdef _WIN32
    system("del test_program.bat test_ps.ps1 2>nul");
#else
    system("rm -f test_program.sh test_python.py");
#endif
    
    std::cout << "\nAll tests completed successfully!\n";
    return 0;
}