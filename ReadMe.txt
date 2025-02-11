To use this tester:

1. Compile your pcc_server.c and pcc_client.c:
gcc -o pcc_server -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_server.c
gcc -o pcc_client -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_client.c

Compile the tester:
gcc -o tester tester.c

Run the tester:
./tester

This tester covers basic functionality and some edge cases.
However, it doesn't cover all possible scenarios or check the server's statistics output.
You might want to extend it to cover more cases or add more detailed checks based on your
specific requirements.

Also, note that this tester assumes that your server and client are working correctly.
If they're not, you'll need to debug them separately.