/*
 *   This file is part of DroneBridge: https://github.com/DroneBridge/Desktop
 *
 *   Copyright 2018 Wolfgang Christl
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

#include <stdio.h>
#include <gtk/gtk.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../dronebridge_base/common/db_protocol.h"
#include "../dronebridge_base/common/shared_memory.h"


db_gnd_status_t *db_gnd_status;
db_uav_status_t *db_uav_status;
db_rc_values_t *db_rc_values;
db_rc_status_t *db_rc_status;


bool gtk_timer_running;
guint gtk_timeout;
guint32 ui_update_intervall = 100; // [milliseconds]


GtkLabel *l_lostpackets, *l_badblocks, *l_video_bestrssi, *l_uav_cpu_temp, *l_uav_cpu_load, *l_uav_voltage, *l_rc_rssi,
        *l_rc_packets, *l_recvpackets, *l_datarate;
GtkLabel *l_if_adapters[4];
GtkLabel *l_rssi_adapters[4];
GtkLabel *l_rc_ch[NUM_CHANNELS];
GtkProgressBar *pb_rc_ch[NUM_CHANNELS];


/**
 * Iterate over all apaters registered by the video module to get the best RSSI
 * @return
 */
int find_best_video_rssi() {
    int best_dbm = -128;
    for (int cardcounter = 0; cardcounter < db_gnd_status->wifi_adapter_cnt; ++cardcounter) {
        if (best_dbm < db_gnd_status->adapter[cardcounter].current_signal_dbm
            && db_gnd_status->adapter[cardcounter].current_signal_dbm != 0)
            best_dbm = db_gnd_status->adapter[cardcounter].current_signal_dbm;
    }
    return best_dbm;
}

/**
 * Reads the data from shared memory and updates the UI labels
 * @param data unused
 * @return
 */
gint update_ui_callback(gpointer data) {
    gtk_label_set_text(l_datarate, g_strdup_printf("%.02f Mbit/s", db_gnd_status->kbitrate/1000.0f));
    if (db_gnd_status->received_packet_cnt > 0)
        gtk_label_set_text(l_lostpackets, g_strdup_printf("%i (%.2f%%)", db_gnd_status->lost_packet_cnt,
                                                          (double) db_gnd_status->lost_packet_cnt /
                                                          (db_gnd_status->lost_packet_cnt +
                                                           db_gnd_status->received_packet_cnt)));
    gtk_label_set_text(l_recvpackets, g_strdup_printf("%i", db_gnd_status->received_packet_cnt));
    gtk_label_set_text(l_badblocks, g_strdup_printf("%i", db_gnd_status->damaged_block_cnt));
    gtk_label_set_text(l_video_bestrssi, g_strdup_printf("%i dBm", find_best_video_rssi()));
    for (int i = 0; i < db_gnd_status->wifi_adapter_cnt; i++) {
        gtk_label_set_text(l_if_adapters[i], g_strdup_printf("%s:", db_gnd_status->adapter[i].name));
        gtk_label_set_text(l_rssi_adapters[i],
                           g_strdup_printf("%i dBm (%i) @ %iMbps", db_gnd_status->adapter[i].current_signal_dbm,
                                           db_gnd_status->adapter[i].received_packet_cnt,
                                           db_gnd_status->adapter[i].rate * 500 / 1000));
    }
    // TODO add interface name to UI when switched to
    gtk_label_set_text(l_uav_cpu_temp, g_strdup_printf("%i °C", db_uav_status->temp));
    gtk_label_set_text(l_uav_cpu_load, g_strdup_printf("%i %%", db_uav_status->cpuload));
    if (db_uav_status->undervolt == 1)
        gtk_label_set_text(l_uav_voltage, "undervoltage");
    else
        gtk_label_set_text(l_uav_voltage, "good");
    gtk_label_set_text(l_rc_rssi, g_strdup_printf("%i dBm", db_rc_status->adapter[0].current_signal_dbm));
//    gtk_label_set_text(l_rc_packets, g_strdup_printf("%i", db_rc_status->received_packet_cnt));
//    for (int  i = 0;  i < NUM_CHANNELS; ++i) {
//        gtk_label_set_text(l_rc_ch[i], g_strdup_printf("%i", db_rc_values->ch[i]));
//        gtk_progress_bar_set_fraction(pb_rc_ch[i], (gdouble)(db_rc_values->ch[i]-1000)/1000);
//    }
    return 1;
}

/**
 * Starts a timer that will trigger an event with a specified interval.
 * Used to update the UI regularly
 */
void start_timer() {
    if (!gtk_timer_running) {
        gtk_timeout = g_timeout_add(ui_update_intervall, update_ui_callback, NULL);
        gtk_timer_running = true;
    }
}

void stop_timer() {
    if (gtk_timer_running) {
        g_source_remove(gtk_timeout);
        gtk_timer_running = false;
    }
}

void on_main_window_destroy() {
    stop_timer();
    gtk_main_quit();
}


void get_all_ui_elements(GtkBuilder *pBuilder) {
    l_datarate = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_datarate"));
    l_lostpackets = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_lostpackets"));
    l_recvpackets = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_recvpackets"));
    l_badblocks = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_badblocks"));
    l_video_bestrssi = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_video_bestrssi"));

    l_if_adapters[0] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_adapter_1"));
    l_if_adapters[1] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_adapter_2"));
    l_if_adapters[2] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_adapter_3"));
    l_if_adapters[3] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_adapter_4"));
    l_rssi_adapters[0] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rssi_adapter_1"));
    l_rssi_adapters[1] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rssi_adapter_2"));
    l_rssi_adapters[2] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rssi_adapter_3"));
    l_rssi_adapters[3] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rssi_adapter_4"));

    l_uav_cpu_temp = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_uav_cpu_temp"));
    l_uav_cpu_load = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_uav_cpu_load"));
    l_uav_voltage = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_uav_voltage"));

    l_rc_rssi = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_rssi"));
    l_rc_packets = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_recv_packets"));

    l_rc_ch[0] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch1"));
    l_rc_ch[1] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch2"));
    l_rc_ch[2] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch3"));
    l_rc_ch[3] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch4"));
    l_rc_ch[4] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch5"));
    l_rc_ch[5] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch6"));
    l_rc_ch[6] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch7"));
    l_rc_ch[7] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch8"));
    l_rc_ch[8] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch9"));
    l_rc_ch[9] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch10"));
    l_rc_ch[10] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch11"));
    l_rc_ch[11] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch12"));
    l_rc_ch[12] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch13"));
    l_rc_ch[13] = GTK_LABEL(gtk_builder_get_object(pBuilder, "l_rc_ch14"));

    pb_rc_ch[0] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch1"));
    pb_rc_ch[1] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch2"));
    pb_rc_ch[2] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch3"));
    pb_rc_ch[3] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch4"));
    pb_rc_ch[4] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch5"));
    pb_rc_ch[5] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch6"));
    pb_rc_ch[6] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch7"));
    pb_rc_ch[7] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch8"));
    pb_rc_ch[8] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch9"));
    pb_rc_ch[9] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch10"));
    pb_rc_ch[10] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch11"));
    pb_rc_ch[11] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch12"));
    pb_rc_ch[12] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch13"));
    pb_rc_ch[13] = GTK_PROGRESS_BAR(gtk_builder_get_object(pBuilder, "pb_ch14"));
}

/**
 * Read DroneBridge shared memory information and outputs it using a GTK+ based UI
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {
    db_gnd_status = db_gnd_status_memory_open();
    db_uav_status = db_uav_status_memory_open();
    db_rc_status = db_rc_status_memory_open();
    db_rc_values = db_rc_values_memory_open();

    printf("DB_DESKTOP_MONITOR: started!\n");
    GError *err = NULL;
    GtkBuilder *builder;
    GtkWidget *window;

    gtk_init(&argc, &argv);

    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "glade/monitor_ui.glade", &err);
    if (err != NULL) {
        fprintf(stderr, "Unable to read file: %s\n", err->message);
        g_error_free(err);
        return 1;
    }

    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    g_signal_connect(window, "destroy", G_CALLBACK(on_main_window_destroy), NULL);
    get_all_ui_elements(builder);

    g_object_unref(builder);

    start_timer(); // start UI updater
    gtk_widget_show(window);
    gtk_main();

    return 0;
}