#ifndef STUDYSCHEDULE_SCHEDULE_CORE_H
#define STUDYSCHEDULE_SCHEDULE_CORE_H

#include <string>
#include <vector>
#include <map>

namespace schedule {

// ---- Types -----------------------------------------------------------

enum class PairType {
    LECTURE = 0,
    PRACTICE = 1,
    REMOTE = 2,
    WINDOW = 3   // a gap between real pairs — only start/end matter
};

std::string pairTypeToString(PairType t);
PairType pairTypeFromString(const std::string &s);

// One class ("pair") on a given day.
struct Pair {
    int id = 0;                 // unique id within the day
    std::string room;
    std::string name;
    PairType type = PairType::LECTURE;
    std::string startTime;      // "HH:MM" 24h
    std::string endTime;        // "HH:MM" 24h

    bool isWindow() const { return type == PairType::WINDOW; }
};

// All pairs for one calendar date.
struct Day {
    std::string date;           // "YYYY-MM-DD"
    std::vector<Pair> pairs;
    int nextId = 1;

    void sortByTime();
};

// ---- Manager -----------------------------------------------------------
// Owns every day currently kept (up to a 30-day rolling window) and the
// autocomplete history. This is the single source of truth; the JNI layer
// is a thin wrapper around it, and the Kotlin UI only ever renders what
// this class hands back.
class ScheduleManager {
public:
    explicit ScheduleManager(std::string storageDir);

    // Loads every day file found in storageDir, then purges days older
    // than 7 days (per the "automatic deletion" requirement) and drops
    // anything beyond the 30-day rolling window.
    void load();
    void saveDay(const std::string &date);
    void saveAll();

    // CRUD -----------------------------------------------------------
    int addPair(const std::string &date, const Pair &p);     // returns new id, 0 on failure
    bool editPair(const std::string &date, int id, const Pair &p);
    bool deletePair(const std::string &date, int id);
    int addWindow(const std::string &date, const std::string &start, const std::string &end);

    // Query ------------------------------------------------------------
    Day &getOrCreateDay(const std::string &date);
    const Day *findDay(const std::string &date) const;
    std::vector<std::string> listDates() const;               // sorted, oldest first

    // House-keeping ------------------------------------------------------
    void purgeOlderThan(const std::string &todayIso, int daysToKeep = 7);
    void enforceRollingWindow(int maxDays = 30);

    // Autocomplete --------------------------------------------------------
    // Returns distinct previously-used names/rooms whose text starts with
    // `prefix` (case-insensitive), most-recently-used first, capped at `limit`.
    std::vector<std::string> suggestNames(const std::string &prefix, int limit = 8) const;
    std::vector<std::string> suggestRooms(const std::string &prefix, int limit = 8) const;

    // Serialization helpers used by the JNI layer to hand data back as
    // plain strings (keeps the native side free of any JSON dependency).
    // Format per pair, one per line:
    //   id\troom\tname\ttype\tstart\tend
    std::string serializeDay(const std::string &date) const;

private:
    std::string storageDir_;
    std::map<std::string, Day> days_; // keyed by ISO date, sorted automatically

    std::string dayFilePath(const std::string &date) const;
};

} // namespace schedule

#endif // STUDYSCHEDULE_SCHEDULE_CORE_H
