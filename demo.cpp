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
    t.level_view().render_ch(9,6,'x',Colour::magenta);
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
{
    std::vector<std::string> test_grid;
    //Load level.
    std::ifstream is{"level1.txt"};
    is.imbue(std::locale{""});
    std::string ln;
    getline(is,ln);//Drop first line.
    while(getline(is,ln))
        test_grid.push_back(ln);
    //Run display.
    auto t = Term();
    t.push_message("Testing");
    t.push_message("Another");
    t.level_view().resize_to_grid(test_grid);
    int x{}, y{};
    bool bold = true;
    while(true) {
        t.level_view().clear();
        t.level_view().render_grid(test_grid);
        t.level_view().render_ch(x,y,'@',Colour::blue,bold);
        t.level_view().set_focus(x,y);
        t.refresh();
        t.next_message();
        auto key = t.get_key();
        while(t.any_messages()) {
            if(key==" " or key=="\n") {
                t.refresh();
                t.next_message();
            }
            else if(key=="q")
                break;
            key = t.get_key();
        }
        if((key=="l" or key=="Right") and x<t.level_view().width()-1)
            x+=1;
        else if((key=="h" or key=="Left") and x>0)
            x-=1;
        else if((key=="j" or key=="Down") and y<t.level_view().height()-1)
            y+=1;
        else if((key=="k" or key=="Up") and y>0)
            y-=1;
        else if(key=="b")
            mock_battle(t);
        else if(key=="B")
            bold = not bold;
        else if(key=="q")
            break;
    }
}
