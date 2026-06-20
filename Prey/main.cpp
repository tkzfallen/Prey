#include <iostream>
#include <termios.h>
#include <unistd.h>
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
int main(){
    draw();
}
