package com.gokila.studyschedule

import android.content.Intent
import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.floatingactionbutton.FloatingActionButton
import java.text.SimpleDateFormat
import java.util.*

class MainActivity : AppCompatActivity() {

    private lateinit var adapter: PairAdapter
    private lateinit var dateLabel: TextView
    private val isoFormat = SimpleDateFormat("yyyy-MM-dd", Locale.US)
    private val displayFormat = SimpleDateFormat("EEE, d MMM", Locale.US)
    private var currentCalendar: Calendar = Calendar.getInstance()

    companion object {
        const val EXTRA_DATE = "extra_date"
        const val EXTRA_PAIR_ID = "extra_pair_id"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        applyStoredTheme()

        // Native storage lives in the app's private files dir — Kotlin
        // decides *where*, C++ decides everything about *what's in it*.
        ScheduleNative.nativeInit(filesDir.absolutePath)

        dateLabel = findViewById(R.id.dateLabel)
        val recycler = findViewById<RecyclerView>(R.id.pairsRecycler)
        recycler.layoutManager = LinearLayoutManager(this)
        adapter = PairAdapter(
            onEdit = { pair -> openEditor(pair.id) },
            onDelete = { pair ->
                ScheduleNative.nativeDeletePair(currentDateIso(), pair.id)
                refreshDay()
            }
        )
        recycler.adapter = adapter

        findViewById<FloatingActionButton>(R.id.fabAddPair).setOnClickListener { openEditor(null) }
        findViewById<FloatingActionButton>(R.id.fabAddWindow).setOnClickListener { promptAddWindow() }
        findViewById<android.widget.ImageButton>(R.id.btnPrevDay).setOnClickListener { shiftDay(-1) }
        findViewById<android.widget.ImageButton>(R.id.btnNextDay).setOnClickListener { shiftDay(1) }
        findViewById<android.widget.ImageButton>(R.id.btnSettings).setOnClickListener {
            startActivity(Intent(this, SettingsActivity::class.java))
        }

        // House-keeping required by the spec: drop anything older than a
        // week, and never keep more than a 30-day rolling window.
        val cutoff = Calendar.getInstance().apply { add(Calendar.DAY_OF_YEAR, -7) }
        ScheduleNative.nativePurgeOlderThan(isoFormat.format(cutoff.time))

        refreshDay()
    }

    override fun onResume() {
        super.onResume()
        applyStoredTheme()
        refreshDay()
    }

    private fun currentDateIso(): String = isoFormat.format(currentCalendar.time)

    private fun shiftDay(delta: Int) {
        currentCalendar.add(Calendar.DAY_OF_YEAR, delta)
        // Keep navigation inside the supported 30-day window (15 back / 15 forward from today).
        val today = Calendar.getInstance()
        val diffDays = ((currentCalendar.timeInMillis - today.timeInMillis) / 86400000L).toInt()
        if (diffDays < -7 || diffDays > 30) {
            currentCalendar.add(Calendar.DAY_OF_YEAR, -delta) // revert
            return
        }
        refreshDay()
    }

    private fun refreshDay() {
        dateLabel.text = displayFormat.format(currentCalendar.time)
        val raw = ScheduleNative.nativeGetDay(currentDateIso())
        adapter.submit(parseDay(raw))
    }

    private fun openEditor(pairId: Int?) {
        val intent = Intent(this, AddEditPairActivity::class.java)
        intent.putExtra(EXTRA_DATE, currentDateIso())
        pairId?.let { intent.putExtra(EXTRA_PAIR_ID, it) }
        startActivity(intent)
    }

    private fun promptAddWindow() {
        // Simple quick-add: reuse the editor screen pre-set to "window" mode.
        val intent = Intent(this, AddEditPairActivity::class.java)
        intent.putExtra(EXTRA_DATE, currentDateIso())
        intent.putExtra("as_window", true)
        startActivity(intent)
    }

    private fun applyStoredTheme() {
        val prefs = getSharedPreferences("settings", MODE_PRIVATE)
        val themeRes = prefs.getInt("theme_res", R.style.Theme_StudySchedule_Blue)
        setTheme(themeRes)
    }
}
