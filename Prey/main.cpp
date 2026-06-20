#include <iostream>
#include <termios.h>
#include <unistd.h>
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
int main(){
    fish fish;
    bool running=true;
    while(running){
        char key=getKey();
        if(key!=0)
        handleinput(key, fish, running);
        draw(fish);
        sleepMs(100);
    }
    return 0;
}
