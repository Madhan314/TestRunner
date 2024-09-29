#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

GtkWidget *text_view;
GtkWidget *output_view;
GtkWidget *question_view;
GtkWidget *custom_output_text_view;
GtkWidget *custom_output_checkbox;

void initialize_file() {
    FILE *file = fopen("output_program.c", "w");
    if (file) {
        fclose(file);
    } else {
        g_print("Error creating file!\n");
    }
}

void load_question() {
    gchar *content = NULL;
    gsize length;
    gboolean success = g_file_get_contents("question.txt", &content, &length, NULL);
    if (success) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(question_view));
        gtk_text_buffer_set_text(buffer, content, -1);
        g_free(content);
    } else {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(question_view));
        gtk_text_buffer_set_text(buffer, "Loading question...", -1);
    }
}

void on_save_button_clicked(GtkWidget *widget, gpointer data) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *input = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    
    gchar *trimmed_input = g_strstrip(input);
    
    FILE *file = fopen("output_program.c", "w");
    if (file == NULL) {
        g_print("Error opening file!\n");
        g_free(input);
        return;
    }

    if (g_strcmp0(trimmed_input, "Terminate") == 0) {
        fclose(file);
        g_free(input);
        gtk_main_quit();
    } else {
        fprintf(file, "%s\n", trimmed_input);
        fclose(file);
    }
    g_free(input);
}

void on_compile_and_run_button_clicked(GtkWidget *widget, gpointer data) {
    GError *error = NULL;
    gchar *output = NULL;
    gint exit_status;

    gchar *command = "gcc output_program.c -o output_program";
    g_spawn_command_line_sync(command, &output, NULL, &exit_status, &error);
    
    GtkTextBuffer *output_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_view));
    
    if (exit_status != 0) {
        gtk_text_buffer_set_text(output_buffer, output ? output : "Compilation error (no output)", -1);
        g_free(output);
        return;
    }

    gtk_text_buffer_set_text(output_buffer, "", -1);
    command = "./output_program";
    g_spawn_command_line_sync(command, &output, NULL, &exit_status, &error);
    
    if (error) {
        gtk_text_buffer_set_text(output_buffer, error->message, -1);
        g_error_free(error);
        g_free(output);
        return;
    }

    gtk_text_buffer_set_text(output_buffer, output ? output : "Program executed successfully, no output.", -1);
    g_free(output);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    initialize_file();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // Top section for the question and code
    GtkWidget *top_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    // Left half for the question
    GtkWidget *question_label = gtk_label_new("Question:");
    question_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(question_view), FALSE);
    GtkWidget *question_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(question_scrolled_window), question_view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(question_scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *question_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(question_vbox), question_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(question_vbox), question_scrolled_window, TRUE, TRUE, 0);

    // Right half for code and output
    GtkWidget *right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // Code area
    GtkWidget *code_label = gtk_label_new("Code:");
    text_view = gtk_text_view_new();
    gtk_widget_set_size_request(text_view, -1, 400); // Increased height
    GtkWidget *code_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(code_scrolled_window), text_view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(code_scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    // Custom input and output area
    GtkWidget *output_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    custom_output_checkbox = gtk_check_button_new_with_label("Custom Output");
    custom_output_text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(custom_output_text_view), TRUE);
    GtkWidget *custom_output_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(custom_output_scrolled_window), custom_output_text_view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(custom_output_scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(custom_output_text_view, -1, 60); // Reduced height

    output_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(output_view), FALSE);
    GtkWidget *output_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(output_scrolled_window), output_view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(output_scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    // Pack the output and custom input sections
    gtk_box_pack_start(GTK_BOX(output_vbox), custom_output_checkbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(output_vbox), custom_output_scrolled_window, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(output_vbox), output_scrolled_window, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(right_vbox), code_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_vbox), code_scrolled_window, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(right_vbox), output_vbox, TRUE, TRUE, 0);
    
    // Pack everything into the top layout
    gtk_box_pack_start(GTK_BOX(top_hbox), question_vbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(top_hbox), right_vbox, TRUE, TRUE, 0);
    
    // Button box at the bottom
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *save_button = gtk_button_new_with_label("Save");
    GtkWidget *compile_run_button = gtk_button_new_with_label("Compile and Run");

    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_button_clicked), NULL);
    g_signal_connect(compile_run_button, "clicked", G_CALLBACK(on_compile_and_run_button_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(button_box), save_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), compile_run_button, TRUE, TRUE, 0);
    
    // Pack everything into the main layout
    gtk_box_pack_start(GTK_BOX(main_box), top_hbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), button_box, FALSE, FALSE, 0); // Add button box at the bottom
    
    gtk_container_add(GTK_CONTAINER(window), main_box);
    
    gtk_window_set_title(GTK_WINDOW(window), "Code Writer");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_widget_show_all(window);

    load_question();

    gtk_main();
    return 0;
}
 
