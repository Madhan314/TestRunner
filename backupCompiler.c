#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

GtkWidget *text_view;
GtkWidget *output_view;

void initialize_file() {
    FILE *file = fopen("userInput.c", "w");
    if (file) {
        fclose(file);
    } else {
        g_print("Error creating userInput.c!\n");
    }
}

void save_input_to_file(const gchar *input) {
    FILE *file = fopen("userInput.c", "w"); // Open in write mode to flush the file
    if (file) {
        fprintf(file, "%s\n", input);
        fclose(file);
    } else {
        g_print("Error writing to userInput.c!\n");
    }
}

void save_output_to_file(const gchar *output) {
    FILE *file = fopen("userResult.txt", "w"); // Overwrite mode
    if (file) {
        fprintf(file, "%s\n", output);
        fclose(file);
    } else {
        g_print("Error creating userResult.txt!\n");
    }
}

void on_save_button_clicked(GtkWidget *widget, gpointer data) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *input = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    
    gchar *trimmed_input = g_strstrip(input);
    
    // Save input to userInput.c (flush the file first)
    save_input_to_file(trimmed_input);
    
    if (g_strcmp0(trimmed_input, "Terminate") == 0) {
        g_free(input);
        gtk_main_quit();
    }

    g_free(input);
}

void on_compile_and_run_button_clicked(GtkWidget *widget, gpointer data) {
    GError *error = NULL;
    gchar *output = NULL;
    gint exit_status;

    // Compile the userInput.c file and capture both stdout and stderr
    gchar *compile_command = "sh -c 'gcc userInput.c -o userInputProgram 2> compile_error.txt'";
// Redirect stderr to a file
    g_spawn_command_line_sync(compile_command, &output, NULL, &exit_status, &error);
    
    GtkTextBuffer *output_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_view));

    // Read the compile error output
    gchar *compile_output = NULL;
    FILE *file = fopen("compile_error.txt", "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        compile_output = malloc(length + 1);
        fread(compile_output, 1, length, file);
        compile_output[length] = '\0'; // Null-terminate the string
        fclose(file);
    }

    if (exit_status != 0) {
        // Compilation failed
        gtk_text_buffer_set_text(output_buffer, compile_output ? compile_output : "Compilation error (no output)", -1);
        save_output_to_file(compile_output ? compile_output : "Compilation error (no output)"); // Save output to file
        if (compile_output) g_free(compile_output);
        return;
    }

    // Clear previous output before running the program
    gtk_text_buffer_set_text(output_buffer, "", -1);

    // Run the compiled program and capture output
    gchar *run_command = "sh -c './userInputProgram 2>&1'"; // Redirect stderr to stdout
    g_spawn_command_line_sync(run_command, &output, NULL, &exit_status, &error);
    
    // Check for errors during execution
    if (exit_status != 0) {
        // Display runtime errors in the output view
        gtk_text_buffer_set_text(output_buffer, output ? output : "Runtime error (no output)", -1);
        save_output_to_file(output ? output : "Runtime error (no output)"); // Save runtime error to file
        g_free(output);
        return;
    }

    // If the program executed successfully, show its output
    gtk_text_buffer_set_text(output_buffer, output ? output : "Program executed successfully, no output.", -1);
    save_output_to_file(output ? output : "Program executed successfully, no output."); // Save output to file

    // Clean up
    g_free(output);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    initialize_file(); // Initialize userInput.c

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    text_view = gtk_text_view_new();
    output_view = gtk_text_view_new();

    // Create scrollable areas for both text and output views
    GtkWidget *text_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *output_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(text_scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(output_scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(text_scrolled_window), text_view);
    gtk_container_add(GTK_CONTAINER(output_scrolled_window), output_view);

    GtkWidget *save_button = gtk_button_new_with_label("Save");
    GtkWidget *compile_run_button = gtk_button_new_with_label("Compile and Run");

    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_button_clicked), NULL);
    g_signal_connect(compile_run_button, "clicked", G_CALLBACK(on_compile_and_run_button_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(vbox), text_scrolled_window, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), save_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), compile_run_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), output_scrolled_window, TRUE, TRUE, 0);
    
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    gtk_window_set_title(GTK_WINDOW(window), "Code Writer");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400); // Adjusted dimensions
    gtk_widget_show_all(window);

    gtk_main();
    return 0;
}

