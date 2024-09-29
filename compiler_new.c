#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <json-c/json.h>

GtkWidget *text_view;
GtkWidget *output_view;
GtkWidget *question_view;
GtkWidget *custom_output_text_view;
GtkWidget *custom_output_checkbox;

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
    FILE *file = fopen("userResult.txt", "w");
    if (file) {
        fprintf(file, "%s\n", output);
        fclose(file);
    } else {
        g_print("Error creating userResult.txt!\n");
    }
}

char* getSampleTestCases() {
    FILE *fp = fopen("sampleTestCase.json", "r");
    if (fp == NULL) {
        printf("Json doesn't exist.\n");
        return NULL;
    }

    char buffer[2048];
    fread(buffer, sizeof(char), 2048, fp);
    fclose(fp);

    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(buffer);

    struct json_object *test_cases;
    json_object_object_get_ex(parsed_json, "test_cases", &test_cases);

    int array_len = json_object_array_length(test_cases);
    char *result = malloc(1024 * sizeof(char)); // Allocate enough memory for the result
    strcpy(result, "Sample test cases:\n\n");

    for (int i = 0; i < array_len; i++) {
        struct json_object *test_case = json_object_array_get_idx(test_cases, i);
        struct json_object *case_id;
        struct json_object *input;
        struct json_object *expected_output;

        json_object_object_get_ex(test_case, "case_id", &case_id);
        json_object_object_get_ex(test_case, "input", &input);
        json_object_object_get_ex(test_case, "expected_output", &expected_output);

        // Append test case details to the result string
        char buffer[4096]; // Temporary buffer for each case
        snprintf(buffer, sizeof(buffer), "Test Case %d:\nInput: %s\nExpected Output: %s\n\n",
                 json_object_get_int(case_id),
                 json_object_get_string(input),
                 json_object_get_string(expected_output));
        strcat(result, buffer); // Concatenate to the result
    }

    json_object_put(parsed_json);
    return result; // Return the result string
}


void load_question() {
    gchar *content = NULL;
    gsize length;
    gboolean success = g_file_get_contents("question.txt", &content, &length, NULL);
    char *test_cases = getSampleTestCases();
    
    if (success) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(question_view));
        if (test_cases != NULL) {
    	    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(question_view)), test_cases, -1);
    	    free(test_cases); // Free the allocated memory
    	}
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
    fprintf(pipe, "%s\n", input_value);
    pclose(pipe);
}

void on_compile_compiler_button_clicked(GtkWidget *widget, gpointer data) {
    GError *error = NULL;
    gchar *output = NULL;
    gint exit_status;

    gchar *compile_command = "sh -c 'gcc userInput.c -o userInputProgram 2> compile_error.txt'";
    g_spawn_command_line_sync(compile_command, &output, NULL, &exit_status, &error);

    GtkTextBuffer *output_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_view));
    gchar *compile_output = NULL;
    
    FILE *file = fopen("compile_error.txt", "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        compile_output = malloc(length + 1);
        fread(compile_output, 1, length, file);
        compile_output[length] = '\0';
        fclose(file);
    }

    if (exit_status != 0) {
        gtk_text_buffer_set_text(output_buffer, compile_output ? compile_output : "Compilation error (no output)", -1);
        save_output_to_file(compile_output ? compile_output : "Compilation error (no output)");
        if (compile_output) g_free(compile_output);
        return;
    }

    gtk_text_buffer_set_text(output_buffer, "", -1);
    gchar *run_command = "sh -c './userInputProgram 2>&1'";
    g_spawn_command_line_sync(run_command, &output, NULL, &exit_status, &error);

    if (exit_status != 0) {
        gtk_text_buffer_set_text(output_buffer, output ? output : "Runtime error (no output)", -1);
        save_output_to_file(output ? output : "Runtime error (no output)");
        g_free(output);
        return;
    }

    gtk_text_buffer_set_text(output_buffer, output ? output : "Program executed successfully, no output.", -1);
    save_output_to_file(output ? output : "Program executed successfully, no output.");
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
        if (strncmp(req, out, strlen(req)) == 0) {
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
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
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
    GtkWidget *code_label = gtk_label_new("Code:");
    text_view = gtk_text_view_new();
    gtk_widget_set_size_request(text_view, -1, 400);
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

    // Button box at the bottom
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *save_button = gtk_button_new_with_label("Save");
    GtkWidget *compile_button = gtk_button_new_with_label("Compile");
    GtkWidget *run_button = gtk_button_new_with_label("Run");
    GtkWidget *run_test_button = gtk_button_new_with_label("Run Test Cases");

    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_button_clicked), NULL);
    g_signal_connect(compile_button, "clicked", G_CALLBACK(on_compile_compiler_button_clicked), NULL);
    g_signal_connect(run_test_button, "clicked", G_CALLBACK(on_run_with_args_and_check), NULL);

    gtk_box_pack_start(GTK_BOX(button_box), save_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), compile_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), run_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), run_test_button, TRUE, TRUE, 0);

    // Assembling main layout
    gtk_box_pack_start(GTK_BOX(output_vbox), custom_output_checkbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(output_vbox), custom_output_scrolled_window, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(output_vbox), output_scrolled_window, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(right_vbox), code_scrolled_window, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(right_vbox), output_vbox, TRUE, TRUE, 0);
    
    gtk_box_pack_start(GTK_BOX(top_hbox), question_vbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(top_hbox), right_vbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), top_hbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), button_box, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), main_box);
    gtk_window_set_title(GTK_WINDOW(window), "Code Writer");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_widget_show_all(window);
    
    load_question();

    gtk_main();
    return 0;
}


