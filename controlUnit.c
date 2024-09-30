#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <json-c/json.h>
#include <ctype.h>
#include <sys/wait.h>
#include <gtk/gtk.h>
#include <glib.h>

int inputfd[2], outputfd[2];

GtkWidget *text_view;
GtkWidget *output_view;
GtkWidget *question_view;
GtkWidget *custom_output_text_view;
GtkWidget *custom_output_checkbox;

void store_input_file(char* input) {
    FILE *file = fopen("userInput.c", "w");
    if (file == NULL) {
        g_print("Error opening file!\n");
        return;
    }
    fprintf(file, "%s\n", input);
    fclose(file);
}

void compile() {
    int output_pipe[2]; // Pipe for standard output
    int error_pipe[2];  // Pipe for standard error
    pid_t pid;

    // Create pipes
    if (pipe(output_pipe) == -1 || pipe(error_pipe) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork a child process
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        // Close read ends of pipes
        close(output_pipe[0]);
        close(error_pipe[0]);

        // Redirect stdout to the output pipe
        dup2(output_pipe[1], STDOUT_FILENO);
        // Redirect stderr to the error pipe
        dup2(error_pipe[1], STDERR_FILENO);
        
        // Close the original write ends
        close(output_pipe[1]);
        close(error_pipe[1]);

        // Execute the command to compile and run the program
        char command[1024];
        snprintf(command, sizeof(command), "rm -f userInput; gcc userInput.c -o userInput; if [ $? -eq 0 ]; then ./userInput; fi");
        execlp("sh", "sh", "-c", command, NULL);
        
        // If execlp fails
        perror("execlp");
        exit(EXIT_FAILURE);
    } else { // Parent process
        // Close write ends of pipes
        close(output_pipe[1]);
        close(error_pipe[1]);

        char output_buffer[1024] = {0}; // Buffer for output
        char error_buffer[1024] = {0};  // Buffer for error
        ssize_t bytes_read;

        // Read from the output pipe
        while ((bytes_read = read(output_pipe[0], output_buffer + strlen(output_buffer), sizeof(output_buffer) - strlen(output_buffer) - 1)) > 0) {
            // Null-terminate the string
            output_buffer[bytes_read + strlen(output_buffer)] = '\0'; 
        }

        // Read from the error pipe
        while ((bytes_read = read(error_pipe[0], error_buffer + strlen(error_buffer), sizeof(error_buffer) - strlen(error_buffer) - 1)) > 0) {
            // Null-terminate the string
            error_buffer[bytes_read + strlen(error_buffer)] = '\0'; 
        }

        // Combine output and error messages
        char combined_output[2048]; // Adjust size as needed
        snprintf(combined_output, sizeof(combined_output), "%s\n%s", output_buffer, error_buffer);
        printf("From fn call:\n%s\n",combined_output);
        // Write combined output to input_fd
        write(inputfd[1], combined_output, strlen(combined_output));

        // Close read ends
        close(output_pipe[0]);
        close(error_pipe[0]);
        //close(inputfd[1]); // Close the write end of input_fd since done writing
        
        // Wait for the child process to finish
        wait(NULL);
    }
}




/*
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

*/

/*void on_run_with_args_and_check(GtkWidget *widget, gpointer data) {
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
}*/

void monitor_child(){

    char buffer[1024];

    while (1) {
        // Read from the pipe
        ssize_t bytes_read = read(outputfd[0], buffer, sizeof(buffer));
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0'; // Null-terminate the string
            int button_index;
            char *data;
			//printf("Buffer caught: %s\n",buffer);
            // Parse the message
            char *colon_pos = strchr(buffer, ':');
            char string[1024];
   
   	    	// Extract the number before the colon
        	sscanf(buffer, "%d", &button_index);
        	int ind = 0;
        	for(int i = 0;i<strlen(buffer);i++){
        		if(buffer[i-1]==':'){
        			int j = i;
        			char ch=buffer[j];
        			while(ch!='\0'){
        				ch= buffer[j++];
        				string[ind++]=ch; 
        			}
        			string[ind] = '\0';
        			break;
        		}
        	}
        	// Copy everything after the colon (including whitespace)
        	//strncpy(data, colon_pos + 1, sizeof(data) - 1);
            //sscanf(buffer, "%d:%s", &button_index, data);
            //printf("%d clicked\n",button_index);
            // Process the data based on button index
            switch (button_index) {
                case 0:
                    store_input_file(string);
                    break;
                case 1:
                    compile();
                    break;
                default:
                    printf("Unknown button index.\n");
                    break;
            }
        }
    }
}


void initialize_file() {
    FILE *file = fopen("userInput.c", "w");
    if (file) {
        fclose(file);
    } else {
        g_print("Error creating userInput.c!\n");
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
    
    char *trimmed_input = g_strstrip(input);
    //printf("%s\n",trimmed_input);
    int button_index = GPOINTER_TO_INT(data);
    //printf("Button index from save: %d\n",button_index);

    // Prepare the message to send (e.g., "index:data")
    char message[1024];
    snprintf(message, sizeof(message), "%d:%s", button_index, trimmed_input);
    write(outputfd[1], message, strlen(message) + 1);
    
    g_free(input);
}
void on_compile_and_run_button_clicked(GtkButton *button, gpointer user_data) {
    // Ensure that file descriptors are valid before closing
    if (inputfd[1] != -1) {
        close(inputfd[1]); // Only close if it hasn't been closed already
    }
    if (outputfd[0] != -1) {
        close(outputfd[0]); // Only close if it hasn't been closed already
    }
    
    int button_index = GPOINTER_TO_INT(user_data);
    char message[10];
    snprintf(message, sizeof(message), "%d:%s", button_index, "\0");
    
    // Write button index message to output_fd
    ssize_t bytes_written = write(outputfd[1], message, strlen(message) + 1);
    if (bytes_written == -1) {
        perror("write to outputfd failed");
        return;
    }

    // Prepare to read from input_fd
    char buffer[2048];  // Buffer for output
    ssize_t bytes_read;
    
    wait(10);
    // Read from input_fd
    bytes_read = read(inputfd[0], buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_view));
        gtk_text_buffer_set_text(text_buffer, buffer, -1);
    } else if (bytes_read == -1) {
        perror("read from input_fd failed");  // Handle read error
        GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_view));
        gtk_text_buffer_set_text(text_buffer, "Error reading from input_fd.", -1);
    } else {
        GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_view));
        gtk_text_buffer_set_text(text_buffer, "No output received.", -1);
    }

    // Close the read end of input_fd after usage
    if (inputfd[0] != -1) {
        close(inputfd[0]);
        inputfd[0] = -1; // Set to -1 to avoid closing again
    }
}


/*void on_compile_and_run_button_clicked(GtkWidget *widget, gpointer data) {
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
*/



int main(int argc, char* argv[]){

	pipe(inputfd);
	pipe(outputfd);
	
	int pid = fork();
	if(pid == -1){
		printf("Not forked");
		exit(1);
	}
	else if(pid==0){

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

        g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_button_clicked), GINT_TO_POINTER(0));
        g_signal_connect(compile_run_button, "clicked", G_CALLBACK(on_compile_and_run_button_clicked), GINT_TO_POINTER(1));

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
	}
	else{
        close(inputfd[0]);
        close(outputfd[1]);
        monitor_child();
        wait(NULL);
	}
	
	return 0;
}
