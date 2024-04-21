#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <dirent.h>
#include <algorithm>
#include <sys/ioctl.h>

struct ProcessInfo {
    std::string pid;
    std::string name;
    std::string state;
    long rss; // Resident set size (reserved memory)
};

std::vector<ProcessInfo> getProcessList() {
    std::vector<ProcessInfo> processList;
    DIR* dir = opendir("/proc");
    if (dir == nullptr) {
        std::cerr << "Failed to open /proc directory." << std::endl;
        return processList;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR && std::isdigit(entry->d_name[0])) {
            std::string pid = entry->d_name;
            std::ifstream statusFile("/proc/" + pid + "/status");
            if (!statusFile) {
                continue;
            }

            std::string line;
            ProcessInfo process;
            process.pid = pid;
            while (std::getline(statusFile, line)) {
                std::istringstream iss(line);
                std::string key, value;
                if (std::getline(iss, key, ':') && std::getline(iss >> std::ws, value)) {
                    if (key == "Name") {
                        process.name = value;
                    } else if (key == "State") {
                        process.state = value;
                    } else if (key == "RssAnon") {
                        process.rss = std::stol(value.substr(0, value.find(" ")));
                    }
                }
            }
            processList.push_back(process);
        }
    }

    closedir(dir);
    return processList;
}

int main() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int screenHeight = w.ws_row - 2; // Subtract 2 for the header and prompt lines

    while (true) {
        std::system("clear");
        std::vector<ProcessInfo> processList = getProcessList();

        // Sort the processes by reserved memory usage in descending order
        std::sort(processList.begin(), processList.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
            return a.rss > b.rss;
        });

        std::cout << "PID\tMemory\tState\t\tName" << std::endl;
        int count = 0;
        for (const auto& process : processList) {
            if (count >= screenHeight) {
                break;
            }
            std::string truncatedName = process.name.substr(0, 16);
            std::cout << process.pid << "\t" << process.rss << "\t"
                      << process.state << "\t" << truncatedName << std::endl;
            count++;
        }
        sleep(1);
    }

    return 0;
}
