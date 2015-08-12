//Wrapper around ncurses to simplify usage.
//  Interally mainly std::wstring is used. This is to support unicode output
//  (ncurses uses wchar_t for it, not char).
//  The public API is std::string (UTF-8) to simplify usage, as the wchar_t is
//   only needed by ncurses.
#include <vector>
#include <string>
#include <stdexcept>
#include <functional>

namespace ui {

enum class Colour {
    normal, black, red, green, brown, dark_blue, purple, cyan, grey,
    dark_grey, light_red, light_green, yellow, blue, magenta, light_blue, white
};

class Level_view {
public:
    Level_view() = default;
    void resize(int height, int width)
    {
        m_width = width;
        m_height = height;
        int new_size = m_height*m_width;
        m_grid.resize(new_size,' ');
        m_attribs.resize(new_size,0);
    }
    void resize(const std::vector<std::string>& grid);
    void render(const std::vector<std::string>& grid);
    void render(int x, int y, char ch, Colour c=Colour::normal);
    void render(int x, int y, const std::string& ch, Colour c=Colour::normal);
    void render(int x, int y, wchar_t ch, Colour c=Colour::normal);
    //Sets position of blinking cursor.
    void set_focus(int x, int y)
    {   focus_x = x; focus_y = y;   }
    //Clear level view contents (retains size).
    void clear();
    //Draw to screen. It is recommended that only Display calls this.
    void refresh(int screen_min_x, int screen_min_y, int height, int width);
    //Returns dimensions as previously set.
    int width() const
    {   return m_width; }
    int height() const
    {   return m_height; }
private:
    std::vector<wchar_t> m_grid; //Uses wchar_t for ncurses.
    std::vector<int> m_attribs; //Display attributes (bold, color, etc.).
    int m_width{0}, m_height{0};
    int focus_x{0}, focus_y{0};
};

class Status_bar {
public:
    Status_bar() = default;
    void add(const std::string& name);
    void set(const std::string& name, const std::string& value,
            Colour value_c=Colour::normal);
    void set_title(const std::string& title);
    void clear()
    {   m_stats.clear();  }
    void refresh(int screen_x, int screen_y, int height, int width);
private:
    struct Stat {
        std::wstring name;
        std::wstring value;
        int value_attrib;
    };
    std::vector<Stat> m_stats;
    std::wstring m_title;
};

class List_overlay {
public:
    //Items are displayed in the order added.
    void push_item(const std::string& s, Colour c=Colour::normal);
    void push_heading(const std::string& s);
    void set_title(const std::string& s);
    //Change which page is displayed.
    void next_page()
    {   ++m_page; }
    void prev_page()
    {   if(m_page>1) --m_page;  }
    void first_page()
    {   m_page = 1;   }
    bool on_last_page() const
    {   return m_on_last_page;  }

    void refresh(int screen_x, int screen_y, int height, int width);
private:
    struct Item {
        std::wstring value;
        int attrib;
    };
    std::vector<Item> items;
    std::wstring m_title;
    int m_page{0};
    bool m_on_last_page{false};
};

class Display {
public:
    Display();
    ~Display();
    Display(const Display&) = delete;
    Display(Display&&) = default;
    Display& operator=(Display&) = delete;
    Display& operator=(Display&&) = default;

    void queue_message(std::string msg);
    //Number of messages queued.
    int messages_count() const
    {   return messages.size();   }
    //Move onto the next message.
    void next_message()
    {
        if(not messages.empty())
            messages.erase(messages.begin());
    }
    void set_show_overlay(bool show)
    {   m_show_overlay = show;  }
    //Shows all changes made. Must be called to display them.
    void show_changes();
    //Get a key press.
    // All keys that produce printable output are returned as they are.
    // Escape is "Esc" and the arrow keys are "Up", "Down", "Left" and "Right".
    // Ctrl+X returns "^X".
    std::string get_key();
    std::string get_answer(std::string msg);
    std::string get_long_answer(std::string prompt="# ",
            std::function<std::string(std::string)> autocompleter={});
        //The string returned by autocompleter (if it starts with the input) is
        //used as the autocompletion and returned if the user presses [ENTER].

    Level_view& level_view()
    {   return m_level_view;    }
    Status_bar& status_bar()
    {   return m_status_bar;    }
    List_overlay& list_overlay()
    {   return m_list_overlay;  }
private:
    void show_message(int max_width);
    std::vector<std::wstring> messages;
    bool m_show_overlay{false};
    Level_view m_level_view;
    List_overlay m_list_overlay;
    Status_bar m_status_bar;
};

class Exception : std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

}//End namespace ui.
