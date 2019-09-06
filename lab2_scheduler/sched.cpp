#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include <stdio.h>
#include <algorithm>
#include <iomanip>

using namespace std;

typedef enum{ CREATED, READY, RUNNING, BLOCKED} process_state;

typedef enum{ 
    CREATED_READY,
    READY_RUN,
    RUN_READY,
    RUN_BLOCK,
    BLOCK_READY
     } transition_state;

class Process{
public:
    int PID;
    int arrival_time; 
    int total_cpu_time; 
    int cpu_burst;
    int io_burst;
    process_state state= CREATED;
    int current_cpu_burst =0; 
    int current_io_burst;
    int static_prio = 0;
    int dynamic_prio;
    int remaining_cpu = total_cpu_time;
    int ready_start = 0; // cpu wait time can be derived 
    int blocked_start;    
    int finish_time; 
    int turnaround_time;
    int io_time = 0;
    int cpu_waiting_time =0; // time in Ready state

    void init_process(int _PID, int _AT, int _TC, int _CB, int _IO){
        PID = _PID;
        arrival_time = _AT;
        total_cpu_time = _TC;
        cpu_burst = _CB;
        io_burst = _IO;
        remaining_cpu = total_cpu_time;
    }

    friend bool process_priority(Process &A, Process &B){
        return A.dynamic_prio > B.dynamic_prio;
    }
};

class Event{
public:
    int timestamp;
    int PID;
    int duration;
    process_state oldstate;
    process_state newstate;
    transition_state transition;

    void init_event(int _timestamp, int _PID, process_state _oldstate, process_state _newstate, transition_state _tr, int _dur){
        timestamp = _timestamp;
        PID = _PID;
        oldstate = _oldstate;
        newstate = _newstate;        
        transition = _tr;
        duration = _dur;
    }

    bool operator < (const Event& other) const{
            return timestamp < other.timestamp;
        }

};

class DES_Layer {    
public:
    list<Event> eventList;
            Event get_event(){
                Event e = eventList.front();
                eventList.pop_front();
                return e;
        }
        void put_event(Event e){
            // put created Event e to timeline
            eventList.insert(upper_bound(eventList.begin(), eventList.end(),e),e); 

        }

        void remove_event(int prev_pid){

            list<Event>::iterator it;
            it = eventList.begin();
            
            eventList.remove_if([prev_pid](Event e){return e.PID == prev_pid && (e.transition == RUN_READY || e.transition == RUN_BLOCK);});
            
        }

};

class Scheduler {
public:
    list<Process*> runqueue;
    virtual void add_process(Process* process, bool quantumUsed) {};
    virtual Process* pop_process() { return nullptr; };
};

class FCFS_Scheduler: public Scheduler{
    void add_process(Process* process, bool quantumUsed = false){
        runqueue.push_back(process);
    }
     
    Process* pop_process(){
        Process* p;
        if (!runqueue.empty()){
            p = runqueue.front();
            runqueue.pop_front();
            return p;
        }else{
            return nullptr;
        }
    }

};

class LCFS_Scheduler: public Scheduler{
    void add_process(Process* process, bool quantumUsed){
        runqueue.push_front(process);
    }
    Process* pop_process(){
        Process* p;
        if (!runqueue.empty()){
            p = runqueue.front();
            runqueue.pop_front();
            return p;
        }else{
          return nullptr;  
        }
    }
};

class SRTF_Scheduler: public Scheduler{
    void add_process(Process* process, bool quantumUsed){
        runqueue.push_back(process);

    }
    Process* pop_process(){
        Process* p;
        if (runqueue.empty()){
            return nullptr;
        }
        else{
            list<Process*>::iterator it;
            it = runqueue.begin();
            int minimum = (**it).remaining_cpu;
            list<Process*>::iterator it2;
            it2= it;          

            for (it=runqueue.begin(); it != runqueue.end(); it++){
                if (minimum > (**it).remaining_cpu){
                    it2 = it;
                    minimum = (**it).remaining_cpu;
                }
            }
            p = *it2;
            runqueue.erase(it2);
            return p;

        }
    }
};

class RoundRobin_Scheduler: public Scheduler{
    void add_process(Process* process, bool quantumUsed){
        runqueue.push_back(process);
    }
    Process* pop_process(){
        Process* p;
        if (runqueue.empty()){
            return nullptr;
        }
        else{
            p = runqueue.front();
            runqueue.pop_front();
            return p;
        }
    }

};
class Priority_Scheduler: public Scheduler{

    list<Process*> expired;

    void add_process(Process* process, bool quantumUsed){

        //cout << "dynamic prio " << process -> dynamic_prio << endl;

        if (process -> dynamic_prio == -1){
            process -> dynamic_prio = process -> static_prio -1;
            expired.push_back(process);    
        }
        else{
            runqueue.push_back(process);
        }
    }
    
    Process* pop_process(){
        if (runqueue.empty() && expired.empty()){
            return nullptr;
        }
        else{
            Process* p;
            if (runqueue.empty()){
                runqueue.swap(expired);
                //cout << "swapped!" <<endl;
            }
            list<Process*>::iterator it;
            list<Process*>::iterator it2;
            it = runqueue.begin();
            int highest = (**it).dynamic_prio;
            it2 = it;

            for (it=runqueue.begin(); it != runqueue.end(); it++){
                if (highest < (**it).dynamic_prio){
                    it2 =it;
                    highest = (**it).dynamic_prio;
                }
            }
            p = *it2;
            runqueue.erase(it2);

            return p;

        }

    }
    
};


class Simulator {
public:
    vector<Process> processList;
    vector<int> randvals;
    unsigned int ofs = 0;
    int sim_time = 0;
    

    void file_input(string filename){
        ifstream file(filename);
        int AT, TC, CB, IO;
        int PID =0;
        while(file>> AT >> TC >> CB >> IO){            
            Process proc;
            proc.init_process(PID, AT, TC, CB, IO);
            processList.push_back(proc);
            PID++;
        }
    }

    void random_input(string filename){
        ifstream file(filename);
        string magic;
        getline(file, magic);
        int random_num;
        while (file >> random_num){
            randvals.push_back(random_num);
        }
    
    }

    //burst calculate
    int myrandom(int burst){
        if (ofs == randvals.size()) ofs = 0;
        return 1 + (randvals[ofs++] % burst);
    }

    //event putters
    void createdtoReady(DES_Layer& des, Scheduler& sch, int proc_num , bool _quantumUsed = false){
        // create event for create->ready
        Event e;
        e.init_event(processList[proc_num].arrival_time, processList[proc_num].PID, CREATED, READY, CREATED_READY, 0);
        processList[proc_num].state = READY;
        des.put_event(e);
    }

    // ready to running, running to blocked, blocked to ready, running to ready
    void eventDispatcher(DES_Layer& des, int _timestamp, int _pid, process_state prev, process_state next, transition_state trans, int _dur){
        Event e;
        e.init_event(_timestamp, _pid, prev, next, trans, _dur);
        des.put_event(e);
    }

    void verboseOut(int _curr_pid, Event e){

        cout << e.timestamp << " " << _curr_pid << " " << e.duration << ":";

        switch (e.transition)
        {
            case CREATED_READY:
                cout << "CREATED ->  READY"<< endl;
                break;
            case READY_RUN:
                cout << "READY -> RUNNG" << " cb=" << processList[_curr_pid].current_cpu_burst<< " rem="<< processList[_curr_pid].remaining_cpu<<" prio=" << processList[_curr_pid].dynamic_prio<<endl;
                break;
            case RUN_BLOCK:
                cout << "RUNNG -> BLOCK" << " ib=" << processList[_curr_pid].current_io_burst<< " rem="<< processList[_curr_pid].remaining_cpu<<endl;
                break;
            case BLOCK_READY:
                cout << "BLOCK -> READY"<< endl;
                break;
            case RUN_READY:
                cout << "RUNNG -> READY" << " cb=" << processList[_curr_pid].current_cpu_burst<< " rem="<< processList[_curr_pid].remaining_cpu<<" prio=" << processList[_curr_pid].dynamic_prio<<endl;
                //cout << "Done" << endl;
                break;
        }

    }

    void simulationRun(string input, string rfile, Scheduler& sch, int timequantum, int maxprio, bool verbose, bool preem){
        bool tq_used = true;
        int io_using_n = 0;
        int curr_io_start_t;
        int io_util_total = 0; 
        int cpu_free_time = 0; 
        int previous_t; // cpu running start marker;
        int previous_prio; // currently running process' prio
        int previous_pid; // currently running process' pid;

        bool cpu_running = false;
        bool PreEventWasRun = false;
        
        file_input(input);
        random_input(rfile);

        DES_Layer des;

        if (timequantum == 10000){
            tq_used = false;
        }

        for (unsigned int i = 0; i < processList.size(); i++){
            //static_prio assign
            processList[i].static_prio = myrandom(maxprio);
            // init dynamic prio
            processList[i].dynamic_prio = processList[i].static_prio - 1;
            createdtoReady(des, sch, i, tq_used);

        }

 
        while (!des.eventList.empty()){
            Event e;
            int curr_pid;
            int burst;  
            int candi_pid;   
            
            Process* tmp_p;
            e = des.get_event();
            curr_pid = e.PID;
            sim_time = e.timestamp;

            switch(e.transition){
                case CREATED_READY:

                    PreEventWasRun = false ; //==============================================================================
                    processList[curr_pid].state = READY;
                    processList[curr_pid].ready_start = sim_time;

                    if (verbose) verboseOut(curr_pid, e);

                    if (preem && cpu_free_time > sim_time && processList[curr_pid].dynamic_prio > previous_prio){
                        des.remove_event(previous_pid);
                        eventDispatcher(des, sim_time, previous_pid, RUNNING, READY, RUN_READY, sim_time - previous_t);
                        if (verbose) cout << "preem!" << endl;
                        processList[previous_pid].current_cpu_burst += cpu_free_time - sim_time;
                        processList[previous_pid].remaining_cpu += cpu_free_time - sim_time;
                        cpu_free_time = sim_time;
                        eventDispatcher(des, sim_time, curr_pid, READY, RUNNING, READY_RUN, 0);
                        sch.add_process(&processList[curr_pid], tq_used);
                       

                    }
                    else{
                        if (sch.runqueue.empty()){
                            sch.add_process(&processList[curr_pid], tq_used);
                            processList[curr_pid].cpu_waiting_time = 0;

                            eventDispatcher(des, sim_time, curr_pid, READY, RUNNING, READY_RUN, sim_time - processList[curr_pid].ready_start);
                
                        }
                        else{
                            cpu_free_time = max(cpu_free_time, sim_time); // after feedback 
                            sch.add_process(&processList[curr_pid], tq_used);
                            eventDispatcher(des, cpu_free_time, curr_pid, READY, RUNNING, READY_RUN, cpu_free_time - processList[curr_pid].ready_start); // after_feedback
                        
                        }
                    }
                    
                    break;


                case READY_RUN:
                    

                    if (cpu_free_time > sim_time || (cpu_free_time == sim_time && PreEventWasRun)){ 
                        eventDispatcher(des, cpu_free_time, curr_pid, READY, RUNNING, READY_RUN, cpu_free_time - processList[curr_pid].ready_start);
                        //cout << sim_time << " run skipped" << endl;
                        continue;

                    }

                           
                    tmp_p = sch.pop_process();
                    if (tmp_p == nullptr){

                        //cout << "here " << sim_time << endl;
                        continue;
                    }
                    else{
                        e.PID = tmp_p ->PID;
                        tmp_p -> state = RUNNING;
                        tmp_p -> cpu_waiting_time += sim_time - (tmp_p -> ready_start);
                        e.duration = sim_time - tmp_p -> ready_start;
                        PreEventWasRun = true; 
                        
                        // burst retrieval
                        if (tmp_p -> current_cpu_burst > 0){
                            burst = tmp_p-> current_cpu_burst;
                        }
                        else{ // compute new
                            burst = min(tmp_p -> remaining_cpu, myrandom(tmp_p->cpu_burst));
                            tmp_p -> current_cpu_burst = burst;
                        }

                        previous_t = sim_time;
                        previous_prio = tmp_p -> dynamic_prio;
                        previous_pid = tmp_p-> PID;

                        //tmp_p -> current_cpu_burst = burst;
                        if (verbose) verboseOut(tmp_p -> PID, e);

                        if (burst > timequantum){ // preemption
                            
                            cpu_free_time = sim_time + timequantum;
                            tmp_p-> current_cpu_burst -= timequantum;
                            tmp_p-> remaining_cpu -= timequantum;  

                            if (tmp_p -> remaining_cpu == 0){ // this process is terminated after this run if there's no preemption
                                tmp_p -> finish_time = sim_time + timequantum;
                                tmp_p -> turnaround_time = tmp_p -> finish_time - tmp_p -> arrival_time;
                                if (verbose) cout << sim_time + timequantum << "Done" << endl;

                                PreEventWasRun = false; 

                            }
                            else{ // RUN_READY event should be created
                                eventDispatcher(des, sim_time + timequantum, tmp_p -> PID, RUNNING, READY, RUN_READY, timequantum);
                            }

                        }
                        else{ // burst is used as it is
                            cpu_free_time = sim_time + burst;
                            if (burst == tmp_p -> remaining_cpu){ // process is ended after this run
                                tmp_p -> remaining_cpu = 0;
                                tmp_p -> finish_time = sim_time + burst;
                                tmp_p -> current_cpu_burst =  0;
                                tmp_p -> turnaround_time = tmp_p -> finish_time - tmp_p -> arrival_time;
                                if (verbose) cout << sim_time + burst << "Done" << endl; 

                                PreEventWasRun = false; 
                            }
                            else{ // process is not finished yet, create IO block event
                                tmp_p -> remaining_cpu -= burst;
                                tmp_p -> current_cpu_burst -= burst;
                                eventDispatcher(des, sim_time + burst, tmp_p -> PID, RUNNING, BLOCKED, RUN_BLOCK, burst);
                            }
                        }
    
                    }
                    break;


                case RUN_BLOCK:
                    PreEventWasRun = false; //=================================================================
                  

                    processList[curr_pid].state = BLOCKED;                   
                    processList[curr_pid].blocked_start = sim_time;
                    burst = myrandom(processList[curr_pid].io_burst);
                    processList[curr_pid].current_io_burst = burst;
                    eventDispatcher(des, sim_time + burst, curr_pid, BLOCKED, READY, BLOCK_READY, burst);
                    if (io_using_n == 0){
                        curr_io_start_t = sim_time;
                    }
                    io_using_n ++;
                    
                    if(verbose) verboseOut(curr_pid, e);

                    
                    // reset the dynamic_prio to default value when IO blocking occurs
                    processList[curr_pid].dynamic_prio = processList[curr_pid].static_prio -1;
                    
                    //eventDispatcher(des, sim_time, curr_pid, READY, RUNNING, READY_RUN, sim_time - processList[curr_pid].ready_start); // after feedback
                    
                      
                    
                    break;
                case BLOCK_READY:
                    io_using_n--;
                    if (io_using_n == 0){
                        io_util_total += sim_time - curr_io_start_t;
                    }
                    processList[curr_pid].state = READY;
                    processList[curr_pid].io_time += sim_time - processList[curr_pid].blocked_start;
                    processList[curr_pid].ready_start = sim_time;

                    if(verbose) verboseOut(curr_pid, e);

                    PreEventWasRun = false; //=============================================================================

                    if (sim_time < cpu_free_time){
                        if (preem){
                            if(processList[curr_pid].dynamic_prio > previous_prio){
                                des.remove_event(previous_pid);
                                processList[previous_pid].current_cpu_burst += cpu_free_time - sim_time;
                                processList[previous_pid].remaining_cpu += cpu_free_time - sim_time;
                                cpu_free_time = sim_time;
                                 

                                eventDispatcher(des, sim_time, previous_pid, RUNNING, READY, RUN_READY, sim_time - previous_t);
                                // preemptive event
                                if (verbose) cout <<"preem!" << endl;
                                eventDispatcher(des, sim_time, curr_pid, READY, RUNNING, READY_RUN, 0);

                            }
                            else{
                                eventDispatcher(des, cpu_free_time, curr_pid, READY, RUNNING, READY_RUN, cpu_free_time - processList[curr_pid].ready_start);
                            }

                        }
                        else{
                            eventDispatcher(des, cpu_free_time, curr_pid, READY, RUNNING, READY_RUN, cpu_free_time - processList[curr_pid].ready_start);
                        }

                    }
                    else{
                        eventDispatcher(des, sim_time, curr_pid, READY, RUNNING, READY_RUN, sim_time - processList[curr_pid].ready_start);
                    }


                    sch.add_process(&processList[curr_pid], tq_used);
                
                    break;
                case RUN_READY: // RUN_READY preemptive

                    // pick new process (including previous proc) to run
                    processList[curr_pid].state = READY;
                    processList[curr_pid].ready_start = sim_time;

                    PreEventWasRun = false; 

                    if (verbose) verboseOut(curr_pid, e);
                   
                    processList[curr_pid].dynamic_prio -=1;

                    //dispatch the run event;
                    if (sim_time < cpu_free_time){
                        eventDispatcher(des, cpu_free_time, curr_pid, READY, RUNNING, READY_RUN, cpu_free_time - processList[curr_pid].ready_start);
                    }
                    else{
                        eventDispatcher(des, sim_time, curr_pid, READY, RUNNING, READY_RUN, sim_time - processList[curr_pid].ready_start);
                    }
                    sch.add_process(&processList[curr_pid], tq_used);
                    
                    break;
            }

        }

        int sum_ft;
        double cpu_util;
        double total_TC = 0.0;
        double avg_tt;
        double sum_tt;
        double num_proc = 0.0;
        double avg_cpu_wait = 0.0;
        double sum_cpu_wait = 0.0;


        for (auto it = processList.begin(); it != processList.end(); it++){

            printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",it->PID, it->arrival_time, it->total_cpu_time, it->cpu_burst, it->io_burst, it->static_prio, 
            it->finish_time, it-> turnaround_time, it-> io_time, it->cpu_waiting_time);

            total_TC += it->total_cpu_time;
            sum_ft = max(it->finish_time, sum_ft);
            sum_tt += it->turnaround_time;
            sum_cpu_wait += it->cpu_waiting_time;
            num_proc ++;

        }
        cpu_util = total_TC*100.0/sum_ft;
        avg_tt = sum_tt / num_proc;
        avg_cpu_wait = sum_cpu_wait / num_proc;
        printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", sum_ft, cpu_util, io_util_total*100.0/sum_ft, avg_tt, avg_cpu_wait, num_proc*100/sum_ft  );

    }
};

int main(int argc, char* argv[]){
    bool verbose = false;
    int maxprio = 4;
    string inputfile, randomfile;
    int quantum = 10000;
    int sched;
    bool preem = false;

    for (int i = 1; i < argc; i++){
        if (argv[i][0] == '-'){
            if (argv[i][1] == 'v'){
                verbose = true;
            }
            else if (argv[i][1] == 's'){

                if (argv[i][2] == 'F'){
                    sched = 0;
                    //FCFS_Scheduler sch;
                }
                else if (argv[i][2] == 'L'){
                    sched = 1;
                    //LCFS_Scheduler sch;
                } 
                else if (argv[i][2] == 'S'){
                    sched = 2;
                    //SRTF_Scheduler sch;
                }
                else if (argv[i][2] == 'R'){
                    sched = 3;
                    //RoundRobin_Scheduler sch;
                    quantum = atoi(&argv[i][3]);
                }
                else if (argv[i][2] == 'P'){
                    //Priority Scheduler
                    sched = 4;
                    //Priority_Scheduler sch;
                    sscanf(&argv[i][3],"%d:%d", &quantum, &maxprio); 
                }
                else if (argv[i][2] == 'E'){
                    //Priority_Scheduler with preemption
                    sched = 5;
                    sscanf(&argv[i][3], "%d:%d", &quantum, &maxprio);
                }
            }
        }
        else{
            if (inputfile.empty()){
                inputfile = argv[i];
            }
            else{
                randomfile = argv[i];
            }
        }
    }
    Simulator sim;

    switch (sched)
    {
        case 0:{
            cout << "FCFS" << endl;
            FCFS_Scheduler sch;
            sim.simulationRun(inputfile, randomfile, sch, quantum, maxprio, verbose, preem);
            break;
        }case 1:{
            cout << "LCFS" << endl;
            LCFS_Scheduler sch;
            sim.simulationRun(inputfile, randomfile, sch, quantum, maxprio, verbose, preem);
            break;
        }case 2:{
            cout << "SRTF" << endl;
            SRTF_Scheduler sch;
            sim.simulationRun(inputfile, randomfile, sch, quantum, maxprio, verbose, preem);
            break;
        }case 3:{
            cout << "RR" << " " << quantum << endl;
            RoundRobin_Scheduler sch;
            sim.simulationRun(inputfile, randomfile, sch, quantum, maxprio, verbose, preem);
            break;
    
        }case 4:{
            cout << "PRIO" << " " << quantum << endl;
            Priority_Scheduler sch;
            sim.simulationRun(inputfile, randomfile, sch, quantum, maxprio, verbose, preem);
            break;
        }case 5:{
            cout << "PREPRIO" << " " << quantum << endl;
            Priority_Scheduler sch;
            preem = true;
            sim.simulationRun(inputfile, randomfile, sch, quantum, maxprio, verbose, preem);
            break;
        }    
    }

    return 0;
}