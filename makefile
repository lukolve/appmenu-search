All:
	gcc search.c -o my_search `pkg-config --cflags --libs gtk+-3.0 libwnck-3.0`
