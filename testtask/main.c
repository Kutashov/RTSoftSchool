#include "md5.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/param.h>
#include <pthread.h>
#include <sys/stat.h>
#include <libgen.h>
#include <unistd.h>
#include "main.h"

const int hash_len = 16;
const int buf_len = 64;
const int command_size = 256;

const char *const usage = "\
Usage:\n\
	./lookat [-t time] [-d dirs]	\n\
		> add dir	\n\
		> forget dir	\n\
		> quit	\n\
";

int thread_count;
int sleep_time = 5;
struct thd *threads;

char pathname[MAXPATHLEN];

int main(int argc, char *argv[])
{
	thread_count = 0;
	threads = NULL;
	int i, dir_mode = 0;

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-t") == 0) {
			dir_mode = 0;
			sleep_time = atoi(argv[++i]);
			continue;
		}

		if (strcmp(argv[i], "-d") == 0) {
			dir_mode = 1;
			continue;
		}

		if (dir_mode)
			start_lookat(argv[i]);
		else {
			printf("%s", usage);
			_exit(EXIT_FAILURE);
		}

	}


	char command[command_size];

	while (1) {
		strcpy(command, "\0");
		printf("> ");
		fgets(command, sizeof(command), stdin);
		remove_substring(command, "\n");

		if (strcmp(command, "") == 0)
			continue;

		if (strcmp(command, "help") == 0)
			printf("%s", usage);


		if (strcmp(command, "quit") == 0) {
			printf("closing.. wait a while\n");
			stop_lookat();
			break;
		}

		if (strstr(command, "add") != NULL) {
			remove_substring(command, "add ");
			start_lookat(command);
		}

		if (strstr(command, "forget") != NULL) {
			remove_substring(command, "forget ");
			stop_path(command);
		}
	}

	_exit(0);
}

void stop_lookat(void)
{
	int i;

	for (i = thread_count - 1; i >= 0; --i)
		stop_thread(i);
}

void start_lookat(char *path)
{

	struct stat new_fol;

	if (stat(path, &new_fol) != 0 || !S_ISDIR(new_fol.st_mode)) {
		printf("Not a folder\n");
		return;
	}

	struct thd *exists = threads;

	while (exists != NULL && strstr(exists->folder, path) != 0)
		exists = exists->next;
	if (exists != NULL) {
		printf("This folder is already under our protection!\n");
		return;
	}

	exists = threads;
	while (exists != NULL && strstr(path, exists->folder) != 0)
		exists = exists->next;
	if (exists != NULL) {
		free(exists->folder);
		exists->folder = malloc(MAXPATHLEN * sizeof(char));
		strcpy(exists->folder, path);
		return;
	}

	++thread_count;
	struct thd *curr = malloc(sizeof(struct thd));

	curr->folder = malloc(MAXPATHLEN * sizeof(char));
	strcpy(curr->folder, path);

	curr->running = 1;
	curr->just_started = 1;

	int res = pthread_create(&curr->thread,
		NULL, thread_function, curr);
	if (res != 0) {
		perror("Thread creation failed");
		_exit(EXIT_FAILURE);
	}

	curr->prev = NULL;
	curr->next = threads;
	if (curr->next != NULL)
		curr->next->prev = curr;
	threads = curr;
}

void *thread_function(void *arg)
{
	struct thd *t = (struct thd *) arg;
	char *folder;

	while (t->running) {
		folder = strdup(t->folder);
		check_folder(folder, !t->just_started);
		if (check_deleted_files(folder)) {
			stop_path(folder);
		} else {
			t->just_started = 0;
			/*print_tree();*/
			sleep(sleep_time);
		}
		free(folder);
	}
}

void stop_path(char *path)
{

	struct thd *curr = threads;

	while (curr != NULL && strcmp(curr->folder, path) != 0)
		curr = curr->next;

	if (curr == NULL) {
		printf("not found\n");
		return;
	}

	curr->running = 0;
	int res = pthread_join(curr->thread, NULL);

	if (res != 0) {
		perror("Thread join-failed");
		_exit(EXIT_FAILURE);
	}
	delete_folder(curr->folder);
	free(curr->folder);
	if (curr == threads) {
		threads = curr->next;
	} else {
		curr->prev->next = curr->next;
		if (curr->next != NULL)
			curr->next->prev = curr->prev;
	}
	free(curr);
	--thread_count;
}

void stop_thread(int t)
{

	struct thd *curr = threads;

	while (--t >= 0)
		curr = curr->next;

	curr->running = 0;
	int res = pthread_join(curr->thread, NULL);

	if (res != 0) {
		perror("Thread join-failed");
		_exit(EXIT_FAILURE);
	}
	free(curr->folder);
	if (curr == threads) {
		threads = curr->next;
	} else {
		curr->prev->next = curr->next;
		if (curr->next != NULL)
			curr->next->prev = curr->prev;
	}
	free(curr);
	--thread_count;
}

char *file_hash(char *path)
{

	int fd = open(path, O_RDONLY);

	if (fd == -1) {
		printf("Can't open file %s\n", path);
	    return "";
	}

	int status = 0;

	md5_state_t state;
	md5_byte_t digest[hash_len];
	char *hex = malloc(hash_len*2 + 1);
	char buf[buf_len];

	int di, n;

	md5_init(&state);
	while ((n = read(fd, buf, buf_len-1)) > 0)
		md5_append(&state, (const md5_byte_t *)buf, n);
	md5_finish(&state, digest);

	for	(di = 0; di < hash_len; ++di)
		sprintf(hex + di * 2, "%02x", digest[di]);

	close(fd);

	return hex;
}

void check_folder(char *path, int log)
{
	/*printf("folder %s\n", path);*/
	add_folder(path, log);
	int i, err_code;
	struct stat statbuf;
	char file[MAXPATHLEN];
	struct dirent **files;

	int count = scandir(path, &files,
				file_select, NULL);


	for (i = 0; i < count; ++i) {
		strcpy(file, path);
		strcat(file, "/");
		strcat(file, files[i]->d_name);

		err_code = stat(file, &statbuf);
		if (err_code == -1) {
			printf("Can't open file %s\n", file);
			continue;
		}

		if (is_file(statbuf)) {
			add_file(file, file_hash(file), log);
			/*printf("file %s\n", file);*/

		} else if (is_dir(statbuf)) {
			check_folder(file, log);
		}
	}

	if (count != -1) {
		for (i = 0; i < count; ++i)
			free(files[i]);
		free(files);
	}

}

int file_select(const struct dirent *entry)
{
	if ((strcmp(entry->d_name, ".") == 0)
		|| (strcmp(entry->d_name, "..") == 0))
		return 0;
	else
		return 1;
}

int is_file(struct stat statbuf)
{
	if (S_ISREG(statbuf.st_mode) == 0)
		return 0;
	return 1;
}

int is_dir(struct stat statbuf)
{

	if (S_ISDIR(statbuf.st_mode) == 0)
		return 0;
	return 1;
}

void remove_substring(char *s, const char *toremove)
{
	s = strstr(s, toremove);
	if (s)
		memmove(s, s+strlen(toremove),
			1+strlen(s+strlen(toremove)));
}
