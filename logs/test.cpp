#include<iostream>
#include<fstream>
#include<sstream>
#include<cstring>
#include<cassert>
#include<vector>
using namespace std;
int main() {
    ifstream infile;
    vector<string> filename1 = {"fastfair_nvm.log", "pgm_nvm.log",
     "alex_nvm.log", "xindex_nvm.log", "learngroup_nvm.log"};
    vector<string> filename2 = {"fastfair_dram.log", "pgm_dram.log",
     "alex_dram.log", "xindex_dram.log", "learngroup_dram.log"};
    vector<string> filename3 = {"1.log", "2.log",
     "3.log", "4.log", "5.log"};
    vector<vector<string> > delays(5);
    ofstream outfile;
    outfile.open("statistics.dat", ios::out | ios::trunc);
    outfile << "num fastfair pgm alex xindex learngroup" << endl;
    for(int i = 0; i < 5; i++) {
        infile.open(filename2[i]);
        string line;
        while(getline(infile, line)) {
            stringstream input(line);
            string res;
            int count = 0;
            while(input >> res) {
                count++;
                if(count == 1 && res != "op") break;
                if(count == 2 && res == "0") break;
                if(count != 6) continue;
                float val = stof(res);
                val /= 1000;
                res = to_string(val);
                delays[i].push_back(res);
            }
        }
        infile.close();
    }
    assert((delays[0].size() == delays[1].size()) &&
        (delays[0].size() == delays[2].size()) &&
        (delays[0].size() == delays[3].size()) &&
        (delays[0].size() == delays[4].size()));
    for(int i = 0; i < delays[0].size(); i++) {
        float j = (float)i/100;
        outfile << j;
        for(int j = 0; j < 5; j++) {
            outfile << " " << delays[j][i];
            
        }
        outfile << endl;
    }

}
