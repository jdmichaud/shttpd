all:
	cc -Wall -Wextra -Wpedantic -Wfatal-errors main.c httpd.c -o shttpd
test:
	cc -Wall -Wextra -Wpedantic -Wfatal-errors test.c httpd.c -o testshttpd && ./testshttpd
clean:
	rm -fr shttpd testshttpd
