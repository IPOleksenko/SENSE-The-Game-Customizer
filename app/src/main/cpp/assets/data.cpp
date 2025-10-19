#include <assets/data.hpp>

std::vector<std::pair<std::string, std::array<char, 1024>>> LocalizationList = {
    MAKE_ENTRY("LOADING_TEXT", "Loading..."),
    MAKE_ENTRY("ENDLESS_MODE", "ENDLESS MODE"),
    MAKE_ENTRY("IDLE", IDLE_TEXT),
    MAKE_ENTRY("A_START", "You opened your eyes, but did you see anything new?"),
    MAKE_ENTRY("B_START", "Every day is like the one before, yet you search for differences"),
    MAKE_ENTRY("C_START", "People chase dreams, but who said dreams hold value?"),
    MAKE_ENTRY("D_START", "How many questions have you asked, and how many answers have you received?"),
    MAKE_ENTRY("E_START", "The world moves in circles, but where is its beginning and where is its end?"),
    MAKE_ENTRY("F_START", "You strive to find a purpose, but what is it even for?"),
    MAKE_ENTRY("G_START", "Stones lie on the ground for millennia, yet you live for only a moment."),
    MAKE_ENTRY("H_START", "What matters more: your thoughts or the sound of the wind?"),
    MAKE_ENTRY("I_START", "The history of the world is full of heroes, but no one remembers them."),
    MAKE_ENTRY("J_START", "If something disappears tomorrow, what changes today?"),
    MAKE_ENTRY("K_START", "The sun rises every day, but not for you."),
    MAKE_ENTRY("L_START", "Your heart beats, but who cares?"),
    MAKE_ENTRY("M_START", "Everything you build will one day turn to dust."),
    MAKE_ENTRY("N_START", "You search for truth, but in this world, there is no law of truth."),
    MAKE_ENTRY("O_START", "You search for gods, but there are none."),
    MAKE_ENTRY("P_START", "Joy and pain alternate, but both eventually fade."),
    MAKE_ENTRY("Q_START", "You want to be needed, but by whom?"),
    MAKE_ENTRY("R_START", "The stars shine, but not to show you the way."),
    MAKE_ENTRY("S_START", "Eternity is a word that both frightens and frees."),
    MAKE_ENTRY("T_START", "In this chaos, you seek meaning, but chaos demands no explanation."),
    MAKE_ENTRY("FINAL_START", "THE UNIVERSE DOESN'T MAKE SENSE.")
};

std::vector<std::pair<std::string, std::variant<int, std::array<char, 1024>>>> FontList = {
    {"FONT", std::array<char, 1024>{""}},
    {"FONT_SIZE", 24},
    {"OTHER_TEXT_FONT_SIZE", 48}
};

std::vector<std::pair<std::string, bool>> StandartDecorList = {
    {"grass", true},
    {"flower1", true},
    {"flower2", true},
    {"flower3", true},
    {"flower4", true},
    {"smallrock1", true},
    {"smallrock2", true},
    {"smallrock3", true} 
};

std::vector<CustomeDecorationList> CustomDecorList;