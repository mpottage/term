#include "Term.h"
#include "utf8.h"
#include <ncurses.h>
#include <algorithm>
#include <locale>
#include <codecvt>

static std::wstring_convert<std::codecvt_utf8<wchar_t>,wchar_t> utf8_wchar;

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
            throw Term_error{"Unable to initialise ncurses: cbreak"};
        res = keypad(stdscr, TRUE);
        if(res==ERR)
            throw Term_error{"Unable to initialise ncurses: keypad"};
        res = noecho();
        if(res==ERR)
            throw Term_error{"Unable to initialise ncurses: noecho"};
        res = set_escdelay(25);
        if(res==ERR)
            throw Term_error{"Unable to initialise ncurses: esc delay."};
        use_default_colors();
        res = start_color(); //Setup all colors.
        if(res==ERR)
            throw Term_error{"Unable to initialise ncurses with colors."};
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
void Term::push_message(const std::string msg)
{
    messages.push_back(utf8_wchar.from_bytes(msg));
}
void Term::refresh()
{
    clear();
    int width{0}, height{0};
    getmaxyx(stdscr,height,width); //MACRO, see ncurses documentation.
    show_message(width);
    status_bar.refresh(0,height-1,1,width);
    level_view.refresh(0,1,height-2,width);
    ::refresh();
}
void Term::show_message(int width)
{
    static const std::wstring more_text = L" --More--";
    if(any_messages()) {
        if(messages[0].size()+more_text.size()>width) {
            auto current_msg = messages[0];
            int max_ch = width-more_text.size();
            messages[0] = current_msg.substr(0,max_ch);
            messages.insert(messages.begin()+1,current_msg.substr(max_ch));
        }
        if(messages.size()>1)
            messages[0] += more_text;
        addwstr(messages[0].data());
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

void Level_view::resize(const std::vector<std::string>& grid)
{
    int max_len = 0;
    for(const auto& s : grid)
        if(utf8::size(s)>max_len)
            max_len = s.size();
    resize(grid.size(),max_len);
}
void Level_view::render(const std::vector<std::string>& grid)
{
    for(int y=0; y<grid.size(); ++y) {
        int line_start = y*m_width;
        std::wstring ln = utf8_wchar.from_bytes(grid[y].data());
        for(int x=0; x<ln.size(); ++x) {
            m_grid.at(line_start+x) = ln[x];
            m_attribs.at(line_start+x) = A_NORMAL;
        }
    }
}
void Level_view::render(int x, int y, char ch, Colour c, bool bold)
{
    wchar_t wch = utf8_wchar.from_bytes(ch)[0];
    render(x,y,wch,c,bold);
}
void Level_view::render(int x, int y, const std::string& ch, Colour c,
        bool bold)
{
    if(utf8::size(ch)>1)
        throw Term_error{"Too long std::string for Level_view::render."};
    wchar_t wch = utf8_wchar.from_bytes(ch)[0];
    render(x,y,wch,c,bold);
}
void Level_view::render(int x, int y, wchar_t ch, Colour c, bool bold)
{
    int position = y*m_width+x;
    m_grid.at(position) = ch;
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
    if(m_width>width and focus_x>width/2) {
        if(focus_x>=m_width-width/2)
            start_x = m_width-width;
        else
            start_x = focus_x-width/2;
    }
    if(m_height>height and focus_y>height/2) {
        if(focus_y>m_height-height/2)
            start_y = m_height-height;
        else
            start_y = focus_y-height/2;
    }
    int max_x{std::min(start_x+width,m_width)};
    int max_y{std::min(start_y+height,m_height)};
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
    move(focus_y-start_y+screen_min_y,focus_x-start_x+screen_min_x);
}


void Status_bar::add(const std::string& name)
{
    m_stats.push_back(Stat{});
    m_stats.back().name = utf8_wchar.from_bytes(name);
    m_stats.back().value_attrib = A_NORMAL;
}
void Status_bar::set(const std::string& name, const std::string& value,
        Colour c)
{
    auto w_name = utf8_wchar.from_bytes(name);
    auto st = find_if(begin(m_stats),end(m_stats),
            [&w_name](const Stat& s) { return s.name==w_name; });
    if(st==m_stats.end())
        throw Term_error{"Unknown statistic being set on Status_bar."};
    st->value = utf8_wchar.from_bytes(value);
    if(c!=Colour::normal)
        st->value_attrib = A_BOLD|COLOR_PAIR(static_cast<int>(c));
    else
        st->value_attrib = A_NORMAL;
}
void Status_bar::refresh(int x, int y, int height, int width)
{
    int pos = 0;
    static const std::wstring item_spacer{L"  "};
    static const std::wstring value_gap{L": "};
    move(y,x);
    clrtoeol();
    for(const auto& stat : m_stats) {
        int new_pos = pos+stat.name.size()+value_gap.size()
            +stat.value.size()+item_spacer.size();
        if(new_pos>=width)
            break;
        addwstr(stat.name.data());
        addwstr(value_gap.data());
        attron(stat.value_attrib);
        addwstr(stat.value.data());
        attroff(stat.value_attrib);
        addwstr(item_spacer.data());
    }
}
