#!/bin/bash

# Run the test script and capture output
bash test_script.sh > output.txt 2>&1

# Compare output and print line numbers of mismatches
echo "Differences found at line numbers:"
diff -y --suppress-common-lines output.txt expected_output.txt | nl

