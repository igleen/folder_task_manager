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
    std::vector<ProcessInfo> children;
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

int selected_row = 1;
int first_process_row = 100;

void printProcessList(std::vector<ProcessInfo> processList) {
    mvprintw(0, 0, "PID\tRSS\tName");
    for (int row = 1; row < LINES; row++) {
        const ProcessInfo& process = processList[row-1+first_process_row];
        if (row == selected_row) attron(A_REVERSE);
        mvprintw(row, 0, "%s\t%ld\t%s", 
                process.pid.c_str(), process.rss, process.name.c_str());
        if (row == selected_row) attroff(A_REVERSE);
    }
    refresh();   
}

int main() {
    initscr(); // Initialize the ncurses library
    noecho(); // Disable automatic echoing of characters to the screen
    curs_set(0); // Make the cursor invisible
    keypad(stdscr, true); // Enable keypad mode, which allows to use keys
    nodelay(stdscr, true); // Disable blocking of getch()
    // otherwise the program will wait until a key is pressed

    std::vector<ProcessInfo> processList = getProcessList();

    int clock_ms = 1000;
    while (true) {
        if (clock_ms >= 1000) {
            clock_ms = 0;

            processList = getProcessList();

            // Sort the process list by reserved memory
            std::sort(processList.begin(), processList.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
                return a.rss > b.rss;
            });

            // Print the process list
            clear();
            printProcessList(processList);
        }

    
        int c = getch();
        if (c == KEY_UP)
            if (selected_row >= 2)
                selected_row--;
            else if (first_process_row > 0)
                first_process_row--;
        if (c == KEY_DOWN)
            if (selected_row < LINES - 1)
                selected_row++;
            else if (first_process_row < processList.size()-LINES)
                first_process_row++;

        if (c != ERR) printProcessList(processList);

        clock_ms += 10;
        napms(10);
    }
}