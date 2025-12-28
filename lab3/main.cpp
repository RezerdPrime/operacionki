#include "shmem.hpp" // либы из code examples
#include "mutex.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <cstring>

#if defined(_WIN32)
    #include <windows.h>
    #include <process.h>
    #include <tlhelp32.h>
    #define getpid _getpid
    #define pid_t DWORD
    #define SIGTERM 15
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <signal.h>
    #include <errno.h>
#endif

struct SharedData {
    int counter;
    bool is_master;
    pid_t master_pid;
    time_t last_fork_time;
    pid_t child1_pid;
    pid_t child2_pid;
    bool child1_running;
    bool child2_running;
};

cplib::SharedMem<SharedData>* g_shared_mem = nullptr;
std::ofstream g_log_file;
std::string g_log_filename = "counter_app.log";
std::atomic<bool> g_running(true);
std::atomic<bool> g_is_master(false);
std::atomic<bool> g_is_child(false);
std::atomic<int> g_child_type(0); // 0 = master, 1 = child1, 2 = child2
cplib::Mutex g_log_mutex;

std::string get_current_time_string(bool with_ms = false) {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm_buf;
#if defined(_WIN32)
    localtime_s(&tm_buf, &now_time_t);
#else
    localtime_r(&now_time_t, &tm_buf);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    if (with_ms) {
        oss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
    }
    return oss.str();
}

void log_message(const std::string& message) {
    cplib::AutoMutex lock(g_log_mutex);
    
    if (g_log_file.is_open()) {
        g_log_file << get_current_time_string(true) << " [PID: " << getpid();
        if (g_is_child) {
            g_log_file << " Child" << g_child_type;
        }
        g_log_file << "] " << message << std::endl;
        g_log_file.flush();
    }
    
    std::cout << get_current_time_string(true) << " [PID: " << getpid();
    if (g_is_child) {
        std::cout << " Child" << g_child_type;
    }
    std::cout << "] " << message << std::endl;
}

void run_child1() {
    g_is_child = true;
    g_child_type = 1;
    
    std::ofstream child_log_file("counter_app.log", std::ios::app);
    if (!child_log_file.is_open()) {
        std::cerr << "Child1: Failed to open log file" << std::endl;
        return;
    }
    
    child_log_file << get_current_time_string(true) << " [PID: " << getpid() 
                   << " Child1] started" << std::endl;
    child_log_file.flush();
    std::cout << get_current_time_string(true) << " [PID: " << getpid() 
              << " Child1] started" << std::endl;
    
    cplib::SharedMem<SharedData> local_shared_mem("counter_app_shared");
    if (!local_shared_mem.IsValid()) {
        child_log_file << get_current_time_string(true) << " [PID: " << getpid() 
                       << " Child1] Failed to open shared memory" << std::endl;
        child_log_file.flush();
        return;
    }
    
    local_shared_mem.Lock();
    SharedData* data = local_shared_mem.Data();
    if (data) {
        data->counter += 10;
        child_log_file << get_current_time_string(true) << " [PID: " << getpid() 
                       << " Child1] Added 10 to counter. New value: " << data->counter << std::endl;
        child_log_file.flush();
        std::cout << get_current_time_string(true) << " [PID: " << getpid() 
                  << " Child1] Added 10 to counter. New value: " << data->counter << std::endl;
    }
    local_shared_mem.Unlock();
    
#if defined(_WIN32)
    if (g_shared_mem && g_shared_mem->IsValid()) {
        g_shared_mem->Lock();
        SharedData* master_data = g_shared_mem->Data();
        if (master_data) {
            master_data->child1_running = false;
        }
        g_shared_mem->Unlock();
    }
#endif
    
    child_log_file << get_current_time_string(true) << " [PID: " << getpid() 
                   << " Child1] exited" << std::endl;
    child_log_file.flush();
    std::cout << get_current_time_string(true) << " [PID: " << getpid() 
              << " Child1] exited" << std::endl;
    
    child_log_file.close();
}

void run_child2() {
    g_is_child = true;
    g_child_type = 2;
    
    std::ofstream child_log_file("counter_app.log", std::ios::app);
    if (!child_log_file.is_open()) {
        std::cerr << "Child2: Failed to open log file" << std::endl;
        return;
    }
    
    child_log_file << get_current_time_string(true) << " [PID: " << getpid() 
                   << " Child2] started" << std::endl;
    child_log_file.flush();
    std::cout << get_current_time_string(true) << " [PID: " << getpid() 
              << " Child2] started" << std::endl;
    
    cplib::SharedMem<SharedData> local_shared_mem("counter_app_shared");
    if (!local_shared_mem.IsValid()) {
        child_log_file << get_current_time_string(true) << " [PID: " << getpid() 
                       << " Child2] Failed to open shared memory" << std::endl;
        child_log_file.flush();
        return;
    }
    
    local_shared_mem.Lock();
    SharedData* data = local_shared_mem.Data();
    //int original_value = 0;
    if (data) {
        //original_value = data->counter;
        data->counter *= 2;
        child_log_file << get_current_time_string(true) << " [PID: " << getpid() 
                       << " Child2] Multiplied counter by 2. New value: " << data->counter << std::endl;
        child_log_file.flush();
        std::cout << get_current_time_string(true) << " [PID: " << getpid() 
                  << " Child2] Multiplied counter by 2. New value: " << data->counter << std::endl;
    }
    local_shared_mem.Unlock();
    
#if defined(_WIN32)
    Sleep(2000);
#else
    sleep(2);
#endif
    
    local_shared_mem.Lock();
    data = local_shared_mem.Data();
    if (data) {
        //data->counter = original_value;
        data->counter /= 2;
        child_log_file << get_current_time_string(true) << " [PID: " << getpid() 
                       << " Child2] Divided counter by 2. Restored value: " << data->counter << std::endl;
        child_log_file.flush();
        std::cout << get_current_time_string(true) << " [PID: " << getpid() 
                  << " Child2] Divided counter by 2. Restored value: " << data->counter << std::endl;
    }
    local_shared_mem.Unlock();
    
#if defined(_WIN32)
    if (g_shared_mem && g_shared_mem->IsValid()) {
        g_shared_mem->Lock();
        SharedData* master_data = g_shared_mem->Data();
        if (master_data) {
            master_data->child2_running = false;
        }
        g_shared_mem->Unlock();
    }
#endif
    
    child_log_file << get_current_time_string(true) << " [PID: " << getpid() 
                   << " Child2] exited" << std::endl;
    child_log_file.flush();
    std::cout << get_current_time_string(true) << " [PID: " << getpid() 
              << " Child2] exited" << std::endl;
    
    child_log_file.close();
}

class TimerThread : public cplib::Thread {
private:
    cplib::SharedMem<SharedData>* m_shared_mem;
    bool m_is_master;
    
protected:
    virtual void Main() override {
        while (true) {
            cplib::Thread::Sleep(0.3);
            
            CancelPoint();
            
            if (!g_running) break;
            
            if (m_shared_mem && m_shared_mem->IsValid()) {
                m_shared_mem->Lock();
                SharedData* data = m_shared_mem->Data();
                if (data) {
                    data->counter++;
                }
                m_shared_mem->Unlock();
            }
        }
    }
    
    virtual int MainStart() override { return 0; }
    virtual void MainQuit() override {}
    
public:
    TimerThread(cplib::SharedMem<SharedData>* shared_mem, bool is_master) 
        : m_shared_mem(shared_mem), m_is_master(is_master) {}
};

class LogThread : public cplib::Thread {
private:
    cplib::SharedMem<SharedData>* m_shared_mem;
    std::ofstream* m_log_file;
    cplib::Mutex* m_log_mutex;
    
protected:
    virtual void Main() override {
        while (true) {
            cplib::Thread::Sleep(1.0);
            
            CancelPoint();
            
            if (!g_running) break;
            
            if (m_shared_mem && m_shared_mem->IsValid()) {
                m_shared_mem->Lock();
                SharedData* data = m_shared_mem->Data();
                if (data) {
                    cplib::AutoMutex lock(*m_log_mutex);
                    if (m_log_file->is_open()) {
                        *m_log_file << get_current_time_string(true) << " [PID: " << getpid() 
                                   << " Master] Counter = " << data->counter << std::endl;
                        m_log_file->flush();
                    }
                    std::cout << get_current_time_string(true) << " [PID: " << getpid() 
                             << " Master] Counter = " << data->counter << std::endl;
                }
                m_shared_mem->Unlock();
            }
        }
    }
    
    virtual int MainStart() override { return 0; }
    virtual void MainQuit() override {}
    
public:
    LogThread(cplib::SharedMem<SharedData>* shared_mem, std::ofstream* log_file, cplib::Mutex* log_mutex) 
        : m_shared_mem(shared_mem), m_log_file(log_file), m_log_mutex(log_mutex) {}
};

class ForkThread : public cplib::Thread {
private:
    cplib::SharedMem<SharedData>* m_shared_mem;
    std::ofstream* m_log_file;
    cplib::Mutex* m_log_mutex;
    
    void log_message(const std::string& message) {
        cplib::AutoMutex lock(*m_log_mutex);
        if (m_log_file->is_open()) {
            *m_log_file << get_current_time_string(true) << " [PID: " << getpid() 
                       << " Master] " << message << std::endl;
            m_log_file->flush();
        }
        std::cout << get_current_time_string(true) << " [PID: " << getpid() 
                 << " Master] " << message << std::endl;
    }
    
    bool launch_child_process(int child_type) {
#if defined(_WIN32)

        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        
        char cmd_line[MAX_PATH + 50];
        char module_name[MAX_PATH];
        GetModuleFileName(NULL, module_name, MAX_PATH);
        
        if (child_type == 1) {
            sprintf_s(cmd_line, sizeof(cmd_line), "\"%s\" child1", module_name);
        } else {
            sprintf_s(cmd_line, sizeof(cmd_line), "\"%s\" child2", module_name);
        }
        
        if (!CreateProcess(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            DWORD error = GetLastError();
            log_message("Failed to create process. Error: " + std::to_string(error));
            return false;
        }
        
        DWORD child_pid = pi.dwProcessId;
        
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        
        return true;
#else

        pid_t pid = fork();
        if (pid == 0) {

            if (child_type == 1) {
                run_child1();
            } else {
                run_child2();
            }
            exit(0);
        } else if (pid > 0) {
            return true;
        }
        return false;
#endif
    }
    
protected:
    virtual void Main() override {
        while (true) {
            cplib::Thread::Sleep(3.0);
            
            CancelPoint();
            
            if (!g_running) break;
            
            if (m_shared_mem && m_shared_mem->IsValid()) {
                m_shared_mem->Lock();
                SharedData* data = m_shared_mem->Data();
                
                if (data) {
                    // Проверяем, завершились ли предыдущие копии
#if defined(_WIN32)

                    bool can_fork = true;
                    
                    if (data->child1_pid != 0) {
                        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, data->child1_pid);
                        if (process) {
                            DWORD exit_code;
                            if (GetExitCodeProcess(process, &exit_code)) {
                                if (exit_code == STILL_ACTIVE) {
                                    can_fork = false;
                                    log_message("Skipping fork: Child1 still running (PID: " + 
                                               std::to_string(data->child1_pid) + ")");
                                } else {
                                    data->child1_pid = 0;
                                    data->child1_running = false;
                                }
                            }
                            CloseHandle(process);
                        } else {
                            data->child1_pid = 0;
                            data->child1_running = false;
                        }
                    }
                    
                    if (data->child2_pid != 0) {
                        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, data->child2_pid);
                        if (process) {
                            DWORD exit_code;
                            if (GetExitCodeProcess(process, &exit_code)) {
                                if (exit_code == STILL_ACTIVE) {
                                    can_fork = false;
                                    log_message("Skipping fork: Child2 still running (PID: " + 
                                               std::to_string(data->child2_pid) + ")");
                                } else {
                                    data->child2_pid = 0;
                                    data->child2_running = false;
                                }
                            }
                            CloseHandle(process);
                        } else {
                            data->child2_pid = 0;
                            data->child2_running = false;
                        }
                    }
#else

                    bool can_fork = true;
                    
                    if (data->child1_pid != 0) {
                        int status;
                        pid_t result = waitpid(data->child1_pid, &status, WNOHANG);
                        if (result == 0) {
                            can_fork = false;
                            log_message("Skipping fork: Child1 still running (PID: " + 
                                       std::to_string(data->child1_pid) + ")");
                        } else if (result > 0) {
                            data->child1_pid = 0;
                            data->child1_running = false;
                        }
                    }
                    
                    if (data->child2_pid != 0) {
                        int status;
                        pid_t result = waitpid(data->child2_pid, &status, WNOHANG);
                        if (result == 0) {
                            can_fork = false;
                            log_message("Skipping fork: Child2 still running (PID: " + 
                                       std::to_string(data->child2_pid) + ")");
                        } else if (result > 0) {
                            data->child2_pid = 0;
                            data->child2_running = false;
                        }
                    }
#endif
                    
                    if (can_fork) {
#if defined(_WIN32)

                        if (launch_child_process(1)) {
                            data->child1_running = true;
                            log_message("Launched Child1");
                        }
                        
                        m_shared_mem->Unlock();
                        cplib::Thread::Sleep(0.1);
                        m_shared_mem->Lock();
                        
                        if (launch_child_process(2)) {
                            data->child2_running = true;
                            log_message("Launched Child2");
                        }
#else

                        pid_t pid1 = fork();
                        if (pid1 == 0) {
                            run_child1();
                            exit(0);
                        } else if (pid1 > 0) {
                            data->child1_pid = pid1;
                            data->child1_running = true;
                            log_message("Launched Child1 (PID: " + std::to_string(pid1) + ")");
                        }
                        
                        pid_t pid2 = fork();
                        if (pid2 == 0) {
                            run_child2();
                            exit(0);
                        } else if (pid2 > 0) {
                            data->child2_pid = pid2;
                            data->child2_running = true;
                            log_message("Launched Child2 (PID: " + std::to_string(pid2) + ")");
                        }
#endif
                        
                        data->last_fork_time = time(nullptr);
                    }
                }
                m_shared_mem->Unlock();
            }
        }
    }
    
    virtual int MainStart() override { return 0; }
    virtual void MainQuit() override {}
    
public:
    ForkThread(cplib::SharedMem<SharedData>* shared_mem, std::ofstream* log_file, cplib::Mutex* log_mutex) 
        : m_shared_mem(shared_mem), m_log_file(log_file), m_log_mutex(log_mutex) {}
};

bool is_process_alive(pid_t pid) {
#if defined(_WIN32)
    if (pid == 0) return false;
    
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (process == NULL) {
        return false;
    }
    
    DWORD exit_code;
    if (GetExitCodeProcess(process, &exit_code)) {
        CloseHandle(process);
        return (exit_code == STILL_ACTIVE);
    }
    
    CloseHandle(process);
    return false;
#else
    return (kill(pid, 0) == 0);
#endif
}

void handle_user_input() {
    std::cout << "\n=== Counter Application ===" << std::endl;
    std::cout << "PID: " << getpid() << std::endl;
    std::cout << "Status: " << (g_is_master ? "MASTER" : "SLAVE") << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  set <value>  - Set counter value" << std::endl;
    std::cout << "  get          - Get current counter value" << std::endl;
    std::cout << "  exit         - Exit application" << std::endl;
    std::cout << "  help         - Show this help" << std::endl;
    std::cout << "================================\n" << std::endl;
    
    while (g_running) {
        std::cout << "> ";
        std::string command;
        if (!std::getline(std::cin, command)) {
            break;
        }
        
        if (command == "exit" || command == "quit") {
            g_running = false;
            break;
        } else if (command == "get") {
            if (g_shared_mem && g_shared_mem->IsValid()) {
                g_shared_mem->Lock();
                SharedData* data = g_shared_mem->Data();
                if (data) {
                    std::cout << "Current counter value: " << data->counter << std::endl;
                }
                g_shared_mem->Unlock();
            } else {
                std::cout << "Shared memory not available" << std::endl;
            }
        } else if (command.substr(0, 4) == "set ") {
            try {
                int value = std::stoi(command.substr(4));
                if (g_shared_mem && g_shared_mem->IsValid()) {
                    g_shared_mem->Lock();
                    SharedData* data = g_shared_mem->Data();
                    if (data) {
                        data->counter = value;
                        std::cout << "Counter set to: " << value << std::endl;
                        log_message("User set counter to " + std::to_string(value));
                    }
                    g_shared_mem->Unlock();
                }
            } catch (const std::exception& e) {
                std::cout << "Invalid value: " << e.what() << std::endl;
            }
        
        } else if (command == "help") {
            std::cout << "\nCommands:" << std::endl;
            std::cout << "  set <value>  - Set counter value" << std::endl;
            std::cout << "  get          - Get current counter value" << std::endl;
            std::cout << "  exit         - Exit application" << std::endl;
            std::cout << "  help         - Show this help" << std::endl;

        } else if (!command.empty()) {
            std::cout << "Unknown command. Type 'help' for help." << std::endl;
        }
    }
}


int main(int argc, char* argv[]) {

    if (argc > 1) { // дочерний ли проц
        std::string arg = argv[1];
        if (arg == "child1") {
            run_child1();
            return 0;
        } else if (arg == "child2") {
            run_child2();
            return 0;
        }
    }
    
    g_log_file.open(g_log_filename, std::ios::app);
    if (!g_log_file.is_open()) {
        std::cerr << "Failed to open log file: " << g_log_filename << std::endl;
        return 1;
    }
    
    log_message("Application started");
    
    try {
        g_shared_mem = new cplib::SharedMem<SharedData>("counter_app_shared", true);
    } catch (...) {
        std::cerr << "Failed to create/open shared memory" << std::endl;
        return 1;
    }
    
    if (!g_shared_mem->IsValid()) {
        std::cerr << "Shared memory is not valid" << std::endl;
        delete g_shared_mem;
        g_shared_mem = nullptr;
        return 1;
    }
    
    g_shared_mem->Lock();
    SharedData* shared_data = g_shared_mem->Data();
    
    bool master_exists = false;
    
    if (shared_data->counter == 0 && !shared_data->is_master) {
        shared_data->counter = 0;
        shared_data->is_master = true;
        shared_data->master_pid = getpid();
        shared_data->last_fork_time = 0;
        shared_data->child1_pid = 0;
        shared_data->child2_pid = 0;
        shared_data->child1_running = false;
        shared_data->child2_running = false;
        g_is_master = true;
        log_message("This process is MASTER (initialized shared memory)");

    } else if (shared_data->is_master) {
        bool master_alive = false;
        
        if (shared_data->master_pid == getpid()) {

            master_alive = true;
        } else {
            master_alive = is_process_alive(shared_data->master_pid);
        }
        
        if (master_alive) {
            master_exists = true;
            log_message("Master already exists (PID: " + std::to_string(shared_data->master_pid) + 
                       "). This process is SLAVE");
        } else {
            // Мастер умер F
            shared_data->is_master = true;
            shared_data->master_pid = getpid();
            shared_data->child1_pid = 0;
            shared_data->child2_pid = 0;
            shared_data->child1_running = false;
            shared_data->child2_running = false;
            g_is_master = true;
            log_message("Master died. This process is now MASTER");
        }
    } else {
        shared_data->is_master = true;
        shared_data->master_pid = getpid();
        g_is_master = true;
        log_message("No master found. This process is now MASTER");
    }
    
    g_shared_mem->Unlock();
    
    if (g_is_master) {

        TimerThread* timer_thread = new TimerThread(g_shared_mem, g_is_master);
        LogThread* log_thread = new LogThread(g_shared_mem, &g_log_file, &g_log_mutex);
        ForkThread* fork_thread = new ForkThread(g_shared_mem, &g_log_file, &g_log_mutex);
        
        timer_thread->Start();
        log_thread->Start();
        fork_thread->Start();
        
        timer_thread->WaitStartup();
        log_thread->WaitStartup();
        fork_thread->WaitStartup();
        
        handle_user_input();
        
        g_running = false;
        timer_thread->Stop();
        log_thread->Stop();
        fork_thread->Stop();
        
        timer_thread->Join(1.0);
        log_thread->Join(1.0);
        fork_thread->Join(1.0);
        
        delete timer_thread;
        delete log_thread;
        delete fork_thread;

    } else {

        TimerThread* timer_thread = new TimerThread(g_shared_mem, g_is_master);
        
        timer_thread->Start();
        timer_thread->WaitStartup();
        
        handle_user_input();
        
        g_running = false;
        timer_thread->Stop();
        timer_thread->Join(1.0);
        
        delete timer_thread;
    }
    
    if (g_is_master && g_shared_mem && g_shared_mem->IsValid()) {
        g_shared_mem->Lock();
        SharedData* data = g_shared_mem->Data();
        if (data && data->master_pid == getpid()) {
            data->is_master = false;
            data->master_pid = 0;
            log_message("Master shutting down");
        }
        g_shared_mem->Unlock();
    }
    
    log_message("Application stopped");
    g_log_file.close();
    
    if (g_shared_mem) {
        delete g_shared_mem;
        g_shared_mem = nullptr;
    }
    
    return 0;
}