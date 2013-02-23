/*
 * compiled with: g++ -o crawler crawler.cpp -lpthread -L/usr/local/lib/ -lcurlpp -I/usr/local/include
 * usage: ./crawler
 * you need to put some html documents in the directory "html_docs" to begin crawling
 */


//for input - output
#include <iostream>
#include <cstdio>

//fstream is a standard C++ library that handles reading from and writing to files either in text or in binary formats. It is an object oriented alternative to C's FILE from the C standard library.
#include <fstream>

//general purpose standard library with functions involving memory allocation, process control, conversions and others
#include <cstdlib>

//contains macro definitions, constants, and declarations of functions and types used not only for string handling but also various memory handling functions
//the std::string class is a standard representation for a string of text
#include <cstring>
#include <string>

#include <list>

//to read and write directories
#include <dirent.h>

//for multithreading programming
#include <pthread.h>

#include <unistd.h>

//c++ wrapper for library libcurl
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#define MAX_DIM_URL 255

using namespace std;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

typedef struct UrlElement{
	
	string url;
	int to_visit;
	
}UrlElement;




class FetchQueue{
	
	public:	

			string get_url(){	
			//called by the fetcher returns an url to download
			
				if(url_list.size() == 0){
					
					cout << "FETCHER is waiting for an url to fetch from parser" << endl;
					while(url_list.size() == 0);
				
				}
			
				do{
				
					iter = url_list.begin();
					while(iter != url_list.end() && iter->to_visit == 0){	
				
						iter++;
				
					}
				
					if(iter != url_list.end()){	
				
						newurl = iter->url;
						iter->to_visit = 0;
						
					}else{
						
						cout << "FETCHER is waiting for new urls to fetch from parser" << endl;
						sleep(10);
						
					}
				
				}while(iter == url_list.end());
			
				return newurl;
					
			}
		
			
			void put_url(string element){	
			//called from the parser to put a new url in the list of url to download
				
				if(!strcmp((element.substr(0, 7)).c_str(), "http://")/* || !strcmp((element.substr(0, 8)).c_str(), "https://")*/){
				
					found = search_url(element);
					if(found == 0){
						
						UrlElement newelement;
						newelement.url = element;
						newelement.to_visit = 1;
						url_list.push_back(newelement);
					
					}
				
				}
			
			}
	
	private:

			int found;
			list<UrlElement> url_list;
			string newurl;
			list<UrlElement>::iterator iter;
			
			int search_url(string element){		
			//checks if the url to insert is allready in the list and return 1 if so
				
				for(list<UrlElement>::const_iterator iter = url_list.begin(); iter != url_list.end(); ++iter){
					
					if(!strcmp((iter->url).c_str(), element.c_str())){
						
						return 1;
					}
					
				}
				
				return 0;
				
			}
			
};




class Fetcher		
{
	public:
			string url, title;
			int i, j;
			char modurl[MAX_DIM_URL];
			
			void SaveDoc(FetchQueue *fetchqueue){	
			//saves the html content of the downloaded url in a document contained in the directory "html_docs"
				
				url = fetchqueue->get_url();
				cout << "FETCHER is downloading url " << url.c_str() << endl;
				j = 0;
				for(i = 0; i < url.length(); i++){
					
					if(url[i] != '/' && url[i] != '&' && url[i] != ':' && url[i] != '%' && url[i] != '=' && url[i] != '?' && url[i] != '.'){
						
						modurl[j] = url[i];
						j++;
					
					}

				}
				
				modurl[j] = '\0';
				title = string(modurl);
				try{
					
					pthread_mutex_lock( &mutex1 );
					curlpp::Cleanup myCleanup;
					cout << "FETCHER IN SEMAPHOR" << endl;
					std::ofstream saveddoc;
					saveddoc.open(("html_docs/" + title).c_str());
					if(!saveddoc.good()){
						
						cerr << "Could not write on File !" << endl;
						exit(1);
						
					}
					
					saveddoc << curlpp::options::Url(url.c_str()) << endl;
					saveddoc.close();
					
					pthread_mutex_unlock( &mutex1 );
					cout << "FETCHER OUT SEMAPHOR" << endl;
				
				}

				catch(curlpp::RuntimeError &e){
				
					cout << e.what() << endl;
					pthread_mutex_unlock( &mutex1 );	//in case of catched exceptions frees the mutex
				
				}

				catch(curlpp::LogicError &e){
				
					cout << e.what() << endl;
					pthread_mutex_unlock( &mutex1 );
				
				}
					
			}
			
	private:
};




class HtmlParser
{
	public:
			string doc_title, fileline, shortline;
			char *var, *cstr;
			size_t posini, posfin;
			char url[MAX_DIM_URL];
			
			void ParseHtmlUrl(FetchQueue *fetchqueue){
				//takes a document from the directory "html_docs", parses it looking for tag <a> and gets the url contained in href=""
				//moves the parsed document to the directory "parsed_docs"
				
				do{
					
					ReadDir("html_docs");
					
					doc_title = doc_list.front();
					doc_list.pop_front();
					
					pthread_mutex_lock( &mutex1 );
					cout << "PARSER IN SEMAPHOR" << endl;
					
					std::ifstream file;
					file.open(("html_docs/" + doc_title).c_str());
					
					if(!file.good()){
						cerr << "File '" << "html_docs/" + doc_title << "' does not exist!" << endl;
						exit(1);
					}
				
					while(!file.eof()){
							
						getline(file, fileline);
						posini = fileline.find("<a");
						posfin = fileline.find("</a>");
		
						if(posfin == string::npos || posini == string::npos){
							continue;
						}
		
						shortline = fileline.substr(posini, posfin - posini);
									
						cstr = new char[shortline.size() + 1];
						strcpy(cstr, shortline.c_str());
							
						var = strstr(cstr, "href=\"");
						if(var != NULL){
						
							int i = 0;		
							while(*var != '"'){	
								var++;
							}
						
							var++;
							while(*var != '"' && i < MAX_DIM_URL - 1){	
								url[i] = *var;
								var++;
								i++;
							}
											
							url[i] = '\0';
								
							if(i < MAX_DIM_URL){
								cout << "extracted url: - " << url << " -" << endl;						
								fetchqueue->put_url(url);
							}
							
						}
									
						delete[] cstr;
					
					}
				
					file.close();
									
					pthread_mutex_unlock( &mutex1 );
					cout << "PARSER OUT SEMAPHOR" << endl;
				
					MoveFile(doc_title, "parsed_docs/");
					cout << "PARSER has successfully parsed from document " << doc_title << endl;
				
				}while(doc_list.size() != 0);
				
			}
			
			void ParseHtmlImage(){
				//will be implemented in a future version
			}
	
	private:
			list<string> doc_list;
			string line, filepath;
			DIR *dp;
			struct dirent *dirp;
			
			void ReadDir(string dir){
			//reads the documents contained in the folder specified and pushes them in a list (for the ParseHtmlUrl method)
				
				int i;
				do{
			
					i = 0;
					doc_list.erase(doc_list.begin(), doc_list.end());
					dp = opendir(dir.c_str());
					if (dp == NULL){
			
						cout << "Error opening directory" << dir << endl;
						exit(1);
			
					}

					while (dirp = readdir(dp)){
			
						i++;
						if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."));
						else{
			
							filepath = dir + "/" + dirp->d_name;
							doc_list.push_back(dirp->d_name);
							
						}
											
					}

					closedir(dp);
					if(i == 2){
			
						cout << "Directory is empty, no documents to parse" << endl;
						sleep(10);
			
					}
				
				}while(i <= 2);
							
			}
			
			void MoveFile(string filename, string newfilepath){
			//moves the file with the title in input to the new file path
				
				pthread_mutex_lock( &mutex1 );

				cout << "MOVE FILE IN SEMAPHOR" << endl;
				ifstream oldfile;
				oldfile.open(("html_docs/" + filename).c_str());
				if(!oldfile.good()){
					
					cerr << "File '" << filename << "' does not exist!" << endl;
					exit(1);
				
				}
				
				ofstream newfile;
				newfile.open((newfilepath + filename).c_str());
				if (!newfile.good()) {
				
					cerr << "Could not write on File '" << newfilepath + filename << "'!" << endl;
					exit(1);
				
				}
				
				while(!oldfile.eof()){
					
					getline(oldfile, line);
					newfile << line << endl;
					
				}
				
				oldfile.close();
				newfile.close();
				
				if(remove(("html_docs/" + filename).c_str()) != 0)
					perror("Error deleting file");
					
				pthread_mutex_unlock( &mutex1 );	
				cout << "MOVE FILE OUT SEMAPHOR" << endl;
					
			}
			
};




void *fetch(void *arg){
	//fetching thread
	
	Fetcher fetcher;
	FetchQueue *fetchqueue;
	fetchqueue = ((FetchQueue *)arg);
	
	while(true){
		
		fetcher.SaveDoc(fetchqueue);
	
	}
}




int main(int argc, char** argv){
	
	FetchQueue *fetchqueue;
	fetchqueue = new FetchQueue();
	
	//parsing thread
	
	HtmlParser parser;
		
	pthread_t tid;
	
	if(pthread_create(&tid, NULL, fetch, fetchqueue) != 0){
	
		perror("Thread creation");
	
	}
	
	while(true){
	
		parser.ParseHtmlUrl(fetchqueue);
	
	}
}
