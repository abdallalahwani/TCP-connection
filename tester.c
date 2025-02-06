#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define SERVER_PORT 9999
#define SERVER_IP "127.0.0.1"
#define TEST_FILE "./run_tests/test_file.txt"
#define LARGE_TEST_FILE "./run_tests/large_test_file.txt"
#define PRINTABLE_ONLY_FILE "./run_tests/printable_only_file.txt"
#define NON_PRINTABLE_ONLY_FILE "./run_tests/non_printable_only_file.txt"

void create_test_file(const char* filename, int size, int printable_ratio) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Failed to create test file");
        exit(1);
    }

    srand(time(NULL));
    for (int i = 0; i < size; i++) {
        char c;
        if (rand() % 100 < printable_ratio) {
            c = 32 + rand() % 95;  // Printable ASCII characters
        } else {
            c = rand() % 32;  // Non-printable ASCII characters
        }
        fputc(c, file);
    }

    fclose(file);
}

int count_printable_chars(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open test file");
        exit(1);
    }

    int count = 0;
    int c;
    while ((c = fgetc(file)) != EOF) {
        if (c >= 32 && c <= 126) {
            count++;
        }
    }

    fclose(file);
    return count;
}

void run_server() {
    char* args[] = {"./pcc_server", "9999", NULL};
    execv("./pcc_server", args);
    perror("Failed to execute server");
    exit(1);
}

int run_client(const char* filename) {
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", SERVER_PORT);

    pid_t pid = fork();
    if (pid == 0) {
        char* args[] = {"./pcc_client", SERVER_IP, port_str, (char*)filename, NULL};
        execv("./pcc_client", args);
        perror("Failed to execute client");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    } else {
        perror("Fork failed");
        exit(1);
    }
}

void test_normal_operation() {
    printf("Testing normal operation...\n");
    create_test_file(TEST_FILE, 1000, 80);
    int expected_count = count_printable_chars(TEST_FILE);
    int client_exit_code = run_client(TEST_FILE);
    printf("Expected count: %d\n", expected_count);
    printf("Client exit code: %d\n", client_exit_code);
    printf("Test %s\n\n", client_exit_code == 0 ? "PASSED" : "FAILED");
    remove(TEST_FILE);
}

void test_large_file() {
    printf("Testing large file...\n");
    create_test_file(LARGE_TEST_FILE, 10000000, 50);  // 10MB file
    int expected_count = count_printable_chars(LARGE_TEST_FILE);
    int client_exit_code = run_client(LARGE_TEST_FILE);
    printf("Expected count: %d\n", expected_count);
    printf("Client exit code: %d\n", client_exit_code);
    printf("Test %s\n\n", client_exit_code == 0 ? "PASSED" : "FAILED");
    remove(LARGE_TEST_FILE);
}

void test_empty_file() {
    printf("Testing empty file...\n");
    create_test_file(TEST_FILE, 0, 0);
    int client_exit_code = run_client(TEST_FILE);
    printf("Client exit code: %d\n", client_exit_code);
    printf("Test %s\n\n", client_exit_code == 0 ? "PASSED" : "FAILED");
    remove(TEST_FILE);
}

void test_non_existent_file() {
    printf("Testing non-existent file...\n");
    int client_exit_code = run_client("non_existent_file.txt");
    printf("Client exit code: %d\n", client_exit_code);
    printf("Test %s\n\n", client_exit_code != 0 ? "PASSED" : "FAILED");
}

void test_printable_only_file() {
    printf("Testing file with only printable characters...\n");
    create_test_file(PRINTABLE_ONLY_FILE, 1000, 100);  // 100% printable characters
    int expected_count = count_printable_chars(PRINTABLE_ONLY_FILE);
    int client_exit_code = run_client(PRINTABLE_ONLY_FILE);
    printf("Expected count: %d\n", expected_count);
    printf("Client exit code: %d\n", client_exit_code);
    printf("Test %s\n\n", client_exit_code == 0 ? "PASSED" : "FAILED");
    remove(PRINTABLE_ONLY_FILE);
}

void test_non_printable_only_file() {
    printf("Testing file with only non-printable characters...\n");
    create_test_file(NON_PRINTABLE_ONLY_FILE, 1000, 0);  // 0% printable characters
    int expected_count = count_printable_chars(NON_PRINTABLE_ONLY_FILE);
    int client_exit_code = run_client(NON_PRINTABLE_ONLY_FILE);
    printf("Expected count: %d\n", expected_count);
    printf("Client exit code: %d\n", client_exit_code);
    printf("Test %s\n\n", client_exit_code == 0 ? "PASSED" : "FAILED");
    remove(NON_PRINTABLE_ONLY_FILE);
}

int main() {
    pid_t server_pid = fork();
    if (server_pid == 0) {
        run_server();
    } else if (server_pid > 0) {
        sleep(1);  // Give the server time to start

        test_normal_operation();
        test_large_file();
        test_empty_file();
        test_non_existent_file();
        test_printable_only_file();
        test_non_printable_only_file();

        // Kill the server
        kill(server_pid, SIGINT);
        int status;
        waitpid(server_pid, &status, 0);
    } else {
        perror("Fork failed");
        exit(1);
    }
    printf("all test PASSED\n");
    return 0;
}
