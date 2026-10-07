#include <gtk/gtk.h>
#include <time.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Bezpečná štruktúra z tvojho kódu na správu labelov
typedef struct {
    GtkWidget *clock;
    GtkWidget *battery;
    GtkWidget *volume;
} PanelLabels;

// Callback funkcia pre otvorenie Firefoxu po stlačení Enter
static void on_entry_activate(GtkEntry *entry, gpointer user_data) {
    const gchar *text = gtk_entry_get_text(entry);

    if (text == NULL || strlen(text) == 0) {
        // gtk_main_quit();
        return;
    }

    gchar *escaped_text = g_uri_escape_string(text, NULL, TRUE);
    gchar *url = g_strdup_printf("https://google.com?q=%s", escaped_text);
    gchar *command = g_strdup_printf("firefox %s &", url);

    int result = system(command);
    (void)result;

    g_free(escaped_text);
    g_free(url);
    g_free(command);

    // gtk_main_quit();
	// 2. ZMENA: Vyčistíme textové pole, aby bolo pripravené na ďalšie hľadanie
    gtk_entry_set_text(entry, "");
}

// Zavretie panela pomocou klávesu Escape
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    if (event->keyval == GDK_KEY_Escape) {
        gtk_main_quit();
        return TRUE;
    }
    return FALSE;
}

// Tvoja funkcia na zistenie stavu batérie
static void get_battery_status(char *buffer, size_t max_len) {
    FILE *f_cap = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    FILE *f_stat = fopen("/sys/class/power_supply/BAT0/status", "r");
    
    if (!f_cap) {
        f_cap = fopen("/sys/class/power_supply/BAT1/capacity", "r");
        f_stat = fopen("/sys/class/power_supply/BAT1/status", "r");
    }

    if (f_cap && f_stat) {
        int capacity = 0;
        char status[32] = {0};
        
        if (fscanf(f_cap, "%d", &capacity) == 1 && fscanf(f_stat, "%31s", status) == 1) {
            const char *icon = "\xf0\x9f\x94\xb4"; // 🔋
            if (g_str_has_prefix(status, "Charg")) {
                icon = "\xe2\x9a\xa1"; // ⚡
            }
            snprintf(buffer, max_len, "%s %d%%", icon, capacity);
        } else {
            snprintf(buffer, max_len, "BAT N/A");
        }
        fclose(f_cap);
        fclose(f_stat);
    } else {
        if (f_cap) fclose(f_cap);
        if (f_stat) fclose(f_stat);
        snprintf(buffer, max_len, "");
    }
}

// Tvoja funkcia na zistenie stavu hlasitosti
static void get_volume_status(char *buffer, size_t max_len) {
    FILE *f = popen("amixer get Master | grep -o -E '[0-9]+%' | head -n 1", "r");
    FILE *f_mute = popen("amixer get Master | grep -o -E '\\[on\\]|\\[off\\]' | head -n 1", "r");
    
    char volume[32] = {0};
    char mute_status[32] = {0};

    if (f && fgets(volume, sizeof(volume), f)) {
        volume[strcspn(volume, "\n")] = 0;
        
        gboolean is_muted = FALSE;
        if (f_mute && fgets(mute_status, sizeof(mute_status), f_mute)) {
            if (strstr(mute_status, "off")) {
                is_muted = TRUE;
            }
        }

        if (is_muted) {
            snprintf(buffer, max_len, "\xf0\x9f\x94\xa0 Mute"); // 🔇
        } else {
            snprintf(buffer, max_len, "\xf0\x9f\x94\xa1 %s", volume); // 🔊
        }
    } else {
        snprintf(buffer, max_len, "🔊 N/A");
    }

    if (f) pclose(f);
    if (f_mute) pclose(f_mute);
}

// Tvoj periodický update labelov obohatený o hodiny
static gboolean update_widgets(gpointer user_data) {
    PanelLabels *labels = (PanelLabels *)user_data;

    time_t rawtime;
    struct tm *timeinfo;
    char time_buffer[40]; 

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_buffer, sizeof(time_buffer), "%H:%M", timeinfo); // Zjednodušený formát pre malý panel
    gtk_label_set_text(GTK_LABEL(labels->clock), time_buffer);

    char bat_buffer[32];
    get_battery_status(bat_buffer, sizeof(bat_buffer));
    gtk_label_set_text(GTK_LABEL(labels->battery), bat_buffer);

    char vol_buffer[32];
    get_volume_status(vol_buffer, sizeof(vol_buffer));
    gtk_label_set_text(GTK_LABEL(labels->volume), vol_buffer);

    return TRUE; 
}

// Úprava CSS štýlov pre jednotný tmavý vzhľad celého panelu
static void apply_css(GtkWidget *window, GtkWidget *entry, GtkWidget *box) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const gchar *css = 
        "window {\n"
        "  background-color: transparent;\n"
        "  box-shadow: none;\n"
        "  border: none;\n"
        "}\n"
        "#main-layout-box {\n"
        "  background-color: #1a1a1a;\n"
        "  border: 1px solid #333333;\n"
        "  border-radius: 8px;\n"
        "  padding: 2px 8px;\n"
        "}\n"
        "entry {\n"
        "  background-color: transparent;\n"
        "  color: #ffffff;\n"
        "  border: none;\n"
        "  box-shadow: none;\n"
        "  font-size: 13px;\n"
        "  min-height: 28px;\n"
        "  caret-color: #ffffff;\n"
        "}\n"
        "label {\n"
        "  color: #aaaaaa;\n"
        "  font-size: 12px;\n"
        "  font-weight: bold;\n"
        "  margin-left: 6px;\n"
        "}\n";

    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    
    gtk_style_context_add_provider(gtk_widget_get_style_context(window), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_style_context_add_provider(gtk_widget_get_style_context(entry), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_style_context_add_provider(gtk_widget_get_style_context(box), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Hlavné bezokrajové okno
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    // ZMENA 1: Nastavíme oknu príznak, že ide o systémový DOCK (panel) alebo DESKTOP (pozadie)
    // To zabezpečí, že okná iných aplikácií sa budú otvárať nad ním a nebude zavadzať v Alt+Tab
//    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DOCK);
    
    // Podpora pre priehľadnosť (ak kompozitor beží)
    GdkScreen *gdk_screen = gtk_widget_get_screen(window);
    GdkVisual *visual = gdk_screen_get_rgba_visual(gdk_screen);
    if (visual != NULL && gdk_screen_is_composited(gdk_screen)) {
        gtk_widget_set_visual(window, visual);
    }

    // Hlavný horizontálny kontajner (Box)
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_name(main_box, "main-layout-box");
    gtk_container_add(GTK_CONTAINER(window), main_box);

    // Vstupné textové pole
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Search, calculate or run...");
    gtk_box_pack_start(GTK_BOX(main_box), entry, TRUE, TRUE, 0);

    // Vytvorenie tvojich stavových labelov
    GtkWidget *vol_label = gtk_label_new("");
    GtkWidget *bat_label = gtk_label_new("");
    GtkWidget *clock_label = gtk_label_new("");
    
    // Zaradenie na pravú stranu boxu
    gtk_box_pack_end(GTK_BOX(main_box), clock_label, FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(main_box), bat_label, FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(main_box), vol_label, FALSE, FALSE, 4);

    // Naplnenie štruktúry pre odovzdanie do callbacku
    PanelLabels *status_labels = g_new0(PanelLabels, 1);
    status_labels->volume = vol_label;
    status_labels->battery = bat_label;
    status_labels->clock = clock_label;

    // Prvotný update a nastavenie časovača na 1 sekundu (1000ms)
    update_widgets(status_labels);
    g_timeout_add(1000, update_widgets, status_labels);

    // Nastavenie šírky panela na 420px
    int panel_width = 420;
    int panel_height = 32;
    gtk_widget_set_size_request(window, panel_width, panel_height);

    // Aplikovanie moderného tmavého vzhľadu
    apply_css(window, entry, main_box);

    // Signály
    // ZMENA 2: Odstránili sme key-press-event so skratkou Escape, takže panel sa nedá vypnúť z klávesnice.
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate), NULL);

    gtk_widget_show_all(window);

    // Presné centrovanie: stred horizontálne, 100px od vrchu vertikálne
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_primary_monitor(display);
    if (!monitor) {
        monitor = gdk_display_get_monitor(display, 0);
    }
    GdkRectangle geometry;
    gdk_monitor_get_geometry(monitor, &geometry);

    int pos_x = geometry.x + (geometry.width - panel_width) / 2;
    int pos_y = geometry.y + 100;
    gtk_window_move(GTK_WINDOW(window), pos_x, pos_y);

    gtk_main();

    g_free(status_labels);
    return 0;
}


