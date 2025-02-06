#!/bin/bash

compiling_files() {
    echo "starting compilation"
    gcc -o pcc_server pcc_server.c
    gcc -o pcc_client pcc_client.c
    echo "compilation finished"
}

# Function to start the server in the background
start_server() {
    ./pcc_server 8080 &  # Start server in background
    SERVER_PID=$!
    echo "Server started"
}

# Function to stop the server by sending SIGINT
stop_server() {
    echo "Sending SIGINT to server"
    kill -SIGINT $SERVER_PID
    wait $SERVER_PID
    echo "Server stopped."
}

# Run tests with SIGINT at specific points
run_tests_sigint() {
    start_server
    sleep 2  # Ensure the server starts properly

    for i in {1..5}; do
        case $i in
            1) TEST_FILE="testfile.txt";;
            2) TEST_FILE="emptyfile.txt";;
            3) TEST_FILE="non_existent_file.txt";;
            4) TEST_FILE="largefile.txt";;
            5) TEST_FILE="non_printable.txt";;
        esac

        echo "Running test $i: $TEST_FILE"
        ./pcc_client 127.0.0.1 8080 "$TEST_FILE"

        if [ "$1" -eq "$i" ]; then
            kill -SIGINT $SERVER_PID
            wait $SERVER_PID 2>/dev/null  # Ensure we wait for the process to exit
            echo "SIGINT sent during test $i"
        fi
        echo "=============================="
    done
}

# Compile files
compiling_files

# Run tests and capture output
run_tests_sigint 0
stop_server

run_tests_sigint 1
run_tests_sigint 2
run_tests_sigint 3
run_tests_sigint 4
run_tests_sigint 5

# Indicate end of tests
echo "Tests completed."
