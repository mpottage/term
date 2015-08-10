#include "Term.h"
#include <ncurses.h>
#include <algorithm>
#include <locale>
#include <codecvt>

static std::wstring_convert<std::codecvt_utf8<wchar_t>,wchar_t> wchar_conv;

Term::Term()
{
    setlocale(LC_ALL, ""); //Set locale.
    int res;
    initscr();
    if(res==ERR)
        throw Term_error{"Unable to initialise ncurses"};
    try {
        res = cbreak();
        if(res==ERR)
            throw Term_error{"Unable to initialise ncurses"};
        res = keypad(stdscr, TRUE);
        if(res==ERR)
            throw Term_error{"Unable to initialise ncurses"};
        res = noecho();
        if(res==ERR)
            throw Term_error{"Unable to initialise ncurses"};
        res = set_escdelay(25);
        if(res==ERR)
            throw Term_error{"Unable to initialise ncurses"};
        use_default_colors();
        res = start_color(); //Setup all colors.
        if(res==ERR)
            throw Term_error{"Unable to initialise ncurses"};
        for(int i=0; i<8; ++i)
            //Value of i+1 is same as the value for the colour in the enum.
            init_pair(i+1,i,-1);
    }
    catch(...) {
        endwin();
        throw;
    }
}
Term::~Term()
{
    //Close curses.
    endwin();
}
void Term::refresh()
{
    clear();
    int width{0}, height{0};
    getmaxyx(stdscr,height,width); //MACRO, see ncurses documentation.
    show_message(width);
    l_level_view.refresh(0,1,height-2,width);
    ::refresh();
}
void Term::show_message(int width)
{
    static const std::string more_text = " --More--";
    if(any_messages()) {
        if(messages[0].size()+more_text.size()>width) {
            auto current_msg = messages[0];
            int max_ch = width-more_text.size();
            messages[0] = current_msg.substr(0,max_ch);
            messages.insert(messages.begin()+1,current_msg.substr(max_ch));
        }
        if(messages.size()>1)
            messages[0] += more_text;
        addstr(messages[0].data());
    }
}
std::string Term::get_key()
{
    int ch = getch();
    //Some special cases for neater names.
    switch(ch) {
    case '\n':
        return "\n";
    case 27:
        return "Esc";
    case 127:
        return "Del";
    case KEY_UP:
        return "Up";
    case KEY_DOWN:
        return "Down";
    case KEY_LEFT:
        return "Left";
    case KEY_RIGHT:
        return "Right";
    default:
        return keyname(ch);
    }
}
std::string Term::get_response(std::string msg)
{
    move(0,0);
    clrtoeol();
    addstr(msg.data());
    return get_key();
}
std::string Term::get_long_response(std::string prompt,
        std::function<std::string(std::string)> autocompleter)
{
    move(0,0);
    clrtoeol();
    addstr(prompt.data());
    std::string res;
    std::string completed;
    while(true) {
        move(0,prompt.size());
        clrtoeol();
        if(not completed.empty()) {
            addstr(completed.data());
            move(0,prompt.size()+res.size());
        }
        else
            addstr(res.data());
        int ch = getch();
        if(31<ch and ch<127) {
            res += char(ch);
            if(autocompleter) {
                completed = autocompleter(res);
                if(completed.find(res)!=0)
                    completed = "";
            }
        }
        else if(ch=='\n')
            break;
        else if(ch==KEY_BACKSPACE or ch==KEY_DC or ch==127) {
            if(res.size()>0)
                res.erase(res.size()-1);
            completed = "";
        }
    }
    return res;
    return completed.empty()?res:completed;
}

void Level_view::resize_to_grid(const std::vector<std::string>& grid)
{
    int max_len = 0;
    for(const auto& s : grid)
        if(s.size()>max_len)
            max_len = s.size();
    resize(grid.size(),max_len);
}
void Level_view::render_grid(const std::vector<std::string>& grid)
{
    for(int y=0; y<grid.size(); ++y) {
        int line_start = y*m_width;
        std::wstring ln = wchar_conv.from_bytes(grid[y].data());
        for(int x=0; x<ln.size(); ++x) {
            m_grid.at(line_start+x) = ln[x];
            m_attribs.at(line_start+x) = A_NORMAL;
        }
    }
}
void Level_view::render_ch(int x, int y, char ch, Colour c, bool bold)
{
    wchar_t wide_ch = wchar_conv.from_bytes(ch)[0];
    int position = y*m_width+x;
    m_grid.at(position) = wide_ch;
    m_attribs.at(position) = A_NORMAL|COLOR_PAIR(static_cast<int>(c));
    if(bold)
        m_attribs[position] |= A_BOLD;
}
void Level_view::clear()
{
    for(int i=0; i<m_grid.size(); ++i) {
        m_grid[i] = ' ';
        m_attribs[i] = 0;
    }
}
void Level_view::refresh(int screen_min_x,
        int screen_min_y, int height, int width)
{
    int start_x{0}, start_y{0};
    int grid_height = m_grid.size()/m_width;
    if(grid_height>width and focus_x>width/2) {
        if(focus_x>=m_width-width/2)
            start_x = m_width-width;
        else
            start_x = focus_x-width/2;
    }
    if(grid_height>height and focus_y>height/2) {
        if(focus_y>grid_height-height/2)
            start_y = grid_height-height;
        else
            start_y = focus_y-height/2;
    }
    int max_x{std::min(start_x+width,m_width)};
    int max_y{std::min(start_y+height,grid_height)};
    for(int y=start_y; y<max_y; ++y) {
        const int screen_y = y-start_y+screen_min_y;
        move(screen_y,screen_min_x);
        for(int x=start_x; x<max_x; ++x) {
            int position = y*m_width+x;
            attron(m_attribs[position]);
            //Place UTF-8 character on the screen.
            cchar_t c{};
            c.chars[0] = m_grid[position];
            add_wch(&c);
            attroff(m_attribs[position]);
        }
    }
    move(focus_y+screen_min_y,focus_x+screen_min_x);
}
