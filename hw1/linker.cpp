#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <iomanip>

using namespace std;

typedef struct __SymbolAddr{
    int value;
    bool isDup = false;
    int fromWhichModule = 0;
    int addr_face = 0;
    bool everused = false; // warning 3... PassTwo part
    bool isUsed = false; // for PassTwo... warning 2 (in uselist not used)
    int ex_addr = -1; // 000 001 002
} SymbolAddr;

int line_num, line_offset;
map<string, SymbolAddr> symbol_map;

void __parseerror (int errcode) {
    static string errstr[] = {
        "NUM_EXPECTED", // number expected
        "SYM_EXPECTED", // symbol expected
        "ADDR_EXPECTED", // addressing expected which is A/E/I/R
        "SYM_TOO_LONG", // symbol name is too long
        "TOO_MANY_DEF_IN_MODULE", // > 16
        "TOO_MANY_USE_IN_MODULE", // > 16
        "TOO_MANY_INSTR", // total num_instr exceeds memory size (512)
    };
    cout << "Parse Error line " << line_num << " offset " << line_offset << ": " << errstr[errcode] << endl;
    exit(1);
}

void __error (int errcode, string symbol = ""){
    switch(errcode) {
        case 1:
            cout << "Error: Absolute address exceeds machine size; zero used" << endl;
            break;
        case 2:
            cout << "Error: Relative address exceeds module size; zero used" << endl;
            break;
        case 3:
            cout << "Error: External address exceeds length of uselist; treated as immediate" << endl;
            break;
        case 4:
            cout << "Error: " << symbol << " is not defined; zero used" << endl;
            break;
        case 5:
            cout << "Error: This variable is multiple times defined; first value used" << endl;
            break;
        case 6:
            cout << "Error: Illegal immediate value; treated as 9999" << endl;
            break;
        case 7:
            cout << "Error: Illegal opcode; treated as 9999" << endl;
            break;
    }
}

void __warning (int warncode, int module_num, string sym_name, int face_val= -1, int module_size = 999) {
    switch(warncode) {
        case 1:
            cout<< "Warning: Module " << module_num << ": " << sym_name << " too big " << face_val << " (max=" << module_size -1 << ") assume zero relative" << endl;
            break;
        case 2:
            cout<< "Warning: Module " << module_num << ": " << sym_name << " appeared in the uselist but was not actually used" << endl;
            break;
        case 3:
            cout<< "Warning: Module " << module_num << ": " << sym_name << " was defined but never used" << endl;
            break;
    }

}

void warningcheck (int checkwhat, int module_num){
    switch (checkwhat) {
        case 1: // uselist
            if (!symbol_map.empty()){
                for (auto it = symbol_map.begin(); it != symbol_map.end(); it++){
                    if(it->second.ex_addr != -1 && it->second.isUsed == false){
                        __warning(2, module_num, it->first);
                    }
                }
            }
            break;
        case 2: // ever used
            if (!symbol_map.empty()){
                for (auto it = symbol_map.begin(); it != symbol_map.end(); it++){
                    if (it->second.everused == false){
                        __warning(3, it->second.fromWhichModule, it->first);
                    }
                }
            }
            break;
    }
}

void clearUselist(){
    if (!symbol_map.empty()){
        for (auto it = symbol_map.begin(); it != symbol_map.end(); it++){
            it->second.isUsed = false;
            it->second.ex_addr = -1;
            if (it->second.value == -10){
                symbol_map.erase(it);
            }
        }
    }
}

bool isNumber (string s){
    for (unsigned int i = 0; i < s.length(); i++){
        if (isdigit(s[i])== false){
            return false;
        }
        //return true;
    }
    return true;
}

bool isSymbol (string s){
    if (isalpha(s[0]) == false){
        return false;
    }
    if (s.length()>1){
        for (unsigned int i = 0; i < s.length(); i++){
            if (isalnum(s[i]) == false){
                return false;
            }
        }
    }
    return true;
}

int readInt(string s){
    if (!isNumber(s)){
        __parseerror(0);
    }
    int i = stoi(s);
    return i;
}

string readSymbol(string s){
    if(isSymbol(s)){
        if (s.length()> 16){
            //error : sym too long
            __parseerror(3);
            //exit(1);
        }
    }
    else{
        //error: sym expected(2)
        __parseerror(1);
        exit(1);
    }
    return s;
}

void readIAER(string s){
    if (s.compare("I") == 0 || s.compare("A") == 0|| s.compare("E")== 0 || s.compare("R")== 0){
        return;
    }
    else{
        // error: addr expected(3)
        __parseerror(2);
        //exit(1);
    }
}

void prettyPrinter(int leftnum, int opcode, int operand, int errorcode, string symbol= ""){
    
    cout << setfill('0') << setw(3) << leftnum << ": " << setfill('0') << setw(4)<< opcode*1000 + operand; // error should be added
    if (errorcode == 0){
        cout << endl;
    }
    else{
        cout <<" ";
        __error(errorcode, symbol);
    }
}

void PassOne(string filename){

    ifstream file(filename);
    string magic;
    int module_num = 0;
    int phase = 0; // def_list, use_list, program_txt
    bool defcountExp = true;
    int defcountLeft = 0;
    bool defsymbolExp = false;
    string tmpSymbol = "";
    bool usecountExp = false;
    int usecountLeft = 0;
    bool addrcountExp = false;
    int addrcountLeft = 0;
    bool codetypeExp = false;

    if (file.fail()){
        cerr << "Error Opening File" << endl;
        exit(1);
    }

    line_num = 0;
    int baseAddr = 0;
    vector<int> length_temp;
        
    while(!file.eof()){
        line_num ++;
        //cout << "line number is " << line_num << endl;
        getline(file, magic);
        // magic to char_array
        int n = magic.length();
        length_temp.push_back(n);
        //cout << "length for the line is " << n << endl;
        char char_array[n]; // n? n+1? it will affect the offset
        strcpy(char_array, magic.c_str()); //magic has been copied to char_array

        char *ptr;
        char delimit [] = " \t\n";
        ptr = strtok(char_array, delimit);

        while(ptr != NULL){

            string token = ptr;
            line_offset = ptr - char_array + 1;
            
            if (phase % 3 == 0){
                //definition list
                if (defcountExp){
                    module_num++;
                    //defcount (int) should appear
                    defcountLeft = readInt(token);

                    if (defcountLeft > 16){
                        // error : too many def in module (5)
                        __parseerror(4);
                    }
                    else if (defcountLeft == 0){
                        phase ++;
                        usecountExp = true;
                    }
                    else{
                        defsymbolExp = true;
                    }
                    defcountExp = false;
                }
                else{ //def pairs expected
                    if (defsymbolExp){
                        // symbol expected
                        tmpSymbol = readSymbol(token);
                        defsymbolExp = false;
                    }
                    else{ // def value (int) expected
                        int facevalue = readInt(token); // address appearing in a definition (rule 5)
                        int addr = facevalue + baseAddr;
                        if (symbol_map.count(tmpSymbol)>0){
                            symbol_map[tmpSymbol].isDup = true;
                        }
                        else{ // check this part
                            SymbolAddr sym_addr;
                            sym_addr.value = addr;
                            sym_addr.fromWhichModule = module_num;
                            sym_addr.addr_face = facevalue;
                            symbol_map[tmpSymbol] = sym_addr;
                        }
                        tmpSymbol = "";
                        defcountLeft--;
                        if (defcountLeft == 0){
                            phase ++;
                            usecountExp = true;
                        }
                        else{
                            defsymbolExp = true;
                        }
                    }
                }
            }
            else if (phase % 3 == 1){ // use list
                if (usecountExp){
                    int i = readInt(token);
                    if (i > 16){
                        //error: too many use in module(6)
                        __parseerror(5);
                    }
                    else if (i == 0){
                        phase++;
                        addrcountExp = true;
                    }
                    else{
                        usecountLeft = i;
                    }
                    usecountExp = false;
                }
                else{ // symbol appears
                    readSymbol(token);
                    usecountLeft--;
                    if (usecountLeft == 0){
                        phase++;
                        addrcountExp = true;
                    }
                }
            }
            else{ // program text
                if (addrcountExp){
                    int module_size = readInt(token); // i = sizeofmodule
                    if (module_size + baseAddr > 512){
                        // error: too many instr(7)
                        __parseerror(6);
                    }
                    // check the warning
                    if (!symbol_map.empty()){
                        for (auto it = symbol_map.begin(); it != symbol_map.end(); it++){
                            if (it->second.fromWhichModule == module_num && it->second.addr_face > module_size){
                                __warning(1,module_num,it->first, it->second.addr_face, module_size);
                                it->second.addr_face = 0;
                                it ->second.value = 0;
                            }
                        }
                    }

                    if (module_size == 0){
                        phase++;
                        defcountExp  = true;                        
                    }
                    else{
                        baseAddr += module_size;
                        addrcountLeft = module_size;
                        codetypeExp = true;
                    }
                    addrcountExp = false;
                }
                else{ // pair expected
                    if (codetypeExp){
                        readIAER(token);
                        codetypeExp = false;
                    }
                    else{ //instr: 4 digit opcode check at pass two
                        readInt(token);
                        addrcountLeft--;
                        if (addrcountLeft == 0){
                            phase ++;
                            defcountExp = true;
                        }
                        else{
                            codetypeExp = true;
                        }                      
                    }
                }
            }
            //next token
            ptr = strtok(NULL, delimit);
        }
        //line_offset = n + 1;
    }
    // if we have read all files but we miss some elements-> parse error with location of last position
    line_num = line_num -1;
    int p = length_temp.size();
    line_offset = length_temp[p-2] + 1;
    
    // 1. symbol expected
    if(defsymbolExp || usecountLeft != 0){
        __parseerror(1);
    }
    // 2. num expected
    if ((defcountLeft != 0 && defsymbolExp == false) || usecountExp == true || addrcountExp == true || (addrcountLeft != 0 && codetypeExp == false)){
        __parseerror(0);
    }
    // 3. addr expected
    if (codetypeExp){
        __parseerror(2);
    }
    file.close();

    // printing
    cout << "Symbol Table" << endl;
    if (!symbol_map.empty()){
        for (auto it = symbol_map.begin(); it != symbol_map.end(); it++){
            if (it->second.isDup){
                cout << it->first << "=" << it->second.value << " ";
                __error(5);
            }
            else{
                cout << it->first << "=" << it->second.value << endl;
            }
        }
    }
}


void PassTwo(string filename){

    ifstream file(filename);
    string magic;
    int module_num = 0;
    int phase = 0; // def_list, use_list, program_txt
    //int val_exp;   // num_expected, symbol_expected, addr_expected
    bool defcountExp = true;
    int defcountLeft = 0;
    bool defsymbolExp = false;
    string tmpIAER = "";
    bool usecountExp = false;
    int usecountLeft = 0;
    bool addrcountExp = false;
    int addrcountLeft = 0;
    bool codetypeExp = false;
    int left_num = 0;

    if (file.fail()){
        cerr << "Error Opening File" << endl;
        exit(1);
    }

    // very first thing: initialization
    int relative_tmp = 0;
    int relative = 0;
    int range = 0; // uselist range ... usecount
    
        
    while(!file.eof()){
        line_num ++;
        getline(file, magic);
        // magic to char_array
        int n = magic.length();
        char char_array[n]; // n? n+1? it will affect the offset
        strcpy(char_array, magic.c_str()); //magic has been copied to char_array

        char *ptr;
        char delimit [] = " \t\n";
        ptr = strtok(char_array, delimit);

        while(ptr != NULL){

            string token = ptr;
            //line_offset = ptr - char_array + 1;
            
            if (phase % 3 == 0){
                //definition list
                if (defcountExp){
                    module_num++;
                    defcountLeft = readInt(token);
                    defcountExp = false;
                    if (defcountLeft == 0){
                        phase++;
                        usecountExp = true;
                    }
                    else{
                        defsymbolExp = true;
                    }  
                }
                else{ //def pairs expected
                    if (defsymbolExp){
                        defsymbolExp = false;
                    }
                    else{ 
                        defcountLeft--;
                        if (defcountLeft == 0){
                            phase ++;
                            usecountExp = true;
                        }
                        else{
                            defsymbolExp = true;
                        }
                    }
                }
            }
            else if (phase % 3 == 1){ // use list
                if (usecountExp){
                    //cout << "uselist-passtwo"<< endl;
                    range = readInt(token);
                    usecountExp = false;
                    //cout << "use list"<< endl;
                    if (range == 0){
                        phase++;
                        addrcountExp = true;
                    }
                    else{
                        usecountLeft = range;
                    }
                }
                else{ // symbol appears... update the map
                    string sym = readSymbol(token);

                    map<string, SymbolAddr>::iterator it;
                    it = symbol_map.find(sym);
                    if (it == symbol_map.end()){
                        // does not exist...
                        SymbolAddr symfield;
                        symfield.ex_addr = range - usecountLeft;
                        symfield.everused = true;
                        symfield.value = -10;
                        symbol_map[sym] = symfield;
                    }
                    else{
                        it-> second.ex_addr = range - usecountLeft;
                    }

                    usecountLeft--;
                    if (usecountLeft == 0){
                        phase++;
                        addrcountExp = true;
                    }
                }
            }
            else{ // program text
                if (addrcountExp){
                    addrcountLeft = readInt(token); // i = sizeofmodule

                    if (addrcountLeft == 0){
                        phase++;
                        defcountExp  = true;
                        // uselist check       
                        warningcheck(1, module_num);   
                        // init the sym_table
                        clearUselist();
                    }
                    else{
                        //baseAddr += module_size;
                        relative_tmp = addrcountLeft;
                        codetypeExp = true;
                    }
                    addrcountExp = false;
                }
                else{ // pair expected
                    if (codetypeExp){
                        codetypeExp = false;
                        tmpIAER = token;
                    }
                    else{ //instr: 4 digit opcode check at pass two
                        int inst = readInt(token);
                        int opcode = inst/1000; 
                        int operand= inst%1000;
                        if (tmpIAER.compare("I") == 0){
                            if (opcode >= 10){
                                prettyPrinter(left_num, 9, 999, 6);
                            }
                            else{
                                prettyPrinter(left_num, opcode, operand, 0);
                            }
                            left_num++;
                        }
                        else{
                            if (opcode >= 10){
                            prettyPrinter(left_num, 9, 999, 7);
                            left_num++;
                            } 
                            else{
                                if (tmpIAER.compare("A")== 0){
                                    if (operand >= 512){
                                        prettyPrinter(left_num, opcode, 0, 1);
                                    }
                                    else{
                                        prettyPrinter(left_num, opcode, operand, 0);
                                    }
                                }
                                else if (tmpIAER.compare("E")==0){
                                    if (operand < range){
                                        for (auto it = symbol_map.begin(); it != symbol_map.end(); it++){
                                            if (it->second.ex_addr == operand){
                                                if (it->second.value != -10){
                                                    prettyPrinter(left_num,opcode, it->second.value, 0);
                                                    it->second.isUsed = true;
                                                    it->second.everused = true;
                                                }
                                                else{
                                                    prettyPrinter(left_num, opcode, 0, 4, it->first);
                                                    it->second.isUsed = true;
                                                }
                                            }
                                        }
                                    }
                                    else{
                                        prettyPrinter(left_num, opcode, operand, 3);
                                    }

                                }
                                else{ // R
                                    if (operand > relative_tmp){
                                        prettyPrinter(left_num,opcode, relative, 2);
                                    }
                                    else{
                                        prettyPrinter(left_num, opcode, operand + relative, 0);
                                    }
                                }
                                left_num++;
                            }

                        }
                        tmpIAER = "";

                        addrcountLeft--;
                        if (addrcountLeft == 0){
                            phase ++;
                            relative += relative_tmp;
                            range = 0;
                            warningcheck(1, module_num);
                            clearUselist();

                            defcountExp = true;
                        }
                        else{
                            codetypeExp = true;
                        }                      
                    }
                }
            }
            ptr = strtok(NULL, delimit);
        }
    }
    cout << endl;

    warningcheck(2, module_num);

    file.close();
}

int main(int argc, char* argv[]){
    
    PassOne(argv[1]);
    cout << endl;
    cout << "Memory Map" << endl;
    PassTwo(argv[1]);
    return 0;
}
