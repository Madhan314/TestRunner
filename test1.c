#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int inputfd[2];

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

        // Write combined output to input_fd
        write(inputfd[1], combined_output, strlen(combined_output));

        // Close read ends
        close(output_pipe[0]);
        close(error_pipe[0]);
        close(inputfd[1]); // Close the write end of input_fd since done writing
        
        // Wait for the child process to finish
        wait(NULL);
    }
}

int main(){
	pipe(inputfd);
	compile();
	
    char buffer[2048];  // Adjust size as needed
    ssize_t bytes_read;

    // Close the write end of input_fd in this context
    close(inputfd[1]); // Close the write end as we are only reading now

    // Read from input_fd
    bytes_read = read(inputfd[0], buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        printf("Received from input_fd:\n%s", buffer); // Print the received output
    } else if (bytes_read == -1) {
        perror("read");
    } else {
        printf("No data read from input_fd\n");
    }

    // Close the read end of input_fd
    close(inputfd[0]);

	return 0;
}
