#ifndef MAIN_H
#define MAIN_H
#include <dirent.h>

struct thd {
	pthread_t thread;
	char *folder;
	int running;
	struct thd *prev;
	struct thd *next;
	int just_started;
};

char *file_hash(char *path);
void check_folder(char *path, int log);
int file_select(const struct dirent *entry);
int is_file(struct stat statbuf);
int is_dir(struct stat statbuf);
void start_lookat(char *path);
void stop_lookat(void);
void stop_path(char *path);
void stop_thread(int t);
void *thread_function(void *arg);
void remove_substring(char *s, const char *toremove);
#endif
