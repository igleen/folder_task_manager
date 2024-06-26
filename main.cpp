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
    bool isMerged = false;
    std::vector<ProcessInfo> threads;
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


void displayProcesses(const std::vector<ProcessInfo>& processList, int selectedProcess, bool showThreads) {
    clear();
    mvprintw(0, 0, "\tPID\tMemory\tName");
    int row = 1;

    for (int i = 0; i < (int)processList.size(); i++) {
        if (row >= LINES - 1) break; // Check if we have reached the end of the screen

        const auto& process = processList[i];
        std::string truncatedName = process.name.substr(0, 16);

        if (i == selectedProcess) attron(A_REVERSE); // Enable reverse video mode
        mvprintw(row, 0, "%s%s\t%ld\t%s", process.isMerged ? "    [+] " : "\t", process.pid.c_str(), process.rss, truncatedName.c_str());
        if (i == selectedProcess) attroff(A_REVERSE); // Disable reverse video mode

        if (showThreads && i == selectedProcess && !process.threads.empty()) {
            for (const auto& thread : process.threads) {
                row++;
                if (row >= LINES - 1) break; // Check if we have reached the end of the screen
                mvprintw(row, 0, "    |_| %s\t%ld\t%s", thread.pid.c_str(), thread.rss, thread.name.c_str());
            }
        }

        row++;
    }

    refresh(); // Refresh the screen
}


int main() {
    initscr(); // Initialize the ncurses library
    noecho(); // Disable automatic echoing of characters to the screen
    curs_set(0); // Make the cursor invisible
    // cbreak(); // cbreak mode, which allows immediate character input without the need to press enter
    keypad(stdscr, true); // Enable keypad mode, which allows to use function keys

    bool mergedMode = true;
    int selectedProcess = 0; // Index of the selected process
    int clock_ms = 1000;
    bool showThreads = false;

    std::vector<ProcessInfo> processList = getProcessList();

    while (true) {
        if (clock_ms >= 1000) {
            clock_ms = 0;

            processList = getProcessList();

            // Merge processes by name
            std::map<std::string, std::vector<ProcessInfo>> processMap;
            for (const auto& process : processList) {
                std::string shortname = process.name.substr(0, 8);
                processMap[shortname].push_back(process);
            }

            // Calculate total RSS and lowest PID for each merge
            std::vector<ProcessInfo> mergedProcessList;
            for (const auto& entry : processMap) {
                ProcessInfo mergedProcess;
                mergedProcess.name = entry.second[0].name;
                mergedProcess.rss = 0;
                mergedProcess.pid = entry.second[0].pid;
                mergedProcess.isMerged = (entry.second.size() > 1); // Set the merged flag
                for (const auto& process : entry.second) {
                    mergedProcess.rss += process.rss;
                    if (std::stoi(process.pid) < std::stoi(mergedProcess.pid)) {
                        mergedProcess.pid = process.pid;
                    }
                    if (mergedProcess.isMerged) {
                        mergedProcess.threads.push_back(process);
                    }
                }
                mergedProcessList.push_back(mergedProcess);
            }

            if (mergedMode) 
                processList = mergedProcessList;

            // Sort the processes by reserved memory usage in descending order
            std::sort(processList.begin(), processList.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
                return a.rss > b.rss;
            });

            displayProcesses(processList, selectedProcess, showThreads);
        }

        nodelay(stdscr, true); // Disable blocking of getch(), otherwise the program will wait until a key is pressed
        int c = getch();
        nodelay(stdscr, false);

        if (c == KEY_UP && selectedProcess > 0)
            selectedProcess--;
        if (c == KEY_DOWN && selectedProcess < LINES - 3) // -3 because of the header and footer
            selectedProcess++;
        if (c == KEY_UP || c == KEY_DOWN)
            showThreads = false;
        if (c == KEY_F(2)) 
            mergedMode = !mergedMode;
        if (c == ' ' && processList[selectedProcess].isMerged)
            showThreads = !showThreads;

        // Refresh if any key pressed
        if (c != ERR) displayProcesses(processList, selectedProcess, showThreads);

        clock_ms += 10;
        napms(10);
    }

    endwin(); // End curses mode
    return 0;
}
