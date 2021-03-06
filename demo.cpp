#include "ui.h"
#include "utf8.h"
#include <iostream>
#include <fstream>

struct Point {
    int x{0}, y{0};
    Point(int xx, int yy)
        :x{xx}, y{yy} {}
};

void mock_battle(ui::Display& t)
{
    t.level_view().render(9,6,'x',ui::Colour::magenta);
    t.show_changes();
    auto r = t.get_answer("Which direction to [hjkl]? ");
    if (r=="h" or r=="j" or r=="k" or r=="l") {
        t.queue_message("You kill the grid bug.");
        t.queue_message("Welcome to experience level 2.");
    }
    else
        t.queue_message("Invalid direction.");
}

void show_inventory(ui::Display& t)
{
    t.set_show_overlay(true);
    t.list_overlay().first_page();
    while(true) {
        t.show_changes();
        auto k = t.get_key();
        if(k=="h" or k=="k")
            t.list_overlay().prev_page();
        else if(k=="j" or k=="l" or k=="\n")
            t.list_overlay().next_page();
        else
            break;
    }
    t.set_show_overlay(false);
}

int main()
try {
    std::vector<std::string> test_grid;
    std::vector<Point> coins;
    std::vector<Point> doors;
    Point player{0,0};
    //Load level.
    std::ifstream is{"demo_level.txt"};
    is.imbue(std::locale{""});
    {
        std::string ln;
        int y=0;
        while(getline(is,ln)) {
            test_grid.push_back(ln);
            int x=0;
            for(int i=0; i<ln.size(); ++x) {
                int offset = utf8::offset_next(ln[i]);
                std::string p = ln.substr(i,offset);
                if(p=="£")
                    coins.push_back(Point{x,y});
                else if(p=="+")
                    doors.push_back(Point{x,y});
                else if(p=="@") {
                    player = Point{x,y};
                    test_grid[y][x] = '.';
                }
                i += offset;
            }
            ++y;
        }
    }
    //Run display.
    auto t = ui::Display();
    t.queue_message("Welcome to the demo.cpp for ui::Display.");
    t.level_view().resize(test_grid);
    t.status_bar().add("Health");
    t.status_bar().add("£");
    t.status_bar().set("£","150");
    t.status_bar().set("Health","10/10",ui::Colour::green);
    t.status_bar().set_title("Demo");
    t.list_overlay().set_title("Inventory");
    t.list_overlay().push_heading("Consumables");
    t.list_overlay().push_item("a - Old cheese");
    t.list_overlay().push_item("b - Barley",ui::Colour::brown);
    t.list_overlay().push_heading("Kernel Modules");
    std::vector<std::string> itms{"ctr","ccm ","nls_iso8859_1","nls_cp437","uvcvideo","arc4"};
    for(int i=0; i<itms.size(); ++i)
        t.list_overlay().push_item(char('d'+i)+(" - "+itms[i]));
    t.list_overlay().push_heading("Weapons");
    t.list_overlay().push_item("j - Code cutter");
    t.list_overlay().push_heading("Extra");
    for(int i=0; i<15; ++i)
        t.list_overlay().push_item("? - Unrecognised item.");

    while(true) {
        t.level_view().clear();
        t.level_view().render(test_grid);
        for(auto p : coins)
            t.level_view().render(p.x,p.y,u8"£",ui::Colour::yellow);
        for(auto p : doors)
            t.level_view().render(p.x,p.y,"+",ui::Colour::brown);
        t.level_view().render(player.x,player.y,'@',ui::Colour::white);
        t.level_view().set_focus(player.x,player.y);
        t.show_changes();
        auto key = t.get_key();
        while(t.messages_count()>1) {
            if(key==" " or key=="\n") {
                t.next_message();
                t.show_changes();
            }
            else if(key=="q")
                break;
            key = t.get_key();
        }
        t.next_message(); //Clear last message..
        if((key=="l" or key=="Right") and player.x<t.level_view().width()-1)
            player.x+=1;
        else if((key=="h" or key=="Left") and player.x>0)
            player.x-=1;
        else if((key=="j" or key=="Down") and player.y<t.level_view().height()-1)
            player.y+=1;
        else if((key=="k" or key=="Up") and player.y>0)
            player.y-=1;
        else if(key=="b")
            mock_battle(t);
        else if(key=="i")
            show_inventory(t);
        else if(key==u8"£")
            t.queue_message("You have 150 coins.");
        else if(key=="#") //Trivial example of get_long_answer.
            t.status_bar().set_title(t.get_long_answer("# ",
                        [](const std::string& s) {return "demo";}));
        else if(key=="q" or key=="Esc")
            break;
    }
}
catch (std::exception& e) {
    std::cerr<<e.what()<<'\n';
}
