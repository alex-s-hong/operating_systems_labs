#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <deque>
#include <stdlib.h>
#include <iomanip>
#include <bits/stdc++.h> 

using namespace std;

int total_time = -1;
int tot_movement = 0;
int max_wait = 0;
double avg_turnaround, avg_wait;
bool verbose = false;
bool flook_info = false;

class IO{
    public:
    int id;
    int arrival_t;
    int start_t = -1;
    int end_t;
    int track;

    void init_io(int _id, int _arrival_t, int _track){
        id = _id;
        arrival_t = _arrival_t;
        track = _track;
    }
};


class Strategy{
    public:
    int track = 0;
    bool newtrack = false;
    deque<IO*> ioqueue;
    virtual bool isEmpty(){
        return ioqueue.empty();
    }
    virtual void io_add(IO* io, int time){
        if (ioqueue.empty()){
            ioqueue.push_back(io);
            ioqueue[0]->start_t = time;
            if (verbose){
                printf("%d:\t %d issue %d %d\n",time, ioqueue[0]->id, ioqueue[0]->track, track);
            }
        }
        else{
            ioqueue.push_back(io);
        }
    };
    virtual void job_proceed(int time){};
};

class FIFO:public Strategy{

    void job_proceed(int time) override{
        if (ioqueue.empty()){
            return;
        }
        //check active is finished
        if (ioqueue[0]->track == track){
            while (newtrack == false){
                
                ioqueue[0]->end_t = time;
                if (verbose){
                    printf("%d:\t %d finish %d\n",time, ioqueue[0]->id, ioqueue[0]->end_t - ioqueue[0]->arrival_t);
                }
                ioqueue.pop_front();

                if (ioqueue.empty()){
                    total_time = time;
                    break;
                }
                else{
                    ioqueue[0]->start_t = time;
                    if (verbose){
                        printf("%d:\t %d issue %d %d\n",time, ioqueue[0]->id, ioqueue[0]->track, track);
                    }
                    if (ioqueue[0]-> track != track){
                        newtrack = true;
                    }
                }

                
            }
            newtrack = false;
              
        }
        //move track
        if (!ioqueue.empty()){
            if (ioqueue[0]->track > track){
                track++;
                tot_movement++;
            }
            else{
                track--;
                tot_movement++;
            }
        }
    }
};

class SSTF:public Strategy{
    int curr = 0;
    void job_proceed(int time) override{
        if (ioqueue.empty()){
            return;
        }
        if (ioqueue[curr]->track == track){

            while (newtrack == false){
                ioqueue[curr]-> end_t = time;
                if (verbose){
                    printf("%d:\t %d finish %d\n",time, ioqueue[curr]->id, ioqueue[curr]->end_t - ioqueue[curr]->arrival_t);
                }
                deque<IO*>::iterator it = ioqueue.begin();
                for (int i = 0; i < curr; i++){
                    it++;
                }
                ioqueue.erase(it);

                if (ioqueue.empty()){
                    total_time = time;
                    break;
                }
                else{
                    //search for next io
                    int min_diff = abs(ioqueue[0]->track - track);
                    curr = 0;
                    //it = ioqueue.begin();
                    for (int i = 0; i < ioqueue.size(); i++){
                        if (min_diff > abs(ioqueue[i]-> track - track)){
                            curr = i;
                            min_diff = abs(ioqueue[i]->track - track);
                        }
                    }
                    ioqueue[curr]->start_t = time;
                    if (verbose){
                        printf("%d:\t %d issue %d %d\n",time, ioqueue[curr]->id, ioqueue[curr]->track, track);
                    }
                    if (ioqueue[curr]->track != track){
                        newtrack = true;
                    }

                }

            }
            newtrack = false;          
        }
        if (!ioqueue.empty()){
            if (ioqueue[curr]->track > track){
                track++;
                tot_movement++;
            }
            else{
                track--;
                tot_movement++;
            }
        }
    }

};

class LOOK:public Strategy{
    int curr =0;
    bool ascending = true;
    
    void io_add(IO* io, int time) override{
        if (ioqueue.empty()){
            ioqueue.push_back(io);
            ioqueue[0]->start_t = time;
            if (verbose){
                printf("%d:\t %d issue %d %d\n",time, ioqueue[0]->id, ioqueue[0]->track, track);
            }
            if (ioqueue[0]->track < track && ascending) ascending = false;
            else if (ioqueue[0]->track > track && ascending == false) ascending = true;
        }
        else{
            ioqueue.push_back(io);
        }
    }

    bool isEdge(){
        if (ascending){
            for (int i = 0; i < ioqueue.size(); i++){
                if (ioqueue[i]-> track >=track){
                    return false;
                }
            }
            return true;
        }
        else{
            for (int i = 0; i < ioqueue.size(); i++){
                if (ioqueue[i]-> track <= track){
                    return false;
                }
            }
            return true;
        }
    }

    void job_proceed(int time) override{
        if (ioqueue.empty()){
            return;
        }

        if (ioqueue[curr]->track == track){
            //cout << "current time:"  << time << " ascending " << ascending <<endl;
            while (newtrack == false){
                ioqueue[curr]->end_t = time;
                if (verbose){
                    printf("%d:\t %d finish %d\n",time, ioqueue[curr]->id, ioqueue[curr]->end_t - ioqueue[curr]->arrival_t);                    
                }
                deque<IO*>::iterator it = ioqueue.begin();
                for (int i = 0; i < curr; i++){
                    it++;
                }
                ioqueue.erase(it);

                if (ioqueue.empty()){
                    total_time =time;
                    break;
                }
                else{

                    if (isEdge()){
                        ascending = !ascending;
                        //cout << "flipped: " << ascending << endl;        
                    }
                    //if (time == 7058) cout << "ascending is " << ascending <<endl;
                    int min_diff = INT_MAX;
                    for (int i = 0; i < ioqueue.size(); i++){
                        if (ascending){
                            if (ioqueue[i]->track >= track && ioqueue[i]->track - track < min_diff){
                                min_diff = ioqueue[i]->track - track;
                                curr = i;
                            }
                        }
                        else{
                            if (ioqueue[i]->track <= track && track -ioqueue[i]->track < min_diff){
                                min_diff = track - ioqueue[i]->track;
                                curr = i;
                            }
                        }
                    }
                    ioqueue[curr]->start_t = time;
                    if (verbose){
                        printf("%d:\t %d issue %d %d\n",time, ioqueue[curr]->id, ioqueue[curr]->track, track);
                    }    
                    if (ioqueue[curr]-> track != track){
                        newtrack = true;
                    }
                    
                }
            }
            newtrack = false;
        }
        if (!ioqueue.empty()){
            track = (ioqueue[curr]->track > track) ? track+1 : track-1;
            tot_movement++;
        }
    }
};

class CLOOK:public Strategy{
    int curr = 0;
    bool ascending = true;

    bool isEdge(){
        if (ascending){
            for (int i = 0; i < ioqueue.size(); i++){
                if (ioqueue[i]-> track >=track){
                    return false;
                }
            }
            return true;
        }
        else{
            for (int i = 0; i < ioqueue.size(); i++){
                if (ioqueue[i]-> track <= track){
                    return false;
                }
            }
            return true;
        }
    }

    void job_proceed(int time) override{
        if (ioqueue.empty()){
            return;
        }
        if (ioqueue[curr]->track == track){
            while (newtrack == false){
                ioqueue[curr]->end_t = time;
                if (verbose){
                    printf("%d:\t %d finish %d\n",time, ioqueue[curr]->id, ioqueue[curr]->end_t - ioqueue[curr]->arrival_t);    
                }
                deque<IO*>::iterator it = ioqueue.begin();
                for (int i = 0; i < curr; i++){
                    it++;
                }
                ioqueue.erase(it);

                if (ioqueue.empty()){
                    total_time =time;
                    break;
                }
                else{
                    if (isEdge()){
                        int min_track = track;
                        for (int i = 0; i < ioqueue.size(); i++){
                            if (ioqueue[i]->track < min_track){
                                min_track = ioqueue[i]->track;
                                curr = i;
                            }
                        }

                    }
                    else{
                        int min_diff = INT_MAX;
                        for (int i = 0; i < ioqueue.size(); i++){
                            if (ascending){
                                if (ioqueue[i]->track >= track && ioqueue[i]->track - track < min_diff){
                                    min_diff = ioqueue[i]->track - track;
                                    curr = i;
                                }
                            }
                            else{ //useless
                                if (ioqueue[i]->track <= track && track -ioqueue[i]->track < min_diff){
                                    min_diff = track - ioqueue[i]->track;
                                    curr = i;
                                }
                            }
                        }
                    }
                    ioqueue[curr]->start_t = time;
                    if (verbose){
                        printf("%d:\t %d issue %d %d\n",time, ioqueue[curr]->id, ioqueue[curr]->track, track);
                    }    
                    if (ioqueue[curr]-> track != track){
                        newtrack = true;
                    }                    
                }
            }
            newtrack = false;
        }
        if (!ioqueue.empty()){
            track = (ioqueue[curr]->track > track) ? track+1 : track-1;
            tot_movement++;
        }
    }
    
};

class FLOOK:public Strategy{
    int curr = 0;
    bool ascending = true;
    deque<IO*> ioqueue[2];
    bool active = 1;

    bool isEmpty() override{
        return (ioqueue[active].empty() && ioqueue[!active].empty());
    }

    void io_add(IO* io, int time) override{
        if (ioqueue[active].empty() && ioqueue[!active].empty()){
            ioqueue[!active].push_back(io);
            active = !active;
            ioqueue[active][0]->start_t = time;
            if (verbose){
                printf("%d:\t %d issue %d %d\n",time, ioqueue[active][0]->id, ioqueue[active][0]->track, track);
            }
            if (ioqueue[active][0]->track > track && ascending == false) ascending = true;
            else if (ioqueue[active][0]->track < track && ascending == true) ascending = false;            
        }
        else{
            ioqueue[!active].push_back(io);
        }
    }

    bool isEdge(){
        if (ascending){
            for (int i = 0; i < ioqueue[active].size(); i++){
                if (ioqueue[active][i]-> track >=track){
                    return false;
                }
            }
            return true;
        }
        else{
            for (int i = 0; i < ioqueue[active].size(); i++){
                if (ioqueue[active][i]-> track <= track){
                    return false;
                }
            }
            return true;
        }
    }

    void job_proceed(int time) override{
        if (ioqueue[active].empty()){
            active = !active;
            if (ioqueue[active].empty()){
                return;
            }
            else{
                //search for new io process
            }
        }
        if (ioqueue[active][curr]->track == track){
            while (newtrack == false){
                ioqueue[active][curr]->end_t = time;
                if (verbose){
                    printf("%d:\t %d finish %d\n",time, ioqueue[active][curr]->id, ioqueue[active][curr]->end_t - ioqueue[active][curr]->arrival_t);    
                }
                deque<IO*>::iterator it = ioqueue[active].begin();
                for (int i =0; i < curr; i++){
                    it++;
                }
                //cout << "Before erase " << ioqueue[active].size() << endl;  
                ioqueue[active].erase(it);
                //cout << "After erase " << ioqueue[active].size() << endl;

                if (ioqueue[active].empty() && ioqueue[!active].empty()){
                    total_time = time;
                    break;
                }

                //new one
                if (ioqueue[active].empty()){
                    active = !active;
                }
                //else{
                if (isEdge()){
                    ascending = !ascending;
                }
                int min_diff = INT_MAX;
                for (int i = 0; i < ioqueue[active].size(); i++){
                    if (ascending){
                        if (ioqueue[active][i]->track >= track && ioqueue[active][i]->track -track < min_diff){
                            min_diff = ioqueue[active][i]-> track - track;
                            curr = i;
                        }
                    }
                    else{
                        if (ioqueue[active][i]->track <= track && track - ioqueue[active][i]->track < min_diff){
                            min_diff = track - ioqueue[active][i]-> track;
                            curr = i;
                        }                            
                    }
                }
                ioqueue[active][curr]->start_t = time;
                if (verbose){
                    printf("%d:\t %d issue %d %d\n",time, ioqueue[active][curr]->id, ioqueue[active][curr]->track, track);
                }                   
                if (ioqueue[active][curr]->track != track){
                    newtrack = true;
                } 

                //}
            }//while loop end
            newtrack = false;
        }
        if (!ioqueue[active].empty()){
            track = (ioqueue[active][curr]->track > track) ? track+1 : track-1;
            tot_movement++;
        }
    }

};

class Simulation{
    ifstream file;
    //deque<pair<int, int>> io_arrivals;
    vector<IO> all_io;

    string after_sharp(string str){
        while(str[0]=='#'){
            if (file.eof()){
                return NULL;
            }
            getline(file, str);
        }
        return str;
    }

    void take_input(string input){
        file.open(input);
        string str;
        int io_num = 0;
        while (getline(file, str)){
            int arrival_t, track_num;
            str = after_sharp(str);
            stringstream s(str);
            string token;
            s >> token;
            arrival_t = stoi(token);
            s >> token;
            track_num = stoi(token);
            IO io;
            io.init_io(io_num, arrival_t, track_num);
            //pair<int,int> p = make_pair(arrival_t, track_num);
            all_io.push_back(io);
            io_num++;
            
        }
    }
    public:

    void simulator(string input, Strategy* strat){
        int time = 1;
        int next_input = 0;

        take_input(input);

        if (verbose){
            cout << "TRACE" <<endl;
        }

        while (! (all_io.size() == next_input && strat->isEmpty())){
            if (all_io[next_input].arrival_t == time){
                if (verbose){
                    printf("%d:\t %d add %d\n",time, all_io[next_input].id, all_io[next_input].track);
                }
                strat->io_add(&(all_io[next_input]), time);
                next_input++;
            }
            //check IO active completes, issue new IO, move the head
            strat->job_proceed(time);
            time++;
        }

        
        for (int i = 0; i < all_io.size(); i++){
            printf("%5d: %5d %5d %5d\n",i, all_io[i].arrival_t, all_io[i].start_t, all_io[i].end_t);
            if (max_wait < all_io[i].start_t - all_io[i].arrival_t){
                max_wait = all_io[i].start_t - all_io[i].arrival_t;
            }
            avg_wait += all_io[i].start_t - all_io[i].arrival_t;
            avg_turnaround += all_io[i].end_t - all_io[i].arrival_t;
        }
        avg_wait = avg_wait / all_io.size();
        avg_turnaround = avg_turnaround / all_io.size();

        //summary
        printf("SUM: %d %d %.2lf %.2lf %d\n", total_time, tot_movement, avg_turnaround, avg_wait, max_wait);



    }
};


int main(int argc, char* *argv){
    string inputfile;
    Simulation sim;
    int algo;

    for (int i = 1; i <argc; i++){
        if (argv[i][0] == '-'){
            if (argv[i][1] == 's'){
                if (argv[i][2] == 'i'){ algo = 1;}
                else if (argv[i][2] == 'j'){ algo = 2;}
                else if (argv[i][2] == 's'){ algo = 3;}
                else if (argv[i][2] == 'c'){ algo = 4;}
                else if (argv[i][2] == 'f'){ algo = 5;}
                else {algo = 0; }
            }
            else if (argv[i][1]== 'v'){
                verbose = true;
            }
            else{
                flook_info = true;
            }
        }
        else{
            inputfile = argv[i];
        }
    }

    if (algo ==1){
        FIFO strat;
        sim.simulator(inputfile, &strat);
    }
    else if (algo == 2){
        SSTF strat;
        sim.simulator(inputfile, &strat);
    }
    else if (algo == 3){
        LOOK strat;
        sim.simulator(inputfile, &strat);
    }
    else if (algo == 4){
        CLOOK strat;
        sim.simulator(inputfile, &strat);
    }
    else if (algo == 5){
        FLOOK strat;
        sim.simulator(inputfile, &strat);
    }
    else{
        cout << "wrong scheduler input" <<endl;
    }
   
    return 0;
}