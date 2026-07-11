#include<iostream>
#include<fstream>
#include<termios.h>
#include<unistd.h>
#include<vector>
using namespace std;
void clearScreen(){
    cout<<"\033[H\033[J";
}
void hideCursor(){
    cout<<"\033[?25l";
}
void showCursor(){
    cout<<"\033[?25h";
}
void sleepMs(int ms){
    usleep(ms*1000);
}
struct termios orig;
void enableRaw(){
    tcgetattr(STDIN_FILENO, &orig);
    struct termios raw=orig;
    raw.c_lflag &= ~(ECHO|ICANON);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_cc[VMIN]=0;
    raw.c_cc[VTIME]=0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
void disableRaw(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}
char getKey(){
    char c=0;
    read(STDIN_FILENO, &c, 1);
    return c;
}
const int width=40;
const int height=16;
void draw(){
    clearScreen();
    cout<<"+"<<string(width, '-')<<"+\n";
    for(int i=0; i<height; i++){
        cout<<"|"<<string(width, '-')<<"|\n";
    }
    cout<<"+"<<string(width, '-')<<"+\n";
}
struct fish{
    int x=width/2;
    int y=height/2;
    int health=100;
    int max_health=100;
    char symbol='>';
};
struct profile{
    string username;
    int highscore;
    int daysSurvived;
};
void draw(const fish& fish){
    vector<string>grid(height, string(width, ' '));
    grid[fish.y][fish.x]=fish.symbol;
    clearScreen();
    cout<<"+"<<string(width, '-')<<"+\n";
    for(int i=0; i<height; i++){
        cout<<"|"<<grid[i]<<"|\n";
    }
    cout<<"+"<<string(width, '-')<<"+\n";
}
void handleinput(char key, fish& fish, bool& running){
    int nx=fish.x;
    int ny=fish.y;
    if(key=='w') ny--;
    else if(key=='s') ny++;
    else if(key=='a') nx--;
    else if(key=='d') nx++;
    else if(key=='q') {
        running=false;
        return;
    }
    if(nx>=0 && nx<width && ny>=0 && ny<height){
        fish.x=nx;
        fish.y=ny;
    }
}
void welcomeScreen(){
    clearScreen();
    cout << "\n\n";
    cout << "     +=====================================+\n";
    cout << "     |                                     |\n";
    cout << "     |                ~~ Prey ~~           |\n";
    cout << "     |                                     |\n";
    cout << "     |   A fish alone in a dangerous sea.  |\n";
    cout << "     |   Survive each day to earn points.  |\n";
    cout << "     |                                     |\n";
    cout << "     |   * = Food      (+20 HP)            |\n";
    cout << "     |   X = Poison    (-15 HP)            |\n";
    cout << "     |   V = Predator  (danger!)           |\n";
    cout << "     |                                     |\n";
    cout << "     |   WASD = Move   Q = Quit            |\n";
    cout << "     |                                     |\n";
    cout << "     |      Press ENTER to dive in...      |\n";
    cout << "     |                                     |\n";
    cout << "     +=====================================+\n";
    cout.flush();
    disableRaw();
    cin.get();
    enableRaw();
}
profile loadProfiles(const string& username){
    ifstream file("profiles.txt");
    string line;
    while(getline(file, line)){
        if(line.empty())
            continue;
        string storedUn=line.substr(0, line.find(','));
        if(storedUn==username){
            profile p;
            p.username=storedUn;
            string rest=line.substr(line.find(',')+1);
            p.highscore=stoi(rest.substr(0, rest.find(',')));
            p.daysSurvived=stoi(rest.substr(rest.find(',')+1));
            return p;
        }
    }
    return {"", -1, 0};
}
void saveProfiles(const profile& p){
    ifstream fileIn("profiles.txt");
    vector<string> lines;
    string line;
    while(getline(fileIn, line)){
        if(!line.empty())
            lines.push_back(line);
    }
    fileIn.close();
    bool found=false;
    for(auto& l:lines){
        string storedun=l.substr(0, l.find(','));
        if(storedun==p.username){
            l=p.username+","+to_string(p.highscore)+","+to_string(p.daysSurvived);
            found=true;
            break;
        }
    }
    if(!found)
        lines.push_back(p.username+","+to_string(p.highscore)+","+to_string(p.daysSurvived));
    ofstream fileOut("profiles.txt");
    for(const auto& l:lines){
        fileOut<<l<<"\n";
    }
}
profile profileScreen(){
    disableRaw();
    clearScreen();

    cout<<"\n\n";
    cout<<"+=====================================+\n";
    cout<<"|                                     |\n";
    cout<<"|               Prey                  |\n";
    cout<<"|           Profile Login             |\n";
    cout<<"|                                     |\n";
    cout<<"+=====================================+\n\n";
    cout<<"Enter your username:";
    cout.flush();
    string username;
    cin>>username;
    profile profiles=loadProfiles(username);
    if(profiles.highscore==-1){
        profiles={username, 0, 0};
        saveProfiles(profiles);
        clearScreen();
        cout << "\n\n";
        cout << "     +=====================================+\n";
        cout << "     |                                     |\n";
        cout << "     |  Profile created for " << username << "!\n";
        cout << "     |  Good luck out there...             |\n";
        cout << "     |                                     |\n";
        cout << "     +=====================================+\n\n";
    }
    else{
        clearScreen();
        cout << "\n\n";
        cout << "     +=====================================+\n";
        cout << "     |                                     |\n";
        cout << "     |  Welcome back, "<<username<<"!\n";
        cout << "     |                                     |\n";
        cout << "     |  High Score   : "<<profiles.highscore<<"\n";
        cout << "     |  Days Survived: "<<profiles.daysSurvived<<"\n";
        cout << "     |                                     |\n";
        cout << "     +=====================================+\n\n"; 
    }
    cout<<"Press Enter to continue...\n";
    cin.ignore();
    cin.get();
    enableRaw();
    return profiles;
}
int main(){
    enableRaw();
    hideCursor();
    profile profiles=profileScreen();
    welcomeScreen();
    fish fish;
    bool running=true;
    int day=1;
    int score=0;
    int daysSurvived=0;
    while(running){
        char key=getKey();
        if(key!=0)
        handleinput(key, fish, running);
        draw(fish);
        sleepMs(100);
    }
    if(score>profiles.highscore)
        profiles.highscore=score;
    profiles.daysSurvived+=daysSurvived;
    saveProfiles(profiles);
    showCursor();
    disableRaw();
    return 0;
}
