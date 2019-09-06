#include <iostream>
#include <bits/stdc++.h>
#include <fstream>
#include <string>
#include <queue>
#include <vector>
#include <list>
#include <iomanip>
#include <array>

using namespace std;

#define PAGE_T_SIZE 64

unsigned long long totalcost = 0;
unsigned long num_instr = 0;
int frame_size;


typedef struct _pte_t{ //32 bit
    unsigned int present_valid:1;
    unsigned int write_protect:1;
    unsigned int modified:1;
    unsigned int referenced:1;
    unsigned int pagedout:1;
    unsigned int frame_num:7; //number of frame at max: 2^7 = 128
    unsigned int freespace:20; //it can be used for mapped or not

}pte_t;

typedef struct _vm_entry{
    unsigned int start;
    unsigned int end;
    bool write_protected;
    bool filemapped;
}vm_entry;

typedef struct _pstats{
    unsigned long unmaps =0;
    unsigned long maps =0;
    unsigned long ins =0;
    unsigned long outs =0;
    unsigned long fins =0;
    unsigned long fouts =0;
    unsigned long zeros =0;
    unsigned long segv =0;
    unsigned long segprot =0;
}pstatistics;

class Process{
public:
    int pid;
    pstatistics pstats;
    vector<vm_entry> vmas; 
    array<pte_t, PAGE_T_SIZE> pageTable = {}; //size is fixed for 64 bits
    
    void init_process(int _pid){
        pid = _pid;
    }

    void add_vma(int _start, int _end, int wprotect, int filemap){
    
        vm_entry entry;
        entry.start = _start;
        entry.end = _end;
        if (wprotect ==0) entry.write_protected = false;
        else entry.write_protected = true;

        if (filemap == 0) entry.filemapped = false;
        else entry.filemapped = true;

        vmas.push_back(entry);
    

    }


    void print_pagetable(){
        for (int i = 0; i < PAGE_T_SIZE; i++){
            if (pageTable[i].present_valid == 1){
                cout<< " " << i << ":";
                
                if (pageTable[i].referenced==1) cout <<"R";
                else cout <<"-";
                
                if (pageTable[i].modified==1) cout <<"M";
                else cout <<"-";

                if (pageTable[i].pagedout==1) cout <<"S";
                else cout <<"-";

            }
            else{  
                if (pageTable[i].pagedout ==1) cout << " #";
                else cout << " *";

            }
        }
        cout <<endl;
    }
    
    bool isValid(int page){ 
        for (int i=0; i < vmas.size(); i++){
            if (vmas[i].start <= page && page <= vmas[i].end){
                return true;
            }
        }
        return false;
    }

    bool isPresent(int page){
        if (pageTable[page].present_valid == 1) return true;
        else false;
    }

    void ZeroInFin(int page, bool ohhh){
        if(pageTable[page].pagedout == 0){
            if (!isFileMapped(page)){ 
                if (ohhh) cout << " ZERO" <<endl;
                pstats.zeros++;
                totalcost +=150;
            }
            else{
                if (ohhh) cout << " FIN" <<endl;
                pstats.fins++;
                totalcost+=2500;
            }
        }
        else{
            if (!isFileMapped(page)){
                if (ohhh) cout << " IN"<<endl;
                pstats.ins++;
                totalcost+=3000;
            }
            else{
                if (ohhh) cout << " FIN"<<endl;
                pstats.fins++;
                totalcost+=2500;
            }
        }
        
    }

    bool isFileMapped(int page){
        for (int i = 0; i < vmas.size(); i++){
            if (vmas[i].start <= page && page <= vmas[i].end){
                if (vmas[i].filemapped) return true;
                else return false;
            }

        }
    }

    bool isWriteProtected(int page){
        for (int i = 0; i < vmas.size(); i++){
            if (vmas[i].start <= page && page <= vmas[i].end){
                if (vmas[i].write_protected) return true;
            }
        }
        return false;
    }


    void readInstruction(int page){
        pageTable[page].referenced = 1;
    }

    void writeInstruction(int page, bool ohhh){
        if (isWriteProtected(page)){
            if (ohhh) cout << " SEGPROT" << endl;
            pstats.segprot++;
            totalcost+=300;
        }
        else{
            pageTable[page].modified = 1; 
        }
        pageTable[page].referenced = 1;
    }

    void print_stat(){
        printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu \n",
                pid, pstats.unmaps, pstats.maps, pstats.ins, pstats.outs, pstats.fins, pstats.fouts, 
                pstats.zeros, pstats.segv, pstats.segprot );
    }


};

vector<Process> processList;

class Frame_table{
public:
    vector <pair <int, int> > frame_t; // pid, page
    vector <int> free_list;
    int size;
    vector <bitset<32> > age_bit;
    vector <unsigned long> working_set;
//public:    
    void init_frame(int _size){
        size = _size;
        frame_t.resize(size, make_pair(-1,-1));
        for (int i = 0; i < size; i++){
            free_list.push_back(i);
            bitset<32> bset;
            age_bit.push_back(bset);
            working_set.push_back(num_instr);
        }
    }

    int find_freeslot(){
        if (free_list.empty()){
            return -1;
        }
        else{
            int res;
            res = free_list.front();
            free_list.erase(free_list.begin());
            return res;
        }        
    }

    void addPage(int index, int procid, int vpage, bool ohhh){
        frame_t[index].first = procid;
        frame_t[index].second = vpage;
        if (ohhh){
            cout << " MAP " << index << endl;
        }
        totalcost+=400;
        age_bit[index].reset();
    }

    pair<int, int> removePage(int index, bool ohhh){
        if (ohhh){
            cout << " UNMAP " << frame_t[index].first << ":" << frame_t[index].second << endl;
        }


        totalcost+=400;
        return frame_t[index];

    }

    int indexFinder(int _procid, int _vpage){
        
        pair <int, int> target (_procid, _vpage);

        vector<pair<int, int>>::iterator it = find(frame_t.begin(), frame_t.end(), target);
        int index = distance(frame_t.begin(), it);
    
        return index;
    }

    pair<int, int> getter(int _index){
        return frame_t[_index];
    }

    void frame_printer(){
        cout << "FT: ";
        for (int i = 0; i < size; i++){
            if (frame_t[i].first == -1){
                cout << "* ";
            }
            else{
                cout << frame_t[i].first << ":" << frame_t[i].second <<" ";
            }
        }
        cout << endl;

    }
  
};

vector<int> randvals;
unsigned int ofs = 0;

void random_input(string filename){
    ifstream file(filename);
    string magic;
    getline(file, magic);
    int random_num;
    while (file >> random_num){
        randvals.push_back(random_num);
    }
    
}

int myrandom(){ // burst = frame size
    if (ofs == randvals.size()) ofs = 0;
    return (randvals[ofs++] % frame_size);
}


class Pager{
public:
    //virtual void marker(int _index, int _procid, int _vpage, bool insert){}
    virtual int select_victim_frame(Frame_table* frame_table){return 0;}
};

class FIFO: public Pager{
    //vector<int> fifo_queue;
    int hand = 0;
    public:
    /*void marker(int _index, int _procid, int _vpage, bool insert_flag) override{
        if (insert_flag){
            fifo_queue.push_back(_index);
        }
        else{
            vector<int>::iterator it = find(fifo_queue.begin(), fifo_queue.end(), _index);
            fifo_queue.erase(it);

        }
    }
    */
    int select_victim_frame(Frame_table* frame_table) override{
        /*
        int index = fifo_queue.front();
        fifo_queue.erase(fifo_queue.begin());
        return index;
        */
        int res;
        res = hand;
        hand = (hand+1) % frame_size;

        return res;

    }

};

class Random: public Pager{
public:
    int select_victim_frame(Frame_table* frame_table) override{
        int index = myrandom();
        return index;
    }    
    
};

class Clock: public Pager{
    int hand = 0;
public:
    // it should iterate all of the frametable
    int select_victim_frame(Frame_table* frame_table){
        
        pair<int, int> currframe = frame_table-> getter(hand);
        while (processList[currframe.first].pageTable[currframe.second].referenced != 0){
            processList[currframe.first].pageTable[currframe.second].referenced = 0;
            hand = (hand + 1) % frame_size;
            currframe = frame_table-> getter(hand); 
        }
        int res = hand;
        hand = (hand + 1) % frame_size;
        return res;
    }

};

class NRU: public Pager{
    unsigned long prev_instr = 0;
    int hand = 0;
    int classes[4] = {-1, -1, -1, -1};
    public:
    int select_victim_frame (Frame_table* frame_table) override{
        
        for (int i = 0; i < 4; i++){
            classes[i]=-1;
        }

        int index; 
        int class_index;
        int returnframe = -1;

        for (int i = 0; i < frame_size; i++){
            index = (i + hand) % frame_size;
            pair<int, int> tmp = frame_table -> getter(index);
            class_index = 2 * processList[tmp.first].pageTable[tmp.second].referenced + processList[tmp.first].pageTable[tmp.second].modified;

            if (class_index == 0){
                //early stopping;
                returnframe = index;
                hand = (returnframe + 1) % frame_size;
                break;
            }

            if (classes[class_index] == -1){
                classes[class_index] = index;
            }
        }

        if (returnframe == -1){
            for (int i = 0; i < 4; i++){
                if (classes[i] != -1){
                    returnframe = classes[i];
                    hand = (returnframe + 1) % frame_size;
                    break;
                }
            }
        }

        // reset reference bits 
        if (num_instr - prev_instr >= 49){
            for (int j = 0; j < frame_size; j++){
                pair<int, int> frame = frame_table -> getter(j);
                if (frame.first != -1){
                    processList[frame.first].pageTable[frame.second].referenced = 0;
                }
            }
            prev_instr = num_instr + 1;
        }

        return returnframe;          
    }

}; // not recently used =? least recently used

class Aging: public Pager{
    int hand = 0;
    int res;
public:
    int select_victim_frame(Frame_table* frame_table) override{
        pair<int, int> currframe;
        res = hand;
        int index;
        for (int i = 0; i < frame_size; i++){
            index = (i + hand) % frame_size;
            frame_table -> age_bit[index] = frame_table -> age_bit[index] >> 1;
            currframe = frame_table -> getter(index);
            if (processList[currframe.first].pageTable[currframe.second].referenced == 1){
                processList[currframe.first].pageTable[currframe.second].referenced = 0; // reset for next round examination
                bitset<32> tmp(1);
                tmp = tmp << 31;
                frame_table -> age_bit[index] |= tmp;
            }
 
            //relaxation
            if (frame_table -> age_bit[index].to_ulong() < frame_table -> age_bit[res].to_ulong()){
                res = index;
            }
        }
        hand = (res + 1) % frame_size;
        
        return res;
    }
};

class WorkingSet: public Pager{
    int hand = 0;
    
    public:
    int select_victim_frame(Frame_table* frame_table) override{
        
        int index;
        pair <int, int> currframe;
        int res = hand;
        unsigned long tmp_oldest = LONG_MAX;

        for (int i = 0; i < frame_size; i++){
            index = (i + hand) % frame_size;
            currframe = frame_table -> getter(index);

            if (processList[currframe.first].pageTable[currframe.second].referenced == 1){
                processList[currframe.first].pageTable[currframe.second].referenced = 0;
                frame_table -> working_set[index] = num_instr;

            }
            else{
                if (num_instr - frame_table->working_set[index] >= 50){
                    res = index;
                    break;
                }
                else{
                    //relaxation                    
                    if (frame_table -> working_set[index] < tmp_oldest){
                        tmp_oldest = frame_table-> working_set[index];
                        res = index;
                    }

                }
            }


        }
        hand = (res + 1) % frame_size;
        return res;
    }
};

class Simulation{
public:
    
    int currproc;
    ifstream file;

    string after_sharp(){
        string str;
        getline(file, str); 
        
        while(str[0] == '#'){
            if (file.eof()){
                return NULL;
            }
            getline(file, str);
        }
        return str;
    }

    Process get_process(int pid){
        string str;
        int vmas;
        int start_v, end_v, write_protected, filemapped;
        
        Process proc;
        proc.init_process(pid);

        str = after_sharp();
        vmas = stoi(str);
        
        for (int i=0; i < vmas; i++){
            // parse the line
            str = after_sharp();
            stringstream s(str);
            string token;
            s >> token;
            start_v = stoi(token);
            s >> token;
            end_v = stoi(token);
            s >> token;
            write_protected = stoi(token);
            s >> token;
            filemapped = stoi(token);
            proc.add_vma(start_v, end_v, write_protected, filemapped); //modify with vmas info later
        }
        return proc;
    }

    void simulator(string filename, int frame_size, Pager& pager, bool ohhh, bool pt_option, bool ft_option, bool summary, bool each_pt, bool whole_pt, bool each_ft){
        file.open(filename);
        Frame_table ft;
        ft.init_frame(frame_size);
        
        int pid = 0;
        char instr;
        int num_proc, vpage, procid;
        
        unsigned long ctx_switches = 0;
        unsigned long process_exits = 0;
        
        
        string str;
        str = after_sharp();
        num_proc = stoi(str);
        //cout << "num_proc:" << num_proc << endl;

        // process and each vmas appear
        for (int i = 0; i < num_proc; i++){
            Process proc;
            proc = get_process(pid);
            processList.push_back(proc);
            pid++;
        }
        //instruction part
        while (getline(file, str)){
            if (str[0]== '#'){
                continue;
            }
            stringstream s(str);
            s >> instr;
            //cout << instr << " ";
            if (instr == 'c'){
                s >> procid;
                currproc = procid;
                totalcost += 121;
                ctx_switches++;
                if (ohhh){
                    printf("%lu: ==> %c %d\n",num_instr, instr, currproc);
                }
            }
            else if (instr == 'w' || instr == 'r'){
                totalcost+=1;
                s >> vpage;
                if (ohhh){
                    printf("%lu: ==> %c %d\n", num_instr, instr, vpage);
                }

                int ind;
                if (!processList[currproc].isPresent(vpage)){ //page fault
                    if (processList[currproc].isValid(vpage)){
                        //first check the frame
                        ind = ft.find_freeslot();
                        if (ind == -1){
                            //paging algorithm is needed
                            //abort first
                            ind = pager.select_victim_frame(&ft);
                            pair<int, int> oldpage;
                            oldpage = ft.removePage(ind, ohhh); // print UNMAP _: __
                            processList[oldpage.first].pstats.unmaps++;
                            //modify the old page
                            processList[oldpage.first].pageTable[oldpage.second].present_valid = 0;
                            processList[oldpage.first].pageTable[oldpage.second].referenced = 0;
                            if (processList[oldpage.first].pageTable[oldpage.second].modified ==1){
                                processList[oldpage.first].pageTable[oldpage.second].modified =0;
                                if (processList[oldpage.first].isFileMapped(oldpage.second)){
                                    if (ohhh) cout << " FOUT" << endl;
                                    processList[oldpage.first].pstats.fouts++;
                                    totalcost+=2500;
                                }
                                else{
                                    processList[oldpage.first].pageTable[oldpage.second].pagedout =1;
                                    if (ohhh) cout << " OUT" << endl;
                                    processList[oldpage.first].pstats.outs++;
                                    totalcost+=3000;
                                }
                            }

                        }
                        processList[currproc].ZeroInFin(vpage, ohhh);
                        ft.addPage(ind, procid, vpage, ohhh); // printMAP

                        processList[currproc].pstats.maps++;
                        processList[currproc].pageTable[vpage].frame_num = ind;
                        processList[currproc].pageTable[vpage].present_valid = 1;
                        
                        if (instr == 'w') processList[currproc].writeInstruction(vpage, ohhh);
                        else processList[currproc].readInstruction(vpage);
                        ft.working_set[ind] = num_instr;

                    }
                    else{
                        // SEGV
                        if (ohhh) cout << " SEGV" << endl;
                        processList[currproc].pstats.segv++;
                        totalcost+=240;
                    }

                }
                else{
                    // already in the frame set the ref bit write bit
                    if (instr == 'w') processList[currproc].writeInstruction(vpage, ohhh);
                    else processList[currproc].readInstruction(vpage);
                    ind = ft.indexFinder(procid, vpage);
                    ft.working_set[ind] = num_instr;
                    
                }
                
            }
            else if (instr == 'e'){
                s >> procid;
                if (ohhh){
                    printf("%lu: ==> %c %d\n", num_instr, instr, procid);
                }

                currproc = procid; //it may not be needed
                process_exits++;
                totalcost += 175;
                if (ohhh) printf("EXIT current process %d\n", currproc);
                
                for (int i =0; i < PAGE_T_SIZE; i++){
                    if (processList[currproc].pageTable[i].present_valid == 1){
                        if (ohhh){
                            printf(" UNMAP %d:%d\n", currproc, i);
                        }
                        processList[currproc].pstats.unmaps++;
                        totalcost += 400;
                        
                        if (processList[currproc].pageTable[i].modified == 1 && processList[currproc].isFileMapped(i)){
                            if (ohhh) cout << " FOUT" << endl;
                            processList[currproc].pageTable[i].modified = 0;
                            processList[currproc].pstats.fouts++;
                            totalcost += 2500;

                        }
                        
                        // deal with frame table
                        int index = processList[currproc].pageTable[i].frame_num;
                        ft.frame_t[index].first = -1;
                        ft.frame_t[index].second = -1;
                        ft.free_list.push_back(index);

                    }
                    // should I put it inside the if statement?
                    processList[currproc].pageTable[i].present_valid = 0;
                    processList[currproc].pageTable[i].referenced = 0;
                    processList[currproc].pageTable[i].pagedout = 0;
                }

            }
            else cout << "invalid instruction: "<< instr << endl;
            num_instr ++;

            // each pt
            if (each_pt && instr != 'c' && instr != 'e'){
                printf("PT[%d]:",currproc);
                processList[currproc].print_pagetable();
            }


            // whole pt
            if (whole_pt && instr != 'c' && instr != 'e'){
                for (int i =0; i < processList.size(); i++){
                    printf("PT[%d]:",i);
                    processList[i].print_pagetable();
                }
            }

            // each ft
            if (each_ft && instr != 'c' && instr != 'e'){
                ft.frame_printer();
            }

        }
        // print P
        if (pt_option){
            for (int i=0; i < processList.size(); i++){
                printf("PT[%d]:",i);
                processList[i].print_pagetable();

            }
        }
        // print F
        if (ft_option){
            ft.frame_printer();
        }
        // print S
        if (summary){
            for (int i=0; i < processList.size(); i++){
                processList[i].print_stat();
            }

            printf("TOTALCOST %lu %lu %lu %llu \n", num_instr, ctx_switches, process_exits, totalcost);
            
        }

    }


};

int main(int argc, char *argv[]){

    bool ohhh = false; // O
    bool pt_option = false; // P
    bool ft_option = false; // F
    bool summary = false; // S
    bool each_pt = false; // x
    bool whole_pt = false; // y
    bool each_ft = false; // f
    bool aging_info = false; // a
    int algo;

    string inputfile, randomfile;
    Simulation sims;

    for (int i = 1; i < argc; i++){
        if (argv[i][0] == '-'){
            if (argv[i][1] == 'a'){
                if (argv[i][2] == 'f'){ algo = 1;}
                else if (argv[i][2] == 'r'){ algo = 2;}
                else if (argv[i][2] == 'c'){ algo = 3;}
                else if (argv[i][2] == 'e'){ algo = 4;}
                else if (argv[i][2] == 'a'){ algo = 5;}
                else{ algo= 6;}
            }
            else if (argv[i][1] == 'o'){
                string tmp = argv[i];
                for (int j = 0; j < tmp.size(); j++){
                    if (j == 0 || j == 1) continue;
                    
                    if (argv[i][j]== 'O') ohhh = true;
                    if (argv[i][j]== 'P') pt_option = true;
                    if (argv[i][j]== 'F') ft_option = true;
                    if (argv[i][j]== 'S') summary = true; 
                    if (argv[i][j]== 'x') each_pt = true;
                    if (argv[i][j]== 'y') whole_pt = true;
                    if (argv[i][j]== 'f') each_ft = true;
                    //if (argv[i][j]== 'a') aging_info = true;
                }
            }
            else if (argv[i][1] == 'f'){
                sscanf(&argv[i][2], "%d", &frame_size);
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

    if (algo == 1){
        FIFO pager;
        sims.simulator(inputfile, frame_size, pager, ohhh, pt_option, ft_option, summary, each_pt, whole_pt, each_ft);
    }
    else if (algo == 2){
        Random pager;
        random_input(randomfile);
        sims.simulator(inputfile, frame_size, pager, ohhh, pt_option, ft_option, summary, each_pt, whole_pt, each_ft);
    }
    else if (algo == 3){
        Clock pager;
        sims.simulator(inputfile, frame_size, pager, ohhh, pt_option, ft_option, summary, each_pt, whole_pt, each_ft);
    }
    else if (algo == 4){
        NRU pager;
        sims.simulator(inputfile, frame_size, pager, ohhh, pt_option, ft_option, summary, each_pt, whole_pt, each_ft);
    }
    else if (algo ==5){
        Aging pager;
        sims.simulator(inputfile, frame_size, pager, ohhh, pt_option, ft_option, summary, each_pt, whole_pt, each_ft);
    }
    else if (algo ==6){
        WorkingSet pager;
        sims.simulator(inputfile, frame_size, pager, ohhh, pt_option, ft_option, summary, each_pt, whole_pt, each_ft);
    }
    else{
        cout << "invalid paging algorithm" << endl; 
    }

    return 0;
}
