# helanam

Tested on Ubuntu 18.04.

Run: ./main lemmad.txt word

Compile: gcc main.c -O3 -o main

Expects lemmad.txt in a format as provided: 
* Windows-1257 (CP1257) encoding,
* rows ordered by byte value
* \r\n for line breaks.

For command line arguments encoding Windows-1257 is preferred (easy to set in Putty), but attempts to decode relevant UTF8 chars to Windows-1257.
