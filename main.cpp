// Study Schedule - a small C++17 timetable app for students.
// Build:  ./build.sh      Run:  ./studyschedule      Widget view:  ./studyschedule --widget
#include <iostream>
#include "ui.hpp"

using namespace ss;

static std::string dataPath() {
    const char* home = std::getenv("HOME");
    return home ? std::string(home) + "/.study_schedule.dat" : "study_schedule.dat";
}

static bool pickIndex(const std::vector<Pair>& v, size_t& idx, std::string& note) {
    if (v.empty()) { note = "Nothing on this day yet."; return false; }
    std::string s;
    ui::readLine("Pair number: ", s);
    int n = std::atoi(s.c_str());
    if (n < 1 || (size_t)n > v.size()) { note = "No such number."; return false; }
    idx = (size_t)n - 1;
    return true;
}

// Fill in a pair. Empty name => window (only the time is needed).
static void fillPair(const Schedule& s, Pair& p) {
    p.name = ui::askName("Name ('-' = window)", p.name, s.names());

    auto pool = s.timePool();
    int ds = p.end > 0 ? p.start : -1, de = p.end > 0 ? p.end : -1;
    for (;;) {
        p.start = ui::askTime("Start", pool, ds);
        p.end = ui::askTime("End", pool, de);
        if (p.end > p.start) break;
        std::cout << "  End must be after start.\n";
        ds = p.start;
    }

    if (p.isWindow()) {
        p.room.clear();
        p.type = Type::Lecture;
    } else {
        p.room = ui::ask("Room", p.room);
        p.type = ui::askType(p.type);
    }
}

static bool confirmOverlap(const Schedule& s, const Date& d, const Pair& p, int skip) {
    if (!s.overlaps(d, p, skip)) return true;
    std::string a;
    ui::readLine("  Overlaps another class. Save anyway? [y/N]: ", a);
    return !a.empty() && (a[0] == 'y' || a[0] == 'Y');
}

int main(int argc, char** argv) {
    Schedule s(dataPath());
    s.load();
    s.purgeOld();  // days older than a week disappear

    if (argc > 1 && std::string(argv[1]) == "--widget") {
        ui::printWidget(s);
        return 0;
    }

    Date cur = today();
    std::string note;

    try {
        for (;;) {
            ui::clearScreen();
            ui::printDay(s, cur);
            ui::printMenu();
            if (!note.empty()) std::cout << "\n" << note << "\n";
            note.clear();

            std::string cmd;
            ui::readLine("\n> ", cmd);
            char c = cmd.empty() ? '\0' : (char)std::tolower((unsigned char)cmd[0]);

            switch (c) {
                case 'n':
                    if (addDays(cur, 1) <= s.maxDate()) cur = addDays(cur, 1);
                    else note = "That's the last available day.";
                    break;
                case 'p':
                    if (addDays(cur, -1) >= s.minDate()) cur = addDays(cur, -1);
                    else note = "Older days are deleted.";
                    break;
                case 't':
                    cur = today();
                    break;
                case 'g': {
                    std::string in;
                    ui::readLine("Go to (0-29 days from today, or YYYY-MM-DD): ", in);
                    Date target = validDate(in) ? in : addDays(today(), std::atoi(in.c_str()));
                    if (s.inRange(target)) cur = target;
                    else note = "Out of range (" + s.minDate() + " .. " + s.maxDate() + ").";
                    break;
                }
                case 'a': {
                    Pair p;
                    fillPair(s, p);
                    if (confirmOverlap(s, cur, p, -1)) { s.add(cur, p); s.save(); note = "Added."; }
                    break;
                }
                case 'e': {
                    size_t i;
                    if (!pickIndex(s.day(cur), i, note)) break;
                    Pair p = s.day(cur)[i];
                    fillPair(s, p);
                    if (confirmOverlap(s, cur, p, (int)i)) { s.update(cur, i, p); s.save(); note = "Updated."; }
                    break;
                }
                case 'd': {
                    size_t i;
                    if (!pickIndex(s.day(cur), i, note)) break;
                    std::string a;
                    ui::readLine("Delete pair " + std::to_string(i + 1) + "? [y/N]: ", a);
                    if (!a.empty() && (a[0] == 'y' || a[0] == 'Y')) { s.remove(cur, i); s.save(); note = "Deleted."; }
                    break;
                }
                case 'c': {
                    for (size_t i = 0; i < ui::kThemeCount; ++i)
                        std::cout << "  " << i + 1 << ") " << ui::kThemes[i].head << ui::kThemes[i].name
                                  << ui::RESET << (i == (size_t)s.theme ? "  (current)" : "") << "\n";
                    std::string in;
                    ui::readLine("Palette number: ", in);
                    int k = std::atoi(in.c_str());
                    if (k >= 1 && (size_t)k <= ui::kThemeCount) { s.theme = k - 1; s.save(); }
                    break;
                }
                case 'w': {
                    ui::clearScreen();
                    ui::printWidget(s);
                    std::string in;
                    ui::readLine("\nEnter to go back ", in);
                    break;
                }
                case 'q':
                    s.save();
                    return 0;
                default:
                    break;
            }
        }
    } catch (const ui::Quit&) {
        s.save();
    }
    return 0;
}
