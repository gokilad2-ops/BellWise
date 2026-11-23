package com.gokila.studyschedule

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView

class PairAdapter(
    private val onEdit: (ClassPair) -> Unit,
    private val onDelete: (ClassPair) -> Unit
) : RecyclerView.Adapter<PairAdapter.ViewHolder>() {

    private var items: List<ClassPair> = emptyList()

    fun submit(newItems: List<ClassPair>) {
        items = newItems
        notifyDataSetChanged()
    }

    class ViewHolder(view: View) : RecyclerView.ViewHolder(view) {
        val time: TextView = view.findViewById(R.id.itemTime)
        val name: TextView = view.findViewById(R.id.itemName)
        val meta: TextView = view.findViewById(R.id.itemMeta)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val view = LayoutInflater.from(parent.context).inflate(R.layout.item_pair, parent, false)
        return ViewHolder(view)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val pair = items[position]
        holder.time.text = "${pair.start} – ${pair.end}"

        if (pair.isWindow) {
            holder.name.text = "Window"
            holder.meta.text = "Free time"
        } else {
            holder.name.text = pair.name
            holder.meta.text = "${pair.room} · ${pair.type.name.lowercase().replaceFirstChar(Char::uppercase)}"
        }

        holder.itemView.setOnClickListener { if (!pair.isWindow) onEdit(pair) }
        holder.itemView.setOnLongClickListener { onDelete(pair); true }
    }

    override fun getItemCount(): Int = items.size
}
