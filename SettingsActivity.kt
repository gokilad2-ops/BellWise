package com.gokila.studyschedule

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.RecyclerView

/** Lets the student pick one of a fixed set of color palettes for the app theme. */
class SettingsActivity : AppCompatActivity() {

    data class Palette(val label: String, val themeRes: Int, val swatchColor: Int)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val prefs = getSharedPreferences("settings", MODE_PRIVATE)
        setTheme(prefs.getInt("theme_res", R.style.Theme_StudySchedule_Blue))
        setContentView(R.layout.activity_settings)

        val palettes = listOf(
            Palette("Blue", R.style.Theme_StudySchedule_Blue, R.color.palette_blue),
            Palette("Green", R.style.Theme_StudySchedule_Green, R.color.palette_green),
            Palette("Purple", R.style.Theme_StudySchedule_Purple, R.color.palette_purple),
            Palette("Orange", R.style.Theme_StudySchedule_Orange, R.color.palette_orange)
        )

        val recycler = findViewById<RecyclerView>(R.id.paletteRecycler)
        recycler.layoutManager = GridLayoutManager(this, 2)
        recycler.adapter = object : RecyclerView.Adapter<RecyclerView.ViewHolder>() {
            override fun onCreateViewHolder(parent: android.view.ViewGroup, viewType: Int): RecyclerView.ViewHolder {
                val view = layoutInflater.inflate(R.layout.item_palette, parent, false)
                return object : RecyclerView.ViewHolder(view) {}
            }
            override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {
                val p = palettes[position]
                val swatch = holder.itemView.findViewById<android.view.View>(R.id.swatch)
                val label = holder.itemView.findViewById<android.widget.TextView>(R.id.swatchLabel)
                swatch.setBackgroundColor(getColor(p.swatchColor))
                label.text = p.label
                holder.itemView.setOnClickListener {
                    prefs.edit().putInt("theme_res", p.themeRes).apply()
                    recreate()
                }
            }
            override fun getItemCount() = palettes.size
        }
    }
}
