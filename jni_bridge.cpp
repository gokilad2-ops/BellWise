#include <jni.h>
#include <memory>
#include <string>
#include "schedule_core.h"

using schedule::ScheduleManager;
using schedule::Pair;
using schedule::pairTypeFromString;

namespace {
// One manager instance per app process, created on first use and reused
// for the life of the process (the Kotlin side owns when to init/save).
std::unique_ptr<ScheduleManager> g_manager;

std::string jstr(JNIEnv *env, jstring s) {
    if (!s) return "";
    const char *chars = env->GetStringUTFChars(s, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(s, chars);
    return result;
}

jstring toJstr(JNIEnv *env, const std::string &s) {
    return env->NewStringUTF(s.c_str());
}
} // namespace

extern "C" {

JNIEXPORT void JNICALL
Java_com_gokila_studyschedule_ScheduleNative_nativeInit(JNIEnv *env, jobject, jstring storageDir) {
    g_manager = std::make_unique<ScheduleManager>(jstr(env, storageDir));
}

// Returns the day's pairs as tab/newline separated text:
// id \t room \t name \t type \t start \t end  (one line per pair)
JNIEXPORT jstring JNICALL
Java_com_gokila_studyschedule_ScheduleNative_nativeGetDay(JNIEnv *env, jobject, jstring date) {
    if (!g_manager) return toJstr(env, "");
    std::string d = jstr(env, date);
    g_manager->getOrCreateDay(d); // ensures file is loaded if it exists
    return toJstr(env, g_manager->serializeDay(d));
}

JNIEXPORT jint JNICALL
Java_com_gokila_studyschedule_ScheduleNative_nativeAddPair(
        JNIEnv *env, jobject, jstring date, jstring room, jstring name,
        jstring type, jstring start, jstring end) {
    if (!g_manager) return 0;
    Pair p;
    p.room = jstr(env, room);
    p.name = jstr(env, name);
    p.type = pairTypeFromString(jstr(env, type));
    p.startTime = jstr(env, start);
    p.endTime = jstr(env, end);
    return g_manager->addPair(jstr(env, date), p);
}

JNIEXPORT jint JNICALL
Java_com_gokila_studyschedule_ScheduleNative_nativeAddWindow(
        JNIEnv *env, jobject, jstring date, jstring start, jstring end) {
    if (!g_manager) return 0;
    return g_manager->addWindow(jstr(env, date), jstr(env, start), jstr(env, end));
}

JNIEXPORT jboolean JNICALL
Java_com_gokila_studyschedule_ScheduleNative_nativeEditPair(
        JNIEnv *env, jobject, jstring date, jint id, jstring room, jstring name,
        jstring type, jstring start, jstring end) {
    if (!g_manager) return JNI_FALSE;
    Pair p;
    p.room = jstr(env, room);
    p.name = jstr(env, name);
    p.type = pairTypeFromString(jstr(env, type));
    p.startTime = jstr(env, start);
    p.endTime = jstr(env, end);
    return g_manager->editPair(jstr(env, date), id, p) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokila_studyschedule_ScheduleNative_nativeDeletePair(JNIEnv *env, jobject, jstring date, jint id) {
    if (!g_manager) return JNI_FALSE;
    return g_manager->deletePair(jstr(env, date), id) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_gokila_studyschedule_ScheduleNative_nativePurgeOlderThan(JNIEnv *env, jobject, jstring cutoffIsoDate) {
    if (!g_manager) return;
    g_manager->purgeOlderThan(jstr(env, cutoffIsoDate));
    g_manager->enforceRollingWindow(30);
}

// Returns suggestions newline-separated.
JNIEXPORT jstring JNICALL
Java_com_gokila_studyschedule_ScheduleNative_nativeSuggestNames(JNIEnv *env, jobject, jstring prefix) {
    if (!g_manager) return toJstr(env, "");
    auto v = g_manager->suggestNames(jstr(env, prefix));
    std::string joined;
    for (auto &s : v) { joined += s; joined += '\n'; }
    return toJstr(env, joined);
}

JNIEXPORT jstring JNICALL
Java_com_gokila_studyschedule_ScheduleNative_nativeSuggestRooms(JNIEnv *env, jobject, jstring prefix) {
    if (!g_manager) return toJstr(env, "");
    auto v = g_manager->suggestRooms(jstr(env, prefix));
    std::string joined;
    for (auto &s : v) { joined += s; joined += '\n'; }
    return toJstr(env, joined);
}

} // extern "C"
