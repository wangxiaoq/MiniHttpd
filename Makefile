CPPFLAGS += -lpthread
CPP = g++

minihttpd: mini-httpd.o serve-client.o
	$(CPP) $^ -o $@ $(CPPFLAGS)
	make -C cgi
	mv cgi/cgi_math www/cgi_bin

mini-httpd.o: mini-httpd.cpp mini-httpd.h
	$(CPP) -c $<

serve-client.o: serve-client.cpp serve-client.h
	$(CPP) -c $<

.PHONY: clean
clean:
	rm *.o
	rm minihttpd
	rm www/cgi_bin/cgi_math
