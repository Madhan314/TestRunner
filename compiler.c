#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <json-c/json.h>

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
    FILE *file = fopen("userInput.c", "w");
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

void run_test_case(const char *input_value) {
    FILE *pipe = popen("./userInputProgram > userResult.txt", "w");
    if (pipe == NULL) {
        perror("popen failed");
        return;
    }
    fprintf(pipe, "%s\n", input_value); // Send the value to the other program
    pclose(pipe);
}


// Function to run and compare with manual expected output

void on_compile_compiler_button_clicked(GtkWidget *widget, gpointer data) {
    GError *error = NULL;
    gchar *output = NULL;
    gint exit_status;

    // Compile the compiler.c file and capture both stdout and stderr
    gchar *compile_command = "sh -c 'gcc userInput.c -o userInputProgram 2> compile_error.txt'";
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

    // Run the compiled compilerProgram if needed (optional)
    gchar *run_command = "sh -c './userInputProgram 2>&1'"; // Redirect stderr to stdout
    g_spawn_command_line_sync(run_command, &output, NULL, &exit_status, &error);

    // Check for errors during execution
    if (exit_status != 0) {
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


void on_run_with_args_and_check(GtkWidget *widget, gpointer data) {
    FILE *fp = fopen("privateTestCase.json", "r");
    if (fp == NULL) {
        g_print("Private test case JSON doesn't exist.\n");
        return;
    }

    char buffer[2048];
    fread(buffer, sizeof(char), 2048, fp);
    fclose(fp);

    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(buffer);

    struct json_object *test_cases;
    json_object_object_get_ex(parsed_json, "test_cases", &test_cases);

    int array_len = json_object_array_length(test_cases);
    int done = 1;
    for (int i = 0; i < array_len; i++) {
        struct json_object *test_case = json_object_array_get_idx(test_cases, i);
        struct json_object *input_args;
        struct json_object *expected_output;

        json_object_object_get_ex(test_case, "input", &input_args);
        json_object_object_get_ex(test_case, "expected_output", &expected_output);

        gchar *input_str = g_strdup(json_object_get_string(input_args));
        gchar *expected_str = g_strdup(json_object_get_string(expected_output));

        g_print("Running with input: %s\n", input_str);
        run_test_case(input_str);

        gchar *output = NULL;
        g_spawn_command_line_sync("cat userResult.txt", &output, NULL, NULL, NULL);

        g_print("Your output: %s\n", output);
        g_print("Expected output: %s\n", expected_str);
        
        const char* req = expected_str;
        const char* out = output;
        if (strncmp(req,out,strlen(req)) == 0) {
            g_print("Test Case %d Passed!\n", i + 1);
        } else {
        	if(done){done = 0;}
            g_print("Test Case %d Failed!\n", i + 1);
        }

        g_free(input_str);
        g_free(expected_str);
        g_free(output);
    }
    if(done == 1){
    	exit(250);
    }
    json_object_put(parsed_json);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    initialize_file();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    text_view = gtk_text_view_new();
    output_view = gtk_text_view_new();

    GtkWidget *text_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *output_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(text_scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(output_scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add(GTK_CONTAINER(text_scrolled_window), text_view);
    gtk_container_add(GTK_CONTAINER(output_scrolled_window), output_view);

    GtkWidget *save_button = gtk_button_new_with_label("Save");
    GtkWidget *run_with_args_button = gtk_button_new_with_label("Run with Arguments and Check");
	GtkWidget *compile_compiler_button = gtk_button_new_with_label("Compile Compiler");


    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_button_clicked), NULL);
    g_signal_connect(compile_compiler_button, "clicked", G_CALLBACK(on_compile_compiler_button_clicked), NULL);
    g_signal_connect(run_with_args_button, "clicked", G_CALLBACK(on_run_with_args_and_check), NULL);

    gtk_box_pack_start(GTK_BOX(vbox), text_scrolled_window, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), save_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), compile_compiler_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), run_with_args_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), output_scrolled_window, TRUE, TRUE, 0);
   

    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    gtk_window_set_title(GTK_WINDOW(window), "Code Writer");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_widget_show_all(window);

    gtk_main();
    return 0;
}

