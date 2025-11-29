/**
 * 
 */

#include "interrupts_101307947_101306496.hpp"

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

void priority(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    if (first.PID == second.PID) {
                        return (first.arrival_time < second.arrival_time);
                    }
                    return (first.PID < second.PID);  
                } 
            );
}



std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

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


    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    // Populate job_list with the given processes
    job_list = list_processes;

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) && !job_list.empty()) {
        
        

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time && process.state == NOT_ASSIGNED) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue

                if(assign_memory(process)) { 
                    process.state = READY;  //Set the process state to READY
                    ready_queue.push_back(process); //Add the process to the ready queue
                    job_list.push_back(process); //Add it to the list of processes

                    execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                } else {
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
            if(process.state == WAITING) {
                if(assign_memory(process) && process.io_freq == 0 && process.io_duration == 0) {
                    process.state = READY;
                    ready_queue.push_back(process);
                    execution_status += print_exec_status(current_time, process.PID, WAITING, READY);

                } else if (process.io_remaining > 0) {
                    process.io_remaining--;
                    if (process.io_remaining == 0) {
                        process.state = READY;
                        ready_queue.push_back(process);
                        execution_status += print_exec_status(current_time, process.PID, WAITING, READY);
                    }
                }
            }
        }

        /////////////////////////////////////////////////////////////////
        
        //////////////////////////SCHEDULER/////////////////////////////

        wait_queue.erase(
            std::remove_if(wait_queue.begin(), wait_queue.end(),
                        [](const PCB &p) { return p.state == READY; }),
            wait_queue.end()
        );
        
        if((running.state == NOT_ASSIGNED || running.state == TERMINATED) && !ready_queue.empty()) {
            priority(ready_queue);
            run_process(running, job_list, ready_queue, current_time);
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);

        }

        if(running.state == RUNNING) {
            sync_queue(job_list, running);

            if (running.io_freq > 0) {
                running.io_count++;

                if(running.io_count == running.io_freq && running.remaining_time > 0) {

                    execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING);
                    running.state = WAITING;
                    running.io_remaining = running.io_duration;
                    running.io_count = 0;
                    wait_queue.push_back(running);
                    
                    idle_CPU(running);
                    continue;
                }
            }

            if(running.state == RUNNING) {
                running.remaining_time--;
                if(running.remaining_time == 0) {
                    terminate_process(running, job_list);
                    execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
                    idle_CPU(running);
                }
            }
        }       
        
        current_time++; 
        
        /////////////////////////////////////////////////////////////////
    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
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
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
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
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}