#include "schedule_core.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cctype>

namespace schedule {

// ---- helpers -----------------------------------------------------------

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

static bool startsWithCi(const std::string &text, const std::string &prefix) {
    if (prefix.empty()) return true;
    if (prefix.size() > text.size()) return false;
    return toLower(text.substr(0, prefix.size())) == toLower(prefix);
}

std::string pairTypeToString(PairType t) {
    switch (t) {
        case PairType::LECTURE:  return "LECTURE";
        case PairType::PRACTICE: return "PRACTICE";
        case PairType::REMOTE:   return "REMOTE";
        case PairType::WINDOW:   return "WINDOW";
    }
    return "LECTURE";
}

PairType pairTypeFromString(const std::string &s) {
    if (s == "PRACTICE") return PairType::PRACTICE;
    if (s == "REMOTE")   return PairType::REMOTE;
    if (s == "WINDOW")   return PairType::WINDOW;
    return PairType::LECTURE;
}

void Day::sortByTime() {
    std::sort(pairs.begin(), pairs.end(), [](const Pair &a, const Pair &b) {
        return a.startTime < b.startTime;
    });
}

// ---- ScheduleManager -----------------------------------------------------

ScheduleManager::ScheduleManager(std::string storageDir) : storageDir_(std::move(storageDir)) {}

std::string ScheduleManager::dayFilePath(const std::string &date) const {
    return storageDir_ + "/" + date + ".csv";
}

void ScheduleManager::load() {
    // The directory itself is scanned by the JNI layer (Kotlin hands us the
    // list of existing dates via listKnownDateFiles); here we just expose
    // a lazy getOrCreateDay() + explicit per-file loader used below.
}

Day &ScheduleManager::getOrCreateDay(const std::string &date) {
    auto it = days_.find(date);
    if (it != days_.end()) return it->second;

    Day d;
    d.date = date;

    // Try loading an existing CSV for this date, if present.
    std::ifstream in(dayFilePath(date));
    if (in.is_open()) {
        std::string line;
        int maxId = 0;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string field;
            std::vector<std::string> fields;
            while (std::getline(ss, field, '\t')) fields.push_back(field);
            if (fields.size() < 6) continue;

            Pair p;
            p.id = std::stoi(fields[0]);
            p.room = fields[1];
            p.name = fields[2];
            p.type = pairTypeFromString(fields[3]);
            p.startTime = fields[4];
            p.endTime = fields[5];
            d.pairs.push_back(p);
            maxId = std::max(maxId, p.id);
        }
        d.nextId = maxId + 1;
        d.sortByTime();
    }

    auto result = days_.emplace(date, std::move(d));
    return result.first->second;
}

const Day *ScheduleManager::findDay(const std::string &date) const {
    auto it = days_.find(date);
    if (it == days_.end()) return nullptr;
    return &it->second;
}

std::vector<std::string> ScheduleManager::listDates() const {
    std::vector<std::string> dates;
    dates.reserve(days_.size());
    for (auto &kv : days_) dates.push_back(kv.first);
    return dates; // std::map keeps keys sorted ascending
}

int ScheduleManager::addPair(const std::string &date, const Pair &p) {
    Day &d = getOrCreateDay(date);
    Pair toAdd = p;
    toAdd.id = d.nextId++;
    d.pairs.push_back(toAdd);
    d.sortByTime();
    saveDay(date);
    return toAdd.id;
}

int ScheduleManager::addWindow(const std::string &date, const std::string &start, const std::string &end) {
    Pair window;
    window.type = PairType::WINDOW;
    window.startTime = start;
    window.endTime = end;
    window.name = "Window";
    return addPair(date, window);
}

bool ScheduleManager::editPair(const std::string &date, int id, const Pair &p) {
    Day &d = getOrCreateDay(date);
    for (auto &existing : d.pairs) {
        if (existing.id == id) {
            std::string keepRoom = existing.room, keepName = existing.name;
            existing.room = p.room;
            existing.name = p.name;
            existing.type = p.type;
            existing.startTime = p.startTime;
            existing.endTime = p.endTime;
            d.sortByTime();
            saveDay(date);
            return true;
        }
    }
    return false;
}

bool ScheduleManager::deletePair(const std::string &date, int id) {
    Day &d = getOrCreateDay(date);
    auto before = d.pairs.size();
    d.pairs.erase(std::remove_if(d.pairs.begin(), d.pairs.end(),
                                  [id](const Pair &p) { return p.id == id; }),
                  d.pairs.end());
    bool removed = d.pairs.size() != before;
    if (removed) saveDay(date);
    return removed;
}

void ScheduleManager::saveDay(const std::string &date) {
    const Day *d = findDay(date);
    if (!d) return;
    std::ofstream out(dayFilePath(date), std::ios::trunc);
    for (auto &p : d->pairs) {
        out << p.id << '\t' << p.room << '\t' << p.name << '\t'
            << pairTypeToString(p.type) << '\t' << p.startTime << '\t' << p.endTime << '\n';
    }
}

void ScheduleManager::saveAll() {
    for (auto &kv : days_) saveDay(kv.first);
}

void ScheduleManager::purgeOlderThan(const std::string &todayIso, int daysToKeep) {
    // ISO dates ("YYYY-MM-DD") compare lexicographically like real dates,
    // so simple string subtraction logic isn't needed for ordering —
    // Kotlin computes the cutoff date and passes it in here.
    (void) daysToKeep; // cutoff is precomputed by the caller as todayIso
    for (auto it = days_.begin(); it != days_.end();) {
        if (it->first < todayIso) {
            std::remove(dayFilePath(it->first).c_str());
            it = days_.erase(it);
        } else {
            ++it;
        }
    }
}

void ScheduleManager::enforceRollingWindow(int maxDays) {
    while (static_cast<int>(days_.size()) > maxDays) {
        auto oldest = days_.begin();
        std::remove(dayFilePath(oldest->first).c_str());
        days_.erase(oldest);
    }
}

std::vector<std::string> ScheduleManager::suggestNames(const std::string &prefix, int limit) const {
    std::vector<std::string> out;
    for (auto it = days_.rbegin(); it != days_.rend() && static_cast<int>(out.size()) < limit; ++it) {
        for (auto &p : it->second.pairs) {
            if (p.isWindow()) continue;
            if (startsWithCi(p.name, prefix) &&
                std::find(out.begin(), out.end(), p.name) == out.end()) {
                out.push_back(p.name);
                if (static_cast<int>(out.size()) >= limit) break;
            }
        }
    }
    return out;
}

std::vector<std::string> ScheduleManager::suggestRooms(const std::string &prefix, int limit) const {
    std::vector<std::string> out;
    for (auto it = days_.rbegin(); it != days_.rend() && static_cast<int>(out.size()) < limit; ++it) {
        for (auto &p : it->second.pairs) {
            if (p.isWindow() || p.room.empty()) continue;
            if (startsWithCi(p.room, prefix) &&
                std::find(out.begin(), out.end(), p.room) == out.end()) {
                out.push_back(p.room);
                if (static_cast<int>(out.size()) >= limit) break;
            }
        }
    }
    return out;
}

std::string ScheduleManager::serializeDay(const std::string &date) const {
    const Day *d = findDay(date);
    std::ostringstream out;
    if (!d) return out.str();
    for (auto &p : d->pairs) {
        out << p.id << '\t' << p.room << '\t' << p.name << '\t'
            << pairTypeToString(p.type) << '\t' << p.startTime << '\t' << p.endTime << '\n';
    }
    return out.str();
}

} // namespace schedule
