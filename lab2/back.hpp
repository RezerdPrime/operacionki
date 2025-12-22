#ifndef BACKGROUND_LAUNCHER_HPP
#define BACKGROUND_LAUNCHER_HPP

#include <string>
#include <vector>
#include <memory>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstring>
#include <errno.h>
#endif


// ну поехали
class BackgroundLauncher {
private:
    BackgroundLauncher() = delete;
    
    struct ProcessInfo {
#ifdef _WIN32
        HANDLE handle;
        DWORD pid;
#else
        pid_t pid;
#endif
        std::string program;
    };
    
    static std::vector<ProcessInfo> processes;
    
    static std::string buildCommandLine(const std::string& program, const std::vector<std::string>& args) {
        std::string commandLine;
        
#ifdef _WIN32
        if (program.find(' ') != std::string::npos) {
            commandLine = "\"" + program + "\"";
        } else {
            commandLine = program;
        }
#else
        commandLine = program;
#endif
        
        for (const auto& arg : args) {
            commandLine += " ";
#ifdef _WIN32
            if (arg.find(' ') != std::string::npos) {
                commandLine += "\"" + arg + "\"";
            } else {
                commandLine += arg;
            }
#else
            std::string escapedArg;
            for (char c : arg) {
                if (c == '\'' || c == '"' || c == '\\' || c == ' ') {
                    escapedArg += '\\';
                }
                escapedArg += c;
            }
            commandLine += escapedArg;
#endif
        }
        
        return commandLine;
    }

#ifdef _WIN32
    static ProcessInfo launchWindows(const std::string& program, const std::vector<std::string>& args) {
        std::string commandLine = buildCommandLine(program, args);
        
        STARTUPINFOA startupInfo;
        PROCESS_INFORMATION processInfo;
        
        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        ZeroMemory(&processInfo, sizeof(processInfo));
        
        ProcessInfo procInfo;
        procInfo.program = program;
        procInfo.handle = NULL;
        procInfo.pid = 0;
        
        if (CreateProcessA(
            NULL,
            const_cast<LPSTR>(commandLine.c_str()),
            NULL,
            NULL,
            FALSE,
            CREATE_NO_WINDOW,
            NULL,
            NULL,
            &startupInfo,
            &processInfo
        )) {
            CloseHandle(processInfo.hThread);
            procInfo.handle = processInfo.hProcess;
            procInfo.pid = processInfo.dwProcessId;
            return procInfo;
        }
        
        std::cerr << "Failed to create process: " << GetLastError() << std::endl;
        return procInfo;
    }
#else

    static ProcessInfo launchUnix(const std::string& program, const std::vector<std::string>& args) {
        pid_t pid = fork();
        
        ProcessInfo procInfo;
        procInfo.program = program;
        procInfo.pid = -1;
        
        if (pid == 0) {

            std::vector<char*> argv;
            argv.push_back(const_cast<char*>(program.c_str()));
            
            for (const auto& arg : args) {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            argv.push_back(nullptr);
            
            if (execvp(program.c_str(), argv.data()) == -1) {
                std::cerr << "execvp failed: " << strerror(errno) << std::endl;
                exit(EXIT_FAILURE);
            }
        } else if (pid > 0) {
            procInfo.pid = pid;
            return procInfo;
        } else {
            std::cerr << "fork failed: " << strerror(errno) << std::endl;
        }
        
        return procInfo;
    }
#endif

public:

    static bool launch(const std::string& program, const std::vector<std::string>& args = {}) {
#ifdef _WIN32
        ProcessInfo procInfo = launchWindows(program, args);
        if (procInfo.handle != NULL) {
            processes.push_back(procInfo);
            return true;
        }
#else
        ProcessInfo procInfo = launchUnix(program, args);
        if (procInfo.pid > 0) {
            processes.push_back(procInfo);
            return true;
        }
#endif
        return false;
    }
    
    static int launchAndWait(const std::string& program, const std::vector<std::string>& args = {}) {
#ifdef _WIN32
        ProcessInfo procInfo = launchWindows(program, args);
        if (procInfo.handle == NULL) {
            return -1;
        }
        
        DWORD exitCode;
        WaitForSingleObject(procInfo.handle, INFINITE);
        GetExitCodeProcess(procInfo.handle, &exitCode);
        CloseHandle(procInfo.handle);
        
        return static_cast<int>(exitCode);
#else
        ProcessInfo procInfo = launchUnix(program, args);
        if (procInfo.pid < 0) {
            return -1;
        }
        
        int status;
        waitpid(procInfo.pid, &status, 0);
        
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            std::cerr << "Process terminated by signal: " << WTERMSIG(status) << std::endl;
            return -1;
        } else {
            return -1;
        }
#endif
    }
    
    static size_t waitForAll() {
        size_t count = 0; //колво завершенных процессов
        
#ifdef _WIN32
        for (auto& procInfo : processes) {
            if (procInfo.handle != NULL) {
                WaitForSingleObject(procInfo.handle, INFINITE);
                CloseHandle(procInfo.handle);
                procInfo.handle = NULL;
                count++;
            }
        }
#else
        for (auto& procInfo : processes) {
            if (procInfo.pid > 0) {
                int status;
                waitpid(procInfo.pid, &status, 0);
                procInfo.pid = -1;
                count++;
            }
        }
#endif
        
        processes.clear();
        return count;
    }
    
    static size_t getRunningCount() { //колво запущенных процессов
        return processes.size();
    }
    
    static std::vector<std::string> getRunningPrograms() { //инфа о запущенных процессах
        std::vector<std::string> result;
        for (const auto& procInfo : processes) {
            result.push_back(procInfo.program);
        }
        return result;
    }
    
    static bool isProgramRunning(const std::string& program) {
        for (const auto& procInfo : processes) {
            if (procInfo.program.find(program) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
};

std::vector<BackgroundLauncher::ProcessInfo> BackgroundLauncher::processes;

#endif // BACKGROUND_LAUNCHER_HPP