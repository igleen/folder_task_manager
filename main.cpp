#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ncurses.h>
#include <unistd.h>
#include <dirent.h>
#include <algorithm>

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
    initscr();
    cbreak();
    noecho();
    curs_set(0);

    while (true) {
        clear();
        std::vector<ProcessInfo> processList = getProcessList();

        // Sort the processes by reserved memory usage in descending order
        std::sort(processList.begin(), processList.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
            return a.rss > b.rss;
        });
        mvprintw(0, 0, "PID\tMemory\tS Name");
        int row = 2;
        for (const auto& process : processList) {
            if (row >= LINES - 1) {
                break;
            }
            std::string truncatedName = process.name.substr(0, 16);
            std::string state = process.state.substr(0, 1); // Get the first character of the state
            mvprintw(row, 0, "%s\t%ld\t%s %s", process.pid.c_str(), process.rss, state.c_str(), truncatedName.c_str());
            row++;
        }

        refresh();
        napms(1000);
    }

    endwin();
    return 0;
}
