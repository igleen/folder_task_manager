# folder_task_manager

this is my attempt to reproduce htop, with some extra features

## usage
gcc main.cpp -lstdc++ -lncurses -O2 -o main

./main

## features:
merged mode [F2]: 
- merges processes with same name, with their rrs (memory) combined   
- shows lowest pid for merged process 
- allows to select merged process, press [space] to see its children
