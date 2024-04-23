#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ncurses.h>
#include <unistd.h>
#include <dirent.h>
#include <algorithm>
#include <map>

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
    initscr(); // Initialize the ncurses library
    noecho(); // Disable automatic echoing of characters to the screen
    curs_set(0); // Make the cursor invisible
    // cbreak(); // cbreak mode, which allows immediate character input without the need to press enter
    keypad(stdscr, true); // Enable keypad mode, which allows to use function keys

    bool mergedMode = true;
    int selectedProcess = 0; // Index of the selected process

    while (true) {
        clear();
        std::vector<ProcessInfo> processList = getProcessList();

        // Merge processes by name
        std::map<std::string, std::vector<ProcessInfo>> processMap;
        for (const auto& process : processList) {
            processMap[process.name].push_back(process);
        }

        // Calculate total RSS and lowest PID for each merge
        std::vector<ProcessInfo> mergedProcessList;
        for (const auto& entry : processMap) {
            ProcessInfo mergedProcess;
            mergedProcess.name = entry.first;
            mergedProcess.rss = 0;
            mergedProcess.pid = entry.second[0].pid;
            for (const auto& process : entry.second) {
                mergedProcess.rss += process.rss;
                if (std::stoi(process.pid) < std::stoi(mergedProcess.pid)) {
                    mergedProcess.pid = process.pid;
                }
            }
            mergedProcessList.push_back(mergedProcess);
        }

        // Toggle merged and unmerged view
        nodelay(stdscr, true); // Disable blocking of getch(), otherwise the program will wait until a key is pressed
        int c = getch();
        nodelay(stdscr, false);
        if (c == KEY_F(2)) // F2
            mergedMode = !mergedMode;
        else if (c == KEY_UP && selectedProcess > 0)
            selectedProcess--;
        else if (c == KEY_DOWN && selectedProcess < (int)processList.size() - 1)
            selectedProcess++;

        if (mergedMode) 
            processList = mergedProcessList;

        // Sort the processes by reserved memory usage in descending order
        std::sort(processList.begin(), processList.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
            return a.rss > b.rss;
        });

        // Display the processes using ncurses library
        mvprintw(0, 0, "PID\tMemory\tName");
        int row = 1;

        // for each process in the list
        for (int i = 0; i < (int)processList.size(); i++) {
            // Check if we have reached the end of the screen
            if (row >= LINES - 1) { break; }

            const auto& process = processList[i];
            std::string truncatedName = process.name.substr(0, 16);

            if (i == selectedProcess) attron(A_REVERSE); // Enable reverse video mode
            // print process line
            mvprintw(row, 0, "%s\t%ld\t%s", process.pid.c_str(), process.rss, truncatedName.c_str());
            if (i == selectedProcess) attroff(A_REVERSE); // Disable reverse video mode
            
            row++;
        }

        refresh(); // Refresh the screen
        napms(1000); // 1 second delay
    }

    endwin(); // End curses mode
    return 0;
}
