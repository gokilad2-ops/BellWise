package com.gokila.studyschedule

import android.app.TimePickerDialog
import android.os.Bundle
import android.widget.ArrayAdapter
import android.widget.AutoCompleteTextView
import android.widget.Spinner
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.button.MaterialButton
import com.google.android.material.switchmaterial.SwitchMaterial
import java.util.*

/**
 * One screen handles add, edit and "quick add window" — the spec only
 * calls for a gap defined by its start/end time, so the window path just
 * hides the room/name/type fields.
 */
class AddEditPairActivity : AppCompatActivity() {

    private var date: String = ""
    private var editingId: Int? = null
    private var startTime = "09:00"
    private var endTime = "10:00"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_add_edit_pair)

        date = intent.getStringExtra(MainActivity.EXTRA_DATE) ?: ""
        val asWindow = intent.getBooleanExtra("as_window", false)
        if (intent.hasExtra(MainActivity.EXTRA_PAIR_ID)) {
            editingId = intent.getIntExtra(MainActivity.EXTRA_PAIR_ID, -1)
        }

        val roomField = findViewById<AutoCompleteTextView>(R.id.fieldRoom)
        val nameField = findViewById<AutoCompleteTextView>(R.id.fieldName)
        val typeSpinner = findViewById<Spinner>(R.id.fieldType)
        val windowSwitch = findViewById<SwitchMaterial>(R.id.switchIsWindow)
        val startBtn = findViewById<MaterialButton>(R.id.btnStartTime)
        val endBtn = findViewById<MaterialButton>(R.id.btnEndTime)
        val saveBtn = findViewById<MaterialButton>(R.id.btnSave)

        typeSpinner.adapter = ArrayAdapter(
            this, android.R.layout.simple_spinner_dropdown_item,
            listOf("LECTURE", "PRACTICE", "REMOTE")
        )

        // Autocomplete: suggestions come straight from the C++ layer,
        // which tracks everything typed before, most-recent first.
        roomField.setAdapter(ArrayAdapter(this, android.R.layout.simple_dropdown_item_1line, emptyList<String>()))
        roomField.addTextChangedListener(simpleWatcher { prefix ->
            val suggestions = ScheduleNative.nativeSuggestRooms(prefix)
                .split('\n').filter { it.isNotBlank() }
            roomField.setAdapter(ArrayAdapter(this, android.R.layout.simple_dropdown_item_1line, suggestions))
        })
        nameField.addTextChangedListener(simpleWatcher { prefix ->
            val suggestions = ScheduleNative.nativeSuggestNames(prefix)
                .split('\n').filter { it.isNotBlank() }
            nameField.setAdapter(ArrayAdapter(this, android.R.layout.simple_dropdown_item_1line, suggestions))
        })

        windowSwitch.isChecked = asWindow
        fun applyWindowVisibility(isWindow: Boolean) {
            roomField.isEnabled = !isWindow
            nameField.isEnabled = !isWindow
            typeSpinner.isEnabled = !isWindow
        }
        applyWindowVisibility(asWindow)
        windowSwitch.setOnCheckedChangeListener { _, checked -> applyWindowVisibility(checked) }

        startBtn.text = startTime
        endBtn.text = endTime
        startBtn.setOnClickListener { pickTime(startBtn) { t -> startTime = t } }
        endBtn.setOnClickListener { pickTime(endBtn) { t -> endTime = t } }

        // Pre-fill when editing an existing pair.
        editingId?.let { id ->
            val existing = parseDay(ScheduleNative.nativeGetDay(date)).find { it.id == id }
            existing?.let {
                roomField.setText(it.room)
                nameField.setText(it.name)
                startTime = it.start; startBtn.text = it.start
                endTime = it.end; endBtn.text = it.end
                val idx = (typeSpinner.adapter as ArrayAdapter<String>).getPosition(it.type.name)
                if (idx >= 0) typeSpinner.setSelection(idx)
                findViewById<TextView>(R.id.editorTitle).text = getString(R.string.edit_pair)
            }
        }

        saveBtn.setOnClickListener {
            if (windowSwitch.isChecked) {
                if (editingId != null) ScheduleNative.nativeDeletePair(date, editingId!!)
                ScheduleNative.nativeAddWindow(date, startTime, endTime)
            } else {
                val room = roomField.text.toString()
                val name = nameField.text.toString()
                val type = typeSpinner.selectedItem?.toString() ?: "LECTURE"
                if (editingId != null) {
                    ScheduleNative.nativeEditPair(date, editingId!!, room, name, type, startTime, endTime)
                } else {
                    ScheduleNative.nativeAddPair(date, room, name, type, startTime, endTime)
                }
            }
            finish()
        }
    }

    private fun pickTime(button: MaterialButton, onPicked: (String) -> Unit) {
        val cal = Calendar.getInstance()
        TimePickerDialog(this, { _, hour, minute ->
            val formatted = String.format(Locale.US, "%02d:%02d", hour, minute)
            onPicked(formatted)
            button.text = formatted
        }, cal.get(Calendar.HOUR_OF_DAY), cal.get(Calendar.MINUTE), true).show()
    }

    private fun simpleWatcher(onChanged: (String) -> Unit) = object : android.text.TextWatcher {
        override fun afterTextChanged(s: android.text.Editable?) { onChanged(s?.toString().orEmpty()) }
        override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
        override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {}
    }
}
