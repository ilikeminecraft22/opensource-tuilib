#pragma once

#ifndef __linux__
#error "OSTL currently only supports Linux" // what an idiot made this
//                                             oh yeah i forgot i made this
#endif

#include <vector>
#include <string>
#include <iostream>
#include <cstdint>
#include <functional>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

namespace OSTL // if this was in a competition for the most fragile library it would get 2nd place cus it would break mid round
{

    // -------------------- ANSI COLORS --------------------

    enum class AnsiFg : std::uint8_t
    {
        black = 30,
        red = 31,
        green = 32,
        yellow = 33,
        blue = 34,
        magenta = 35,
        cyan = 36,
        white = 37,

        bright_black = 90,
        bright_red = 91,
        bright_green = 92,
        bright_yellow = 93,
        bright_blue = 94,
        bright_magenta = 95,
        bright_cyan = 96,
        bright_white = 97
    };

    enum class AnsiBg : std::uint8_t
    {
        black = 40,
        red = 41,
        green = 42,
        yellow = 43,
        blue = 44,
        magenta = 45,
        cyan = 46,
        white = 47,

        bright_black = 100,
        bright_red = 101,
        bright_green = 102,
        bright_yellow = 103,
        bright_blue = 104,
        bright_magenta = 105,
        bright_cyan = 106,
        bright_white = 107
    };

    // -------------------- STYLE --------------------

    struct Style
    {
        AnsiFg fg = AnsiFg::white;
        AnsiBg bg = AnsiBg::black;

        bool bold = false;
        bool underline = false;
        bool dim = false;
    };

    // -------------------- ANSI HELPERS --------------------

    inline std::string sgr(const Style &s) // no idea what this does tbh. it works, not my problem.
    {
        std::string out = "\033[";
        bool first = true;

        auto add = [&](int v)
        {
            if (!first)
                out += ";";
            out += std::to_string(v);
            first = false;
        };

        add(static_cast<int>(s.fg));
        add(static_cast<int>(s.bg));

        if (s.bold)
            add(1);
        if (s.dim)
            add(2);
        if (s.underline)
            add(4);

        out += "m";
        return out;
    }

    inline std::string reset()
    {
        return "\033[0m";
    }

    // -------------------- INPUT --------------------

    class RawMode
    {
        termios oldt{};

    public:
        RawMode()
        {
            tcgetattr(STDIN_FILENO, &oldt);

            termios newt = oldt;
            newt.c_lflag &= ~(ICANON | ECHO);

            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        }

        ~RawMode()
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        }
    };

    inline char getch() // dont use because its bad instead use KeyboardInput::poll()
    {
        char ch;
        read(STDIN_FILENO, &ch, 1);
        return ch;
    }

    //   V----------- so the lib doesnt fuck itself up
    struct Coords;
    struct Cell;
    class Buffer;
    class Button;
    class ButtonSelector;
    class Checkbox;
    class CheckboxSelector;
    class Label;
    class Element;
    class UIElement;
    class InteractiveUIElement;

    // -------------------- CORE TYPES --------------------

    struct Cell
    {
        char ch = ' ';
        Style style{};
    };

    struct Size2D
    {
        int w;
        int h;
    };

    struct Coords
    {
        int x;
        int y;
    };

    struct KeyEvent
    {
        char key;
    };

    class Element
    {
    protected:
        Coords _pos;
        Buffer *_parent;

    public:
        Element(Coords pos, Buffer *parent) : _pos(pos), _parent(parent) {}
        Coords get_pos() // gets position
        {
            return _pos;
        }
        void set_pos(Coords pos) // sets absolute position
        {
            _pos = pos;
        }
    };

    class KeyboardInput
    {
    protected:
        std::vector<KeyEvent> events;

    public:
        void apply_backspace(std::string &s) // i have no idea what is this
        {
            if (!s.empty())
                s.pop_back();
        }
        void poll() // basically getch() like on windows(dont use the other getch in the library instead use this)
        {
            char c = getch();
            events.push_back({c});
        }
        std::vector<KeyEvent> consume_events() // uhh clears the key queue and returns it
        {
            std::vector<KeyEvent> out;
            out.swap(events);
            return out;
        }
        std::string n_scan(int n) // scans a number of characters
        {
            std::string s;
            char c;

            while ((int)s.size() < n)
            {
                this->poll();
                c = consume_events().back().key;

                if (c == '\b' || c == 127)
                {
                    if (!s.empty())
                        s.pop_back();
                }
                else
                {
                    s += c;
                }
            }

            return s;
        }
        std::string s_scan(char stop) // scans characters until it gets a certain key
        {
            std::string s;
            char c;
            this->poll();
            while ((this->consume_events().back().key) != stop)
            {
                if (c == '\b' || c == 127)
                {
                    if (!s.empty())
                        s.pop_back();
                }
                else
                {
                    s += c;
                }
                this->poll();
            }

            return s;
        }
    };

    class UIElement : public Element
    {
    protected:
        Element *_guard;
        KeyboardInput *_keyboard;

    public:
        UIElement(Coords pos, Buffer *parent, Element *guard) : Element(pos, parent), _guard(guard)
        {
            if (guard == nullptr)
            {
                std::cerr << "ELEMENT POINTER IS NULLPTR!\n";
                exit(1);
            }
            set_r_pos(this->get_pos());
        }
        void set_r_pos(Coords dpos) // sets a position relative to the guard(Element object)
        {
            set_pos((Coords){this->get_pos().x + _guard->get_pos().x, this->get_pos().y + _guard->get_pos().y});
        }
    };

    class InteractiveUIElement : public UIElement // dont worry its the last child element class (i hope)
    {
    protected:
        KeyboardInput *_keyboard;

    public:
        InteractiveUIElement(Coords pos, Buffer *parent, Element *guard, KeyboardInput *keyboard) : UIElement(pos, parent, guard), _keyboard(keyboard) {}
    };

    // -------------------- BUFFER --------------------

    class Buffer
    {
        Size2D _size;

        std::vector<Cell> cells;
        std::vector<Cell> previous;

    public:
        Buffer(Size2D size)
            : _size(size),
              cells(_size.w * _size.h),
              previous(_size.w * _size.h)
        {
            std::cout << "\x1b[2J\x1b[H";
            std::cout << "w=" << _size.w
                      << " h=" << _size.h << '\n';
        }

        ~Buffer()
        {
            std::cout << "\033[?25h";
            std::cout << "\x1b[" << _size.h + 2 << ";1H" << std::flush;
        }

        void clear(char fill = ' ') // clears the whole buffer with a character (default is space)
        {
            for (auto &c : cells)
                c.ch = fill;
        }

        void set(Coords pos, char c, Style s = {}) // sets a single character not recommended to use
        {
            if ((unsigned)pos.x < (unsigned)_size.w &&
                (unsigned)pos.y < (unsigned)_size.h)
            {
                auto &cell = cells[pos.y * _size.w + pos.x];
                cell.ch = c;
                cell.style = s;
            }
        }

        void write(Coords pos, const std::string &text, Style s = {}) // print to the buffer not recommended to use
        {
            for (size_t i = 0; i < text.size(); i++)
            {
                int px = pos.x + (int)i;

                if ((unsigned)px >= (unsigned)_size.w ||
                    (unsigned)pos.y >= (unsigned)_size.h)
                    continue;

                auto &cell = cells[pos.y * _size.w + px];
                cell.ch = text[i];
                cell.style = s;
            }
        }

        void flush() // prints the buffer
        {
            for (int y = 0; y < _size.h; y++)
            {
                for (int x = 0; x < _size.w; x++)
                {

                    int i = y * _size.w + x;

                    Cell &cur = cells[i];
                    Cell &prev = previous[i];

                    if (cur.ch != prev.ch ||
                        cur.style.fg != prev.style.fg ||
                        cur.style.bg != prev.style.bg ||
                        cur.style.bold != prev.style.bold ||
                        cur.style.underline != prev.style.underline ||
                        cur.style.dim != prev.style.dim)
                    {
                        std::cout << "\x1b[" << (y + 1)
                                  << ";" << (x + 1) << "H";

                        std::cout << sgr(cur.style) << cur.ch;

                        prev = cur;
                    }
                }
            }

            std::cout << reset();
            std::cout.flush();
        }

        void toggle_cursor(bool onoff) // toggles the terminal cursor on/off
        {
            if (onoff)
            {
                std::cout << "\033[?25h";
            }
            else
            {
                std::cout << "\033[?25l";
            }
        }

        void force_redraw() // idk why is it even here tbh but ill keep it because it might break something
        {
            for (auto &c : previous)
                c.ch = '\0';
        }

        int get_width() // returns width
        {
            return _size.w;
        }
        int get_height() // returns height
        {
            return _size.h;
        }
        Size2D get_size() // returns buffer size in Size2D
        {
            return _size;
        }
    };

    // -------------------- BUTTON --------------------

    class Button : public InteractiveUIElement
    {
        bool selected = false;
        std::string _text;
        ButtonSelector *selector = nullptr;

    public:
        Button(std::string text, Coords pos, Buffer *parent, Element *guard, KeyboardInput *keyboard)
            : _text(std::move(text)), InteractiveUIElement(pos, parent, guard, keyboard) {}

        void draw() // draws the button to the buffer
        {
            std::string s;

            if (selected)
                s = ">" + _text + "<";
            else
                s = "[" + _text + "]";

            _parent->write((Coords){this->get_pos().x + 1, this->get_pos().y}, s);
        }

        int is_pressed(char key, std::function<void()> fn) // checks if its been pressed with a certain key
        {
            if (_keyboard->consume_events().back().key == key && selected)
            {
                fn();
                return 0;
            }
            return 1;
        }

        void set_text(std::string s) // sets the text on the Button
        {
            _text = s;
        }

        std::string get_text() // returns the text on the Button
        {
            return _text;
        }

        void toggle_selected(bool onoff) // toggles if its selected or not
        {
            selected = onoff;
        }

        ButtonSelector *get_selector() // returns the selector
        {
            return selector;
        }
        void set_selector(ButtonSelector *s) // sets the selector -- try not to use it as the better way is to do ButtonSelector.add_button(buttonName);
        {
            selector = s;
        }
        void remove_selector() // removes the selector / sets the selector attribute to nullptr
        {
            selector = nullptr;
        }
    };

    // -------------------- BUTTON SELECTOR --------------------

    class ButtonSelector : public InteractiveUIElement
    {
        std::vector<Button *> buttons;
        size_t index = 0;

    public:
        ButtonSelector(Buffer *parent, Element *guard, KeyboardInput *keyboard) : InteractiveUIElement((Coords){0, 0}, parent, guard, keyboard) {}

        void add_button(Button *b) // connects the button with the selector
        {
            buttons.push_back(b);
            b->set_selector(this);
        }

        void switch_by_press(char next, char prev) // switches by pressing a certain key -- self explanatory
        {
            if (buttons.empty())
                return;
            if (buttons[index]->get_selector() == this)
            {
                if (_keyboard->consume_events().back().key == next)
                {
                    buttons[index]->toggle_selected(false);
                    index = (index + 1) % buttons.size();
                    buttons[index]->toggle_selected(true);
                }

                if (_keyboard->consume_events().back().key == prev)
                {
                    buttons[index]->toggle_selected(false);
                    index = (index == 0) ? buttons.size() - 1 : index - 1;
                    buttons[index]->toggle_selected(true);
                }
            }
        }
    };

    class Label : public UIElement
    {
        std::string _text;

    public:
        Label(std::string text, Coords pos, Buffer *parent, Element *guard) : UIElement(pos, parent, guard), _text(text) {}

        void draw() // draws the label to the buffer
        {
            _parent->write((Coords){this->get_pos().x, this->get_pos().y}, _text);
        }

        void set_text(std::string s) // sets text
        {
            _text = s;
        }
        std::string get_text() // returns the text
        {
            return _text;
        }
    };
    // need one more line to hit 500 perfectly
    // here we are ============================================================|
    // ---------------------------------------------------------------------|  |
    // 500 line mark as of Sat 13 Jun 21:35:34 CEST 2026.    <=================|
    // Quote: My biggest project yet by far -- My biggest pain yet by far...|
    // ---------------------------------------------------------------------|
    class Checkbox : public InteractiveUIElement
    {
        bool active = false;
        bool selected = false;
        std::string _text;
        CheckboxSelector *selector;

    public:
        Checkbox(std::string text, Coords pos, Buffer *parent, Element *guard, KeyboardInput *keyboard) : InteractiveUIElement(pos, parent, guard, keyboard), _text(text) {}

        void draw() // draws the checkbox to the buffer
        {
            _parent->write(_pos, _text);
            if (selected)
            {
                _parent->write((Coords){_pos.x + _text.length(), _pos.y}, " <");
                if (active)
                {
                    _parent->write((Coords){_pos.x + _text.length() + 2, _pos.y}, "V");
                }
                else
                {
                    _parent->write((Coords){_pos.x + _text.length() + 2, _pos.y}, "X");
                }
                _parent->write((Coords){_pos.x + _text.length() + 3, _pos.y}, ">");
            }
            else //---------------------------------------------------------------- pls dont touch the numbers they break
            {
                _parent->write((Coords){_pos.x + _text.length(), _pos.y}, " [");
                if (active)
                {
                    _parent->write((Coords){_pos.x + _text.length() + 2, _pos.y}, "V");
                }
                else
                {
                    _parent->write((Coords){_pos.x + _text.length() + 2, _pos.y}, "X");
                }
                _parent->write((Coords){_pos.x + _text.length() + 3, _pos.y}, "]");
            }
        }
        CheckboxSelector *get_selector() // returns a pointer to the selector that owns the object
        {
            return selector;
        }

        void set_selector(CheckboxSelector *s) // sets selector ownership!!!
        {
            selector = s;
        }

        void set_text(std::string s) // sets text
        {
            _text = s;
        }

        std::string get_text() // gives you the text on the checkbox btw
        {
            return _text;
        }

        void toggle_selected(bool onoff) // toggles selected state
        {
            selected = onoff;
        }

        void toggle_state(bool onoff) // toggles active state
        {
            active = onoff;
        }

        int is_pressed(char key, std::function<void()> fn) // this checks if it has been pressed or not
        {
            if (_keyboard->consume_events().back().key == key && selected)
            {
                active = !active;
                if (active)
                {
                    fn();
                }
                return 0;
            }
            return 1;
        }
    };

    class CheckboxSelector : public InteractiveUIElement
    {
    protected:
        std::vector<Checkbox *> checkboxes;
        size_t index = 0;

    public:
        CheckboxSelector(Buffer *parent, Element *guard, KeyboardInput *keyboard) : InteractiveUIElement((Coords){0, 0}, parent, guard, keyboard) {}

        void add_checkbox(Checkbox *c) // connects checkbox with selector
        {
            checkboxes.push_back(c);
            c->set_selector(this);
        }

        void switch_by_press(char next, char prev) // switches by pressing a certain key
        {
            if (checkboxes.empty())
                return;
            if (checkboxes[index]->get_selector() == this)
            {
                if (_keyboard->consume_events().back().key == next)
                {
                    checkboxes[index]->toggle_selected(false);
                    index = (index + 1) % checkboxes.size();
                    checkboxes[index]->toggle_selected(true);
                }
                else if (_keyboard->consume_events().back().key == prev)
                {
                    checkboxes[index]->toggle_selected(false);
                    index = (index == 0) ? checkboxes.size() - 1 : index - 1;
                    checkboxes[index]->toggle_selected(true);
                }
            }
        }
    };

    Size2D get_terminal_size() // returns the terminal size in Size2D
    {
        winsize ws{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
        {
            return {24, 80}; // fallback
        }
        return {
            static_cast<int>(ws.ws_col),
            static_cast<int>(ws.ws_row)};
    }

}