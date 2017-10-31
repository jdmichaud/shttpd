all:
	cc -Wall -Wextra -Wpedantic -Wfatal-errors main.c httpd.c -o shttpd
static:
	cc -Wall -Wextra -Wpedantic -Wfatal-errors main.c httpd.c -o shttpd -static
test:
	cc -Wall -Wextra -Wpedantic -Wfatal-errors test.c httpd.c -o testshttpd && ./testshttpd
clean:
	rm -fr shttpd testshttpd
