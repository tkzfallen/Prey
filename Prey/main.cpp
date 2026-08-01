#include<iostream>
#include<fstream>
#include<termios.h>
#include<unistd.h>
#include<vector>
#include<ctime>
#include<cstdlib>
using namespace std;

// =========================================
//   SCREEN & TERMINAL UTILITIES
// =========================================
void clearScreen(){ cout<<"\033[H\033[J"; }
void hideCursor() { cout<<"\033[?25l"; }
void showCursor() { cout<<"\033[?25h"; }
void sleepMs(int ms){ usleep(ms*1000); }

struct termios orig;
void enableRaw(){
    tcgetattr(STDIN_FILENO, &orig);
    struct termios raw=orig;
    raw.c_lflag &= ~(ECHO|ICANON);
    raw.c_iflag &= ~(IXON|ICRNL);
    raw.c_cc[VMIN]=0;
    raw.c_cc[VTIME]=0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
void disableRaw(){ tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig); }

char getKey(){
    char c=0;
    read(STDIN_FILENO, &c, 1);
    return c;
}

// =========================================
//   CONSTANTS
// =========================================
const int width=40;
const int height=16;

// =========================================
//   STRUCTS
// =========================================
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

struct Item{
    int x, y;
    bool active;
    char symbol;
    int effect; // positive = heal, negative = damage
};

struct Predator{
    int x, y;
    bool active=true;
    char symbol='V';
    int moveDelay=8;
    int moveTimer=0;
};

struct EntityDef{
    string type;  // FOOD, POISON, PREDATOR
    char symbol;
    int value;    // restore or damage amount
    int speed;    // predators only
};

// =========================================
//   ECOSYSTEM FILE LOADER
// =========================================
vector<EntityDef> loadEcosystem(){
    vector<EntityDef> defs;
    ifstream file("ecosystem.txt");
    string line;
    while(getline(file, line)){
        if(line.empty()) continue;
        EntityDef def;
        def.type=line.substr(0, line.find(' '));

        size_t pos=line.find("symbol=");
        if(pos==string::npos) continue;
        def.symbol=line[pos+7];

        if(line.find("restore=")!=string::npos){
            pos=line.find("restore=");
            def.value=stoi(line.substr(pos+8));
        } else if(line.find("damage=")!=string::npos){
            pos=line.find("damage=");
            def.value=stoi(line.substr(pos+7));
        }

        def.speed=0;
        if(line.find("speed=")!=string::npos){
            pos=line.find("speed=");
            def.speed=stoi(line.substr(pos+6));
        }

        defs.push_back(def);
    }
    return defs;
}

// =========================================
//   SPAWN FUNCTIONS
// =========================================
vector<Item> spawnItems(const vector<EntityDef>& defs){
    vector<Item> items;
    for(const auto& def : defs){
        if(def.type=="FOOD")
            items.push_back({rand()%(width-2)+1, rand()%(height-2)+1, true, def.symbol, def.value});
        else if(def.type=="POISON")
            items.push_back({rand()%(width-2)+1, rand()%(height-2)+1, true, def.symbol, -def.value});
    }
    return items;
}

vector<Predator> spawnPredators(const vector<EntityDef>& defs, int day){
    vector<Predator> predators;
    int count=day+1;
    for(const auto& def : defs){
        if(def.type=="PREDATOR"){
            int speed=max(2, def.speed-day);
            for(int i=0; i<count; i++)
                predators.push_back({rand()%(width-2)+1, rand()%(height-2)+1, true, def.symbol, speed, 0});
        }
    }
    return predators;
}

// =========================================
//   DRAW
// =========================================
void draw(const fish& f, const vector<Item>& items, const vector<Predator>& predators, int day, int score, int timeLeft){
    // build grid
    vector<string> grid(height, string(width, ' '));

    // place items
    for(const auto& item : items)
        if(item.active)
            grid[item.y][item.x]=item.symbol;

    // place predators
    for(const auto& p : predators)
        if(p.active)
            grid[p.y][p.x]=p.symbol;

    // place fish on top
    grid[f.y][f.x]=f.symbol;

    // render
    clearScreen();
    cout<<"+"<<string(width, '-')<<"+\n";
    for(int i=0; i<height; i++)
        cout<<"|"<<grid[i]<<"|\n";
    cout<<"+"<<string(width, '-')<<"+\n";

    // HUD
    int filled=(f.health*20)/f.max_health;
    cout<<" HP: [";
    for(int i=0; i<20; i++)
        cout<<(i<filled ? '#' : '.');
    cout<<"] "<<f.health<<"/"<<f.max_health<<"\n";
    cout<<" Day: "<<day<<"   Time: "<<timeLeft<<"s   Score: "<<score<<"\n";
    cout<<" WASD = move   Q = quit\n";
    cout.flush();
}

// =========================================
//   INPUT & COLLISION
// =========================================
void handleinput(char key, fish& f, bool& running){
    int nx=f.x, ny=f.y;
    if(key=='w') ny--;
    else if(key=='s') ny++;
    else if(key=='a') nx--;
    else if(key=='d') nx++;
    else if(key=='q'){ running=false; return; }
    if(nx>=0 && nx<width && ny>=0 && ny<height){
        f.x=nx;
        f.y=ny;
    }
}

void checkItemCollision(fish& f, vector<Item>& items){
    for(auto& item : items){
        if(item.active && f.x==item.x && f.y==item.y){
            f.health+=item.effect;
            if(f.health>f.max_health) f.health=f.max_health;
            if(f.health<0) f.health=0;
            item.active=false;
        }
    }
}

void updatePredators(vector<Predator>& predators, const fish& f){
    for(auto& p : predators){
        if(!p.active) continue;
        p.moveTimer++;
        if(p.moveTimer<p.moveDelay) continue;
        p.moveTimer=0;
        if(f.x>p.x) p.x++;
        else if(f.x<p.x) p.x--;
        else if(f.y>p.y) p.y++;
        else if(f.y<p.y) p.y--;
    }
}

void checkPredatorCollision(fish& f, vector<Predator>& predators){
    for(auto& p : predators){
        if(p.active && f.x==p.x && f.y==p.y){
            f.health-=35;
            if(f.health<0) f.health=0;
            p.active=false;
        }
    }
}

// =========================================
//   PROFILE SYSTEM
// =========================================
profile loadProfiles(const string& username){
    ifstream file("profiles.txt");
    string line;
    while(getline(file, line)){
        if(line.empty()) continue;
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
    while(getline(fileIn, line))
        if(!line.empty()) lines.push_back(line);
    fileIn.close();

    bool found=false;
    for(auto& l : lines){
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
    for(const auto& l : lines)
        fileOut<<l<<"\n";
}

// =========================================
//   SCREENS
// =========================================
profile profileScreen(){
    disableRaw();
    clearScreen();
    cout<<"\n\n";
    cout<<"     +=====================================+\n";
    cout<<"     |                                     |\n";
    cout<<"     |              ~ Prey ~               |\n";
    cout<<"     |           Profile Login             |\n";
    cout<<"     |                                     |\n";
    cout<<"     +=====================================+\n\n";
    cout<<"     Enter your username: ";
    cout.flush();

    string username;
    cin>>username;

    profile profiles=loadProfiles(username);

    if(profiles.highscore==-1){
        // new user
        profiles={username, 0, 0};
        saveProfiles(profiles);
        clearScreen();
        cout<<"\n\n";
        cout<<"     +=====================================+\n";
        cout<<"     |                                     |\n";
        cout<<"     |  Profile created for "<<username<<"!\n";
        cout<<"     |  Good luck out there...             |\n";
        cout<<"     |                                     |\n";
        cout<<"     +=====================================+\n\n";
    } else {
        // returning user
        clearScreen();
        cout<<"\n\n";
        cout<<"     +=====================================+\n";
        cout<<"     |                                     |\n";
        cout<<"     |  Welcome back, "<<username<<"!\n";
        cout<<"     |                                     |\n";
        cout<<"     |  High Score   : "<<profiles.highscore<<"\n";
        cout<<"     |  Days Survived: "<<profiles.daysSurvived<<"\n";
        cout<<"     |                                     |\n";
        cout<<"     +=====================================+\n\n";
    }

    cout<<"     Press ENTER to continue...\n";
    cin.ignore();
    cin.get();
    enableRaw();
    return profiles;
}

void welcomeScreen(){
    disableRaw();
    clearScreen();
    cout<<"\n\n";
    cout<<"     +=====================================+\n";
    cout<<"     |                                     |\n";
    cout<<"     |              ~ Prey ~               |\n";
    cout<<"     |   ASCII Aquarium Survival Game      |\n";
    cout<<"     |                                     |\n";
    cout<<"     |   Survive each day to earn points.  |\n";
    cout<<"     |   Each day gets harder.             |\n";
    cout<<"     |                                     |\n";
    cout<<"     |   * = Food      (+20 HP)            |\n";
    cout<<"     |   X = Poison    (-15 HP)            |\n";
    cout<<"     |   V = Predator  (danger!)           |\n";
    cout<<"     |                                     |\n";
    cout<<"     |   WASD = Move   Q = Quit            |\n";
    cout<<"     |                                     |\n";
    cout<<"     |      Press ENTER to dive in...      |\n";
    cout<<"     |                                     |\n";
    cout<<"     +=====================================+\n\n";
    cout.flush();
    cin.ignore();
    cin.get();
    enableRaw();
}

void dayClearScreen(int day, int score){
    disableRaw();
    clearScreen();
    cout<<"\n\n";
    cout<<"     +=====================================+\n";
    cout<<"     |                                     |\n";
    cout<<"     |        Day "<<day<<" Survived!              |\n";
    cout<<"     |        Score: "<<score<<"                    |\n";
    cout<<"     |                                     |\n";
    cout<<"     |   Predators getting faster...       |\n";
    cout<<"     |                                     |\n";
    cout<<"     +=====================================+\n\n";
    cout<<"     Press ENTER to continue...\n";
    cin.ignore();
    cin.get();
    enableRaw();
}

void gameOverScreen(int score, int highscore){
    disableRaw();
    clearScreen();
    cout<<"\n\n";
    cout<<"     +=====================================+\n";
    cout<<"     |                                     |\n";
    cout<<"     |           ~ GAME OVER ~             |\n";
    cout<<"     |                                     |\n";
    cout<<"     |     Your Score : "<<score<<"\n";
    cout<<"     |     High Score : "<<highscore<<"\n";
    cout<<"     |                                     |\n";
    cout<<"     +=====================================+\n\n";
    cout<<"     Press ENTER to exit...\n";
    cin.ignore();
    cin.get();
    enableRaw();
}

// =========================================
//   MAIN
// =========================================
int main(){
    srand(time(0));

    // load ecosystem first, check it exists
    vector<EntityDef> defs=loadEcosystem();
    if(defs.empty()){
        cout<<"Error: ecosystem.txt not found or empty!\n";
        cout<<"Please create ecosystem.txt with FOOD, POISON and PREDATOR entries.\n";
        return 1;
    }

    enableRaw();
    hideCursor();

    profile profiles=profileScreen();
    welcomeScreen();

    fish fish;
    int day=1;
    int score=0;
    int daysSurvived=0;
    bool running=true;

    // outer loop: one iteration per day
    while(running){
        vector<Item> items=spawnItems(defs);
        vector<Predator> predators=spawnPredators(defs, day);

        int dayDuration=30;
        time_t dayStart=time(0);
        bool dayRunning=true;

        // inner loop: game loop for this day
        while(dayRunning && running){
            int timeLeft=dayDuration-(int)(time(0)-dayStart);

            char key=getKey();
            if(key!=0) handleinput(key, fish, running);

            checkItemCollision(fish, items);
            updatePredators(predators, fish);
            checkPredatorCollision(fish, predators);
            draw(fish, items, predators, day, score, max(0, timeLeft));

            // check game over
            if(fish.health<=0){
                if(score>profiles.highscore) profiles.highscore=score;
                profiles.daysSurvived+=daysSurvived;
                saveProfiles(profiles);
                gameOverScreen(score, profiles.highscore);
                running=false;
                dayRunning=false;
            }

            // check day cleared
            if(timeLeft<=0 && running){
                score+=100*day;
                daysSurvived++;
                fish.health=min(fish.max_health, fish.health+30); // bonus HP
                dayClearScreen(day, score);
                day++;
                dayRunning=false;
            }

            sleepMs(100);
        }
    }

    // save profile on normal quit (Q key)
    if(score>profiles.highscore) profiles.highscore=score;
    profiles.daysSurvived+=daysSurvived;
    saveProfiles(profiles);

    showCursor();
    disableRaw();
    return 0;
}