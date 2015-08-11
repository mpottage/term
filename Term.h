//Wrapper around ncurses to simplify usage.
//  Interally mainly std::wstring is used. This is to support unicode output
//  (ncurses uses wchar_t for it, not char).
//  The public API is std::string (UTF-8) to simplify usage, as the wchar_t is
//   only needed by ncurses.
#include <vector>
#include <string>
#include <stdexcept>
#include <functional>

enum class Colour {
    normal, black, red, green, yellow, blue, magenta, cyan, white
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
    void render(int x, int y, char ch, Colour c=Colour::normal,
            bool bold=false);
    void render(int x, int y, const std::string& ch, Colour c=Colour::normal,
            bool bold=false);
    void render(int x, int y, wchar_t ch, Colour c=Colour::normal,
            bool bold=false);
    //Sets position of blinking cursor.
    void set_focus(int x, int y)
    {   focus_x = x; focus_y = y;   }
    //Clear level view contents (retains size).
    void clear();
    //Draw to screen. It is recommended that only Term calls this.
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
    void clear()
    {   m_stats.clear();  }
    void refresh(int x, int y, int height, int width);
private:
    struct Stat {
        std::wstring name;
        std::wstring value;
        int value_attrib;
    };
    std::vector<Stat> m_stats;
};

class Term {
public:
    Term();
    ~Term();
    Term(const Term&) = delete;
    Term(Term&&) = default;
    Term& operator=(Term&) = delete;
    Term& operator=(Term&&) = default;
    //Queue a message.
    void push_message(std::string msg);
    //Are any messages queued.
    bool any_messages() const
    {   return messages.size()>0;   }
    //Move onto the next message.
    void next_message()
    {
        if(any_messages())
            messages.erase(messages.begin());
    }
    //Display Term to screen (includes Level_view).
    void refresh();
    //Get a key press.
    // All keys that produce printable output are returned as they are.
    // Escape is "Esc" and the arrow keys are "Up", "Down", "Left" and "Right".
    // Ctrl+X returns "^X".
    std::string get_key();
    std::string get_response(std::string msg);
    std::string get_long_response(std::string prompt="# ",
            std::function<std::string(std::string)> autocompleter={});
        //The string returned by autocompleter (if it starts with the input) is
        //used as the autocompletion and returned if the user presses [ENTER].

    Level_view level_view;
    Status_bar status_bar;
private:
    void show_message(int max_width);
    std::vector<std::wstring> messages;
};

class Term_error : std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
