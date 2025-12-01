/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 * @author James Noel 101306496
 * @author Colin Calhoun 101307947
 * 
 */

#include "interrupts_101307947_101306496.hpp"

unsigned int total_used_memory() {
    unsigned int used_memory = 0;
    for (const auto &process : memory_paritions) {
        if(process.occupied != -1 ) {
            used_memory += process.size;
        }
    }
    return used_memory;
}

unsigned int total_free_memory() {
    unsigned int free_memory = 0;
    for (const auto &process : memory_paritions) {
        if(process.occupied == -1 ) {
            free_memory += process.size;
        }
    }
    return free_memory;
}

unsigned int total_usable_memory() {
    unsigned int free_memory = 0;
    for (const auto &process : memory_paritions) {
        if(process.occupied != -1 ) {
            free_memory += process.size;
        }
    }
    return free_memory;
}

std::string memory_state_capture(unsigned int current_time, int pid) {
    std::stringstream memory_capture;

    memory_capture << "\n";
    memory_capture << "PID " << pid << " starts running at " << current_time << "\n";    
    memory_capture << "-----------------------------------------\n";

    for (const auto &process : memory_paritions) {
        memory_capture << "Partition: " << process.partition_number
            << ", Size: " << process.size << " ";
        if(process.occupied == -1) {
            memory_capture << ", Availability: Free\n";
        } else {
            memory_capture << ", Availability: used by pid " << process.occupied << "\n";
        }
    }

    memory_capture << "-----------------------------------------------------\n";
    memory_capture << "Total memory used:   " << total_used_memory()    << "\n";
    memory_capture << "Total free memory:   " << total_free_memory()    << "\n";
    memory_capture << "Total usable memory:   " << total_usable_memory()  << "\n";
    memory_capture << "-----------------------------------------------------\n";

    return memory_capture.str();
}



std::tuple<std::string, std::string> run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).
    
    unsigned int current_time = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    const unsigned int QUANTUM = 100;

    std::string execution_status = print_exec_header();
    std::string memory_status;

    // Populate job_list with the given processes
    job_list = list_processes;
    
    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while (!all_process_terminated(job_list)) {

        unsigned int end_time = current_time + 1;

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for (auto &process : list_processes) {
            if (process.arrival_time == current_time && process.state == NOT_ASSIGNED) { //check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                if (assign_memory(process)) {
                    process.state = READY;
                    ready_queue.push_back(process);
                    job_list.push_back(process);
                    execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                } 
                else {
                    process.state = WAITING;
                    wait_queue.push_back(process);
                    job_list.push_back(process);
                    execution_status += print_exec_status(current_time, process.PID, NEW, WAITING);
                }
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue

        for (auto &process : wait_queue) {
            if (process.state != WAITING) continue;

            if (process.io_remaining > 0) {
                process.io_remaining--;
                if (process.io_remaining == 0) {
                    if (assign_memory(process)) {
                        process.state = READY;
                        ready_queue.push_back(process);
                        execution_status += print_exec_status(end_time, process.PID, WAITING, READY);
                    }
                }
                continue;
            }

            if (assign_memory(process)) {
                process.state = READY;
                ready_queue.push_back(process);
                execution_status += print_exec_status(end_time, process.PID, WAITING, READY);
            }
        }

        wait_queue.erase(
            std::remove_if(wait_queue.begin(), wait_queue.end(),
                [](const PCB &p){ return p.state == READY; }),
            wait_queue.end()
        );

        /////////////////////////////////////////////////////////////////
        
        //////////////////////////SCHEDULER/////////////////////////////

        if ((running.state == NOT_ASSIGNED || running.state == TERMINATED) &&
            !ready_queue.empty()) 
        {

            run_process(running, job_list, ready_queue, current_time);
            sync_queue(job_list, running);

            running.quantum_remaining = QUANTUM;

            if (!running.has_run_so_far) {
                execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
                memory_status += memory_state_capture(current_time, running.PID);
                running.has_run_so_far = true;
            }
            else {
                execution_status += print_exec_status(end_time, running.PID, READY, RUNNING);
                memory_status += memory_state_capture(current_time, running.PID);
            }
        }

        if ((running.state == NOT_ASSIGNED || running.state == TERMINATED) &&
            ready_queue.empty())
        {
            idle_CPU(running);
        }


        if (running.state == RUNNING) {

            sync_queue(job_list, running);

            running.quantum_remaining--;
            running.remaining_time--;
            if (running.io_freq > 0) running.io_count++;

            if (running.remaining_time == 0) {
                terminate_process(running, job_list);
                sync_queue(job_list, running);
                execution_status += print_exec_status(end_time, running.PID, RUNNING, TERMINATED);
                idle_CPU(running);
            }

            else if (running.quantum_remaining == 0) {
                running.state = READY;
                ready_queue.push_back(running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);

                if (!ready_queue.empty()) {
                    running = ready_queue.front();
                    ready_queue.erase(ready_queue.begin());

                    running.state = RUNNING;
                    running.quantum_remaining = QUANTUM;

                    execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
                    memory_status += memory_state_capture(current_time, running.PID);
                } else {
                    idle_CPU(running);
                }
            }

            else if (running.io_freq > 0 && running.io_count == running.io_freq &&
                     running.remaining_time > 0)
            {
                running.io_count = 0;
                running.state = WAITING;
                running.io_remaining = running.io_duration;
                wait_queue.push_back(running);
                sync_queue(job_list, running);

                execution_status += print_exec_status(end_time, running.PID, RUNNING, WAITING);
                idle_CPU(running);
            }
        }

        current_time = end_time;

        /////////////////////////////////////////////////////////////////
    }

    execution_status += print_exec_footer();
    return std::make_tuple(execution_status, memory_status);
}


int main(int argc, char** argv) {
    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file(file_name);
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec, memory] = run_simulation(list_process);

    write_output(exec, "execution.txt");
    write_output(memory, "memory_usage.txt");

    return 0;
}