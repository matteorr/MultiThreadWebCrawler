all:crawler

crawler:multithreadcrawler.o
	g++ multithreadcrawler.o -lpthread -L/usr/local/lib/ -lcurlpp -I/usr/local/include -o crawler

multithreadcrawler.o:multithreadcrawler.cpp
	g++ -c multithreadcrawler.cpp

clean:
	rm -rf *o crawler parsed_docs/* html_docs/*

