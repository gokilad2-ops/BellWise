package com.gokila.studyschedule.widget

import android.appwidget.AppWidgetManager
import android.appwidget.AppWidgetProvider
import android.content.Context
import android.widget.RemoteViews
import com.gokila.studyschedule.R
import com.gokila.studyschedule.ScheduleNative
import com.gokila.studyschedule.parseDay
import java.text.SimpleDateFormat
import java.util.*

/**
 * Reads today's pairs straight from the native layer and renders them as
 * a simple stacked list inside the widget. Kept deliberately simple (no
 * RemoteViewsService/ListView) so it updates instantly on every refresh.
 */
class ScheduleWidgetProvider : AppWidgetProvider() {

    override fun onUpdate(context: Context, appWidgetManager: AppWidgetManager, appWidgetIds: IntArray) {
        ScheduleNative.nativeInit(context.filesDir.absolutePath)
        val today = SimpleDateFormat("yyyy-MM-dd", Locale.US).format(Date())
        val pairs = parseDay(ScheduleNative.nativeGetDay(today))

        for (widgetId in appWidgetIds) {
            val views = RemoteViews(context.packageName, R.layout.widget_schedule)
            views.setTextViewText(R.id.widgetDate, SimpleDateFormat("EEE, d MMM", Locale.US).format(Date()))

            val summary = if (pairs.isEmpty()) {
                "No classes scheduled"
            } else {
                pairs.joinToString("\n") { p ->
                    if (p.isWindow) "${p.start}-${p.end}  Window"
                    else "${p.start}-${p.end}  ${p.name} (${p.room})"
                }
            }
            views.setTextViewText(R.id.widgetBody, summary)
            appWidgetManager.updateAppWidget(widgetId, views)
        }
    }
}
