/*
Name: Cassidy Nguyen
Date: 03/01/2024
Course Number: COSC 3360
Professor: Professor Paris
Title: First Assignment
Notes: I did keep some of the code I used for debugging in the program as comments just in case my program is not working as 
expected, so that I would be able to return to the program and see where my code went wrong. The program should only output
a short report/summary in which I followed the format of the provided output texts.
*/

#include <iostream>
#include <queue>
#include <vector>

using namespace std;

// structure for processes
struct Process {
    int PID;
    double arrivalTime;
    string instruction;

    // constructor to keep priority queue in correct order- lowest arrivalTime to highest arrivalTime
    Process(int id, double start, string instr) : PID(id), arrivalTime(start), instruction(instr) {}
    bool operator< (const Process& p) const {
        return arrivalTime > p.arrivalTime;
    }
};

// structure for inputs
struct Input {
    string command;
    double num;
};

// structure for process table
struct pTable {
    int PID;
    int startLine;
    int endLine;
    int currLine;
    string state;
    int logReads = 0;
    int phyWrites = 0;
    int phyReads = 0;
    int bufferSize = 0;
};

// declaring functions (will be called depending on the instruction)- will define these at bottom of file after main
void arrival(Process& process);
void coreRequest(Process& process);
void SSDRequest(Process& process);
void ioRequest(Process& process);
void SSDCompletion(Process& process);
void coreCompletion(Process& process);
void completionEvent(Process& process);
void ioCompletion(Process& process);
int handleBuffer(int needed, int curr);

// global variables
vector<Input> inputs; // stores all the inputs line by line, divided into the command & their time value
vector<pTable> processTable;
priority_queue<Process> mainQueue; // shorter times have higher priority
queue<Process> readyQueue; // FCFS
queue<Process> SSDQueue; // FCFS
bool CPUisEmpty = true;
bool SSDisEmpty = true;
double clockTime = 0;
int BSIZE = -1; // will be set in the first line of input (-1 is a placeholder)
double SSDACCESS = 0.1; // All SSD accesses will take 0.1 ms

int main() {
    // read in the BSIZE
    string temp; // get rid of the "BSIZE " in front of the value
    cin >> temp >> BSIZE;

    // reading the inputs in
    Input newInput;
    while (cin >> newInput. command >> newInput.num) {
        inputs.push_back(newInput);
    }

    // transferring inputs into the process table
    int currPID = 0, startLine = 0;
    for (unsigned int i = 0; i < inputs.size(); ++i) {
        if (inputs[i].command == "START") {
            /// finalizes the end line of the previous package
            if (i > 0) {
                processTable.back().endLine = i - 1;
            }
            pTable newEntry;
            newEntry.PID = currPID++;
            newEntry.startLine = i;
            newEntry.currLine = startLine;
            newEntry.state = "NOT STARTED";
            processTable.push_back(newEntry);
        }
    }
    processTable.back().endLine = inputs.size() - 1; // finalizes the end line of the last process

    // adding the processes to the main queue
    for (unsigned int i = 0; i < processTable.size(); ++i) {
        mainQueue.push(Process(processTable[i].PID, inputs[processTable[i].startLine].num, inputs[processTable[i].startLine].command));
    }

    // main while loop
    while (!mainQueue.empty()) {
        Process top = mainQueue.top();
        mainQueue.pop();
        //cout << inputs[processTable[top.PID].currLine].command << " " << inputs[processTable[top.PID].currLine].num << endl; // for debugging
        clockTime = top.arrivalTime;
        if (inputs[processTable[top.PID].currLine].command == "START") {
            arrival(top);
        }
        else if (inputs[processTable[top.PID].currLine].command == "WRITE" || inputs[processTable[top.PID].currLine].command == "READ") {
            SSDCompletion(top);
        }
        else if (inputs[processTable[top.PID].currLine].command == "INPUT" || inputs[processTable[top.PID].currLine].command == "DISPLAY") {
            ioCompletion(top);
        }
        else {
            coreCompletion(top);
        }
    }
    return 0;
}

void arrival(Process& process) {
    processTable[process.PID].state = "RUNNING";
    //cout << "Process " << process.PID << " starts at time " << clockTime << endl; // for debugging
    processTable[process.PID].currLine = processTable[process.PID].startLine + 1;
    if (inputs[processTable[process.PID].currLine].command == "CORE") {
        coreRequest(process);
    }
}

void coreRequest(Process& process) {
    //cout << "Process " << process.PID << " request the CPU at time " << clockTime << " for " << inputs[processTable[process.PID].currLine].num << endl; // for debugging
    if (CPUisEmpty) {
        CPUisEmpty = false;
        processTable[process.PID].state = "RUNNING";
        double completionTime = clockTime + inputs[processTable[process.PID].currLine].num;
        mainQueue.push(Process(process.PID, completionTime, inputs[processTable[process.PID].currLine].command));
        //cout << "Process " << process.PID << " gets the the CPU at time " << clockTime << endl; // for debugging
        //cout << "Process " << process.PID << " will release the CPU at time " << completionTime << endl; // for debugging
        //cout << "Schedule a core completion event for process " << process.PID << " at time " << completionTime << endl; // for debugging
    }
    else { // CPU is full
        //cout << "Process " << process.PID << " must wait for the CPU" << endl; // for debugging
        processTable[process.PID].state = "READY";
        readyQueue.push(process);
        //cout << "Ready queue now contains " << readyQueue.size() << " process(es) waiting for the CPU" << endl; // for debugging
    }
}

void SSDRequest(Process& process) {
    //cout << "Process " << process.PID << " requests the SSD at time " << clockTime << endl; // for debugging
    if (inputs[processTable[process.PID].currLine].command == "READ") {
        if (processTable[process.PID].bufferSize < inputs[processTable[process.PID].currLine].num) {
            // physical read- must use SSDACCESS
            processTable[process.PID].phyReads++;
            if (SSDisEmpty) {
                processTable[process.PID].state = "RUNNING";
                SSDisEmpty = false;
                processTable[process.PID].bufferSize = handleBuffer(inputs[processTable[process.PID].currLine].num, processTable[process.PID].bufferSize);
                double completionTime = clockTime + SSDACCESS;
                mainQueue.push((Process(process.PID, completionTime, inputs[processTable[process.PID].currLine].command)));
            }
            else { // SSD is full
                processTable[process.PID].state = "BLOCKED";
                //cout << "Process " << process.PID << " must wait for the SSD" << endl; // for debugging
                SSDQueue.push(process);
                //cout << "SSD queue now contains " << SSDQueue.size() << " process(es) waiting for the SSD" << endl; // for debugging
            }
        }
        else { // logical read
            processTable[process.PID].logReads++;
            processTable[process.PID].bufferSize = handleBuffer(inputs[processTable[process.PID].currLine].num, processTable[process.PID].bufferSize);
            processTable[process.PID].state = "RUNNING";
            processTable[process.PID].currLine++;
            if (inputs[processTable[process.PID].currLine].command == "CORE") {
                coreRequest(process);
            }
        }
    }
    else if (inputs[processTable[process.PID].currLine].command == "WRITE") {
        // physical write- must use SSDACCESS
        processTable[process.PID].phyWrites++;
        clockTime = clockTime + SSDACCESS;
        if (SSDisEmpty) {
            processTable[process.PID].state = "RUNNING";
            mainQueue.push(Process(process.PID, clockTime, inputs[processTable[process.PID].currLine].command));
        }
        else { // SSD is full
            processTable[process.PID].state = "BLOCKED";
            SSDQueue.push(process);
        }
    }
}

void ioRequest(Process& process) {
    double completionTime = clockTime + inputs[processTable[process.PID].currLine].num;
    mainQueue.push(Process(process.PID, completionTime, inputs[processTable[process.PID].currLine].command));
}

void SSDCompletion(Process& process) {
    processTable[process.PID].state = "BLOCKED";
    SSDisEmpty = true;
    //cout << "Process " << process.PID << " releases the SSD at time " << clockTime << endl; // for debugging
    if (!SSDQueue.empty()) {
        Process curr = SSDQueue.front();
        SSDQueue.pop();
        //cout << "Process " << curr.PID << " is leaving the SSDQueue" << endl; // for debugging
        SSDRequest(curr);
    }
    else {
        processTable[process.PID].currLine++;
        process.instruction = inputs[processTable[process.PID].currLine].command;
        if (process.instruction == "CORE") {
            coreRequest(process);
        }
    }
}

void coreCompletion(Process& process) {
    CPUisEmpty = true;
    //cout << "Process " << process.PID << " releases the CPU at time " << clockTime << endl; // for debugging
    //cout << readyQueue.size() << endl; // for debugging
    if (!readyQueue.empty()) {
        Process top = readyQueue.front();
        readyQueue.pop();
        //cout << "Process " << top.PID << " is leaving the readyQueue" << endl; // for debugging
        coreRequest(top);
    }
    if (processTable[process.PID].currLine == processTable[process.PID].endLine) { // process is done/no more instructions
        completionEvent(process);
    }
    else {
        processTable[process.PID].currLine++;
        process.instruction = inputs[processTable[process.PID].currLine].command;
        if (process.instruction == "READ" || process.instruction == "WRITE") {
            SSDRequest(process);
        }
        else if (process.instruction == "INPUT" || process.instruction == "DISPLAY") {
            ioRequest(process);
        }
        else {
            double completionTime = clockTime + inputs[processTable[process.PID].currLine].num;
            mainQueue.push(Process(process.PID, completionTime, inputs[processTable[process.PID].currLine].command));
        }
    }
}

void completionEvent(Process& process) {
    CPUisEmpty = true;
    processTable[process.PID].state = "TERMINATED";

    // summary
    cout << "Process " << process.PID << " terminates at time " << clockTime << " ms." << endl;
    cout << "It performed " << processTable[process.PID].phyReads << " physical read(s), " << processTable[process.PID].logReads << " in-memory read(s), and " << processTable[process.PID].phyWrites << " physical write(s)." << endl;
    cout << "Process Table: " << endl;

    // only print the process terminating and other active processes
    for (unsigned int i = 0; i < processTable.size(); i++) { 
        if (i == process.PID || processTable[i].state != "TERMINATED")
            cout << "Process " << processTable[i].PID << " is " << processTable[i].state << "." << endl;
    }
    cout << endl;
}

void ioCompletion(Process& process) {
    processTable[process.PID].currLine++;
    process.instruction = inputs[processTable[process.PID].currLine].command;
    coreRequest(process);
}

int handleBuffer(int needed, int curr) {
    int final;
    if (needed <= curr) {
        final = curr - needed;
    }
    else {
        int missing = needed - curr;
        int brought;
        if (missing % BSIZE == 0) {
            brought = missing;
        }
        else {
            brought = (missing / BSIZE + 1) * BSIZE;
        }
        final = brought - missing;
    }
    return final;
}