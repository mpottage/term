#include "ui.h"
#include "utf8.h"
#include <ncurses.h>
#include <algorithm>
#include <locale>
#include <codecvt>
#include <cmath>
using namespace ui;

//Convert std::string to std::wstring.
static std::wstring_convert<std::codecvt_utf8<wchar_t>,wchar_t> utf8_wchar;

//Generate ncurses attribute for a colour.
inline static int colour_attrib(Colour c) {
    int c_no = static_cast<int>(c);
    //Relies on (for the first 8 colours) that the integer value of the enum is
    //  the same as the colour pair number in ncurses.
    if(c_no<9)
        return COLOR_PAIR(c_no);
    else if(c_no<17)
        return COLOR_PAIR(c_no-8)|A_BOLD;
    else
        throw ui::Exception{"ui: Unsupported colour found ("+std::to_string(c_no)+")."};
}

Display::Display()
{
    setlocale(LC_ALL, ""); //Set locale.
    int res;
    initscr();
    if(res==ERR)
        throw ui::Exception{"Unable to initialise ncurses"};
    try {
        res = cbreak();
        if(res==ERR)
            throw ui::Exception{"Unable to initialise ncurses: cbreak"};
        res = keypad(stdscr, TRUE);
        if(res==ERR)
            throw ui::Exception{"Unable to initialise ncurses: keypad"};
        res = noecho();
        if(res==ERR)
            throw ui::Exception{"Unable to initialise ncurses: noecho"};
        res = set_escdelay(25);
        if(res==ERR)
            throw ui::Exception{"Unable to initialise ncurses: esc delay."};
        use_default_colors();
        res = start_color(); //Setup all colors.
        if(res==ERR)
            throw ui::Exception{"Unable to initialise ncurses with colors."};
        for(int i=0; i<8; ++i)
            //Value of i+1 is same as the value for the colour in the enum.
            init_pair(i+1,i,-1);
    }
    catch(...) {
        endwin();
        throw;
    }
}
Display::~Display()
{
    //Close curses.
    endwin();
}
void Display::queue_message(const std::string msg)
{
    messages.push_back(utf8_wchar.from_bytes(msg));
}
void Display::show_changes()
{
    clear();
    int width{0}, height{0};
    getmaxyx(stdscr,height,width); //MACRO, see ncurses documentation.
    show_message(width);
    m_status_bar.refresh(0,height-1,1,width);
    m_level_view.refresh(0,1,height-2,width);
    if(m_show_overlay)
        m_list_overlay.refresh(0,0,height-1,width);
    ::refresh();
}
void Display::show_message(int width)
{
    static const std::wstring more_text = L" --More--";
    static const std::wstring ellipse = L"...";
    if(not messages.empty()) {
        if(messages[0].size()+more_text.size()>width) {
            auto current_msg = messages[0];
            int max_ch = width-more_text.size()-ellipse.size();
            messages[0] = current_msg.substr(0,max_ch)+ellipse;
            messages.insert(messages.begin()+1,current_msg.substr(max_ch));
        }
        if(messages.size()>1)
            messages[0] += more_text;
        addwstr(messages[0].data());
    }
}
std::string Display::get_key()
{
    static_assert(KEY_MIN>255, "Unable to read UTF-8 input (if any)");
    int ch = getch();
    if(ch>127 and ch<=255) { //If part of a UTF-8 multi-byte sequence.
        //Read UTF-8 input.
        std::string res{static_cast<char>(ch)};
        int mb_size = utf8::offset_next(res[0]);
        for(int i=1; i<mb_size; ++i) {
            res += static_cast<char>(getch());
        }
        return res;
    }
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
std::string Display::get_answer(std::string msg)
{
    move(0,0);
    clrtoeol();
    std::wstring w_msg = utf8_wchar.from_bytes(msg);
    addwstr(w_msg.data());
    return get_key();
}
std::string Display::get_long_answer(std::string prompt,
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
        if(31<ch and ch<256) { //Goes up to 256 for UTF-8 multibyte sequences.
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
            m_attribs.at(line_start+x) = 0;
        }
    }
}
void Level_view::render(int x, int y, char ch, Colour c)
{
    wchar_t wch = utf8_wchar.from_bytes(ch)[0];
    render(x,y,wch,c);
}
void Level_view::render(int x, int y, const std::string& ch, Colour c)
{
    if(utf8::size(ch)>1)
        throw ui::Exception{"Too long std::string for Level_view::render (required length is 1)."};
    wchar_t wch = utf8_wchar.from_bytes(ch)[0];
    render(x,y,wch,c);
}
void Level_view::render(int x, int y, wchar_t ch, Colour c)
{
    int position = y*m_width+x;
    m_grid.at(position) = ch;
    m_attribs.at(position) = colour_attrib(c);
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
    m_stats.back().value_attrib = 0;
}
void Status_bar::set(const std::string& name, const std::string& value,
        Colour c)
{
    auto w_name = utf8_wchar.from_bytes(name);
    auto st = find_if(begin(m_stats),end(m_stats),
            [&w_name](const Stat& s) { return s.name==w_name; });
    if(st==m_stats.end())
        throw ui::Exception{"Unknown statistic being set on Status_bar."};
    st->value = utf8_wchar.from_bytes(value);
    st->value_attrib = colour_attrib(c);
}
void Status_bar::set_title(const std::string& title)
{
    m_title = utf8_wchar.from_bytes(title);
}
void Status_bar::refresh(int x, int y, int height, int width)
{
    static const std::wstring item_spacer{L"  "};
    static const std::wstring value_gap{L": "};
    move(y,x);
    clrtoeol();
    int pos = 0;
    if(not m_title.empty() and m_title.size()<width) {
        pos += m_title.size()+item_spacer.size();
        addwstr(m_title.data());
        addwstr(item_spacer.data());
    }
    for(const auto& stat : m_stats) {
        int new_pos = pos+stat.name.size()+value_gap.size()
            +stat.value.size()+item_spacer.size();
        if(new_pos>=width)
            break;
        pos = new_pos;
        //Insert text on screen.
        addwstr(stat.name.data());
        addwstr(value_gap.data());
        attron(stat.value_attrib);
        addwstr(stat.value.data());
        attroff(stat.value_attrib);
        addwstr(item_spacer.data());
    }
}

void List_overlay::push_item(const std::string& s, Colour c)
{
    items.push_back(Item{utf8_wchar.from_bytes(s),colour_attrib(c)});
}
void List_overlay::push_heading(const std::string& s)
{
    items.push_back(Item{L" "+utf8_wchar.from_bytes(s)+L" ",A_STANDOUT});
}
void List_overlay::set_title(const std::string& s)
{
    m_title = L" "+utf8_wchar.from_bytes(s)+L" ";
}
void List_overlay::refresh(int x, int y, int height, int width)
{
    if(items.empty())
        throw ui::Exception{"List_overlay has no items to display."};
    int max_len = 0;
    for(const Item& i : items)
        if(i.value.size()>max_len)
            max_len = i.value.size();
    int indent = max_len<width? (width-max_len)/2 : 1;
    int title_indent = (width-indent-m_title.size())/2;
    int page_height = height-2; //Excluding title and page count.
    int page_count = std::ceil(items.size()/double(page_height));
    if(m_page>page_count) //If attempting to show a non-existent page.
        m_page = page_count;
    m_on_last_page = page_count==m_page;
    int start_ln{0}, end_ln{static_cast<int>(items.size())};
    if(page_count>1) {
        start_ln = (m_page-1)*page_height;
        end_ln = std::min(end_ln, m_page*page_height);
    }
    //Clear background.
    auto end_screen_ln = end_ln-start_ln+2;
    for(int i=0; i<end_screen_ln; ++i) {
        move(i,indent-1);
        clrtoeol();
    }
    if(m_title.size()>0 and title_indent>=0) {
        move(0,indent+title_indent);
        attron(A_STANDOUT|A_BOLD);
        addwstr(m_title.data());
        attroff(A_STANDOUT|A_BOLD);
    }
    for(int i=start_ln; i<end_ln; ++i) {
        auto ln = items[i].value;
        if(ln.size()>width-2)
            ln = ln.substr(0,width-5)+L"...";
        move(i-start_ln+1,indent);
        attron(items[i].attrib);
        addwstr(ln.data());
        attroff(items[i].attrib);
    }
    std::wstring page_detail = L"(page "+std::to_wstring(m_page)+L" of "+
        std::to_wstring(page_count)+L")";
    move(end_screen_ln-1,width-1-page_detail.size());
    addwstr(page_detail.data());
}
