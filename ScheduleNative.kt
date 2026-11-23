package com.gokila.studyschedule

/**
 * Thin Kotlin facade over the C++ core (schedule_core.cpp / jni_bridge.cpp).
 * All real scheduling logic — sorting, gaps, 30-day window, 7-day purge,
 * autocomplete — lives in native code. This object only marshals calls.
 */
object ScheduleNative {

    init {
        System.loadLibrary("schedule_core")
    }

    external fun nativeInit(storageDir: String)
    external fun nativeGetDay(date: String): String
    external fun nativeAddPair(
        date: String, room: String, name: String, type: String, start: String, end: String
    ): Int
    external fun nativeAddWindow(date: String, start: String, end: String): Int
    external fun nativeEditPair(
        date: String, id: Int, room: String, name: String, type: String, start: String, end: String
    ): Boolean
    external fun nativeDeletePair(date: String, id: Int): Boolean
    external fun nativePurgeOlderThan(cutoffIsoDate: String)
    external fun nativeSuggestNames(prefix: String): String
    external fun nativeSuggestRooms(prefix: String): String
}

enum class PairType { LECTURE, PRACTICE, REMOTE, WINDOW }

data class ClassPair(
    val id: Int,
    val room: String,
    val name: String,
    val type: PairType,
    val start: String,
    val end: String
) {
    val isWindow get() = type == PairType.WINDOW
}

/** Parses the tab/newline text nativeGetDay() returns into ClassPair objects. */
fun parseDay(raw: String): List<ClassPair> {
    if (raw.isBlank()) return emptyList()
    return raw.trim('\n').split('\n').mapNotNull { line ->
        val f = line.split('\t')
        if (f.size < 6) return@mapNotNull null
        ClassPair(
            id = f[0].toIntOrNull() ?: return@mapNotNull null,
            room = f[1],
            name = f[2],
            type = runCatching { PairType.valueOf(f[3]) }.getOrDefault(PairType.LECTURE),
            start = f[4],
            end = f[5]
        )
    }.sortedBy { it.start }
}
