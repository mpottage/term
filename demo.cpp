#include "Term.h"
#include <iostream>
#include <fstream>

struct Point {
    int x{0}, y{0};
    Point(int xx, int yy)
        :x{xx}, y{yy} {}
};

void mock_battle(Term& t)
{
    t.level_view.render(9,6,'x',Colour::magenta);
    t.refresh();
    auto r = t.get_long_response("Which direction to fire [hjkl]? ");
    if (r=="h" or r=="j" or r=="k" or r=="l") {
        t.push_message("You kill the grid bug.");
        t.push_message("Welcome to experience level 2.");
    }
    else
        t.push_message("Invalid direction.");
}

int main()
try {
    std::vector<std::string> test_grid;
    std::vector<Point> coins;
    std::vector<Point> doors;
    Point player{0,0};
    //Load level.
    std::ifstream is{"level1.txt"};
    is.imbue(std::locale{""});
    {
        std::string ln;
        getline(is,ln);//Drop first line.
        int y=0;
        while(getline(is,ln)) {
            test_grid.push_back(ln);
            for(int x=0; x<ln.size(); ++x) {
                if(ln[x]=='$')
                    coins.push_back(Point{x,y});
                else if(ln[x]=='+')
                    doors.push_back(Point{x,y});
                else if(ln[x]=='@') {
                    player = Point{x,y};
                    test_grid[y][x] = '.';
                }
            }
            ++y;
        }
    }
    //Run display.
    auto t = Term();
    t.push_message("Testing");
    t.push_message("Another");
    t.level_view.resize(test_grid);
    t.status_bar.add("Health");
    t.status_bar.add("£");
    t.status_bar.set("£","150");
    t.status_bar.set("Health","10/10",Colour::green);
    t.status_bar.set_title("Demo");
    for(int i=0; i<100; ++i) {
        t.status_bar.add(std::to_string(i));
        t.status_bar.set(std::to_string(i),"djk;lsklsldkjhgsjkhd£");
    }
    while(true) {
        t.level_view.clear();
        t.level_view.render(test_grid);
        for(auto p : coins)
            t.level_view.render(p.x,p.y,u8"£",Colour::yellow,true);
        for(auto p : doors)
            t.level_view.render(p.x,p.y,"+",Colour::yellow);
        t.level_view.render(player.x,player.y,'@',Colour::white,true);
        t.level_view.set_focus(player.x,player.y);
        t.refresh();
        auto key = t.get_key();
        while(t.messages_left()>1) {
            if(key==" " or key=="\n") {
                t.next_message();
                t.refresh();
            }
            else if(key=="q")
                break;
            key = t.get_key();
        }
        t.next_message(); //Clear last message..
        if((key=="l" or key=="Right") and player.x<t.level_view.width()-1)
            player.x+=1;
        else if((key=="h" or key=="Left") and player.x>0)
            player.x-=1;
        else if((key=="j" or key=="Down") and player.y<t.level_view.height()-1)
            player.y+=1;
        else if((key=="k" or key=="Up") and player.y>0)
            player.y-=1;
        else if(key=="b")
            mock_battle(t);
        else if(key=="q" or key=="Esc")
            break;
    }
}
catch (std::exception& e) {
    std::cerr<<e.what()<<'\n';
}
