#include "folder_tree.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static struct file *header;

void add_folder(char *name, int log)
{

	/*printf("Add folder %s\n", name);*/

	struct file *curr = malloc(sizeof(struct file));

	curr->name = strdup(name);
	curr->type = TYPE_FOL;
	curr->parent = NULL;
	curr->childs = NULL;
	curr->prev_sib = NULL;
	curr->next_sib = NULL;
	curr->visited = 1;

	add_folder_to(header, curr, log);
}

void add_folder_to(struct file *head_child,
		struct file *curr, int log)
{

	if (head_child == NULL) {
		header = curr;
		if (log)
			printf("NOTE! %s added as root\n", curr->name);
		return;
	}

	struct file *curr_parent = head_child;

	while (curr_parent != NULL
		&& strstr(curr->name, curr_parent->name) == NULL
		&& strcmp(curr->name, curr_parent->name) != 0)
		curr_parent = curr_parent->next_sib;

	if (curr_parent == NULL) {

		if (head_child != NULL) {
			head_child->prev_sib = curr;
			curr->next_sib = head_child;
			if (head_child->parent != NULL)
				head_child->parent->childs = curr;

			curr->parent = head_child->parent;
		}
		if (head_child == header)
			header = curr;

		if (log)
			if (curr->parent != NULL)
				printf("NOTE! %s added to %s\n",
					curr->name, curr->parent->name);
			else
				printf("NOTE! %s added to root\n", curr->name);
	} else if (strcmp(curr->name, curr_parent->name) == 0) {
		curr_parent->visited = 1;
		/*printf("Folder %s already exists\n", curr->name);*/
	} else {
		if (curr_parent->childs == NULL) {
			curr_parent->childs = curr;
			curr->parent = curr_parent;
			if (log)
				if (curr->parent != NULL)
					printf("NOTE! %s added to %s\n",
						curr->name, curr->parent->name);
				else
					printf("NOTE! %s added to root\n",
						curr->name);
		} else
			add_folder_to(curr_parent->childs, curr, log);
	}

}

void add_file(char *name, char *hash, int log)
{

	/*printf("Add file %s - %s\n", name, hash);*/

	struct file *curr = malloc(sizeof(struct file));

	curr->name = strdup(name);
	curr->type = TYPE_REG;
	curr->hash = strdup(hash);
	curr->parent = NULL;
	curr->childs = NULL;
	curr->prev_sib = NULL;
	curr->next_sib = NULL;
	curr->visited = 1;

	add_file_to(header, curr, log);
}

void add_file_to(struct file *parent, struct file *curr, int log)
{

	char *dir = dirname(curr->name);
	char *name = basename(curr->name);

	if (parent == NULL) {
		if (log) {
			char *dir = dirname(curr->name);

			printf("NOTE! %s NOT added: %s\n", dir, name);
			free(dir);
			free(name);
		}
		delete_file(curr);
		return;
	}


	struct file *curr_parent = parent;

	while (curr_parent != NULL &&
		strcmp(dir, curr_parent->name) != 0 &&
		strstr(curr->name, curr_parent->name) == NULL)
		curr_parent = curr_parent->next_sib;


	if (curr_parent == NULL) {

		if (parent->childs != NULL) {
			parent->childs->prev_sib = curr;
			curr->next_sib = parent->childs;
		}

		parent->childs = curr;
		curr->parent = parent;
		if (log)
			printf("NOTE! %s added: %s\n", dir, name);

		return;
	} else if (strcmp(dir, curr_parent->name) == 0) {

		struct file *exists = get_child(curr_parent, curr->name);

		if (exists != NULL) {
			exists->visited = 1;
			if (strcmp(curr->hash, exists->hash) != 0) {
				printf("NOTE! %s changed: %s\n", dir, name);
				/*printf("old-%s- new-%s-\n",
				*	exists->hash, curr->hash);
				*/
				free(exists->hash);
				exists->hash = curr->hash;
			} else {
				/*printf("File %s exists\n", curr->name);*/
			}
		} else {

			curr->next_sib = curr_parent->childs;
			if (curr_parent->childs != NULL)
				curr_parent->childs->prev_sib = curr;


			curr_parent->childs = curr;
			curr->parent = curr_parent;
			if (log)
				printf("NOTE! %s added: %s\n", dir, name);
		}

	} else
		add_file_to(curr_parent->childs, curr, log);

	free(dir);
	free(name);
}

struct file *get_child(struct file *parent, char *childs_name)
{

	if (parent == NULL)
		return NULL;

	struct file *child = parent->childs;

	while (child != NULL &&
		strcmp(child->name, childs_name) != 0) {
		child = child->next_sib;
	}
	return child;
}

char *dirname(char *path)
{
	char *last = strrchr(path, '/');

	return strndup(path, last-path);
}

char *basename(char *path)
{
	char *s = strrchr(path, '/');

	if (!s)
		return strdup(path);
	else
		return strdup(s + 1);
}

void print_tree(void)
{
	printf("Tree -------------------\n");

	print_folder(header);
}

void print_folder(struct file *parent)
{
	printf("down\n");
	struct file *curr = parent;

	while (curr != NULL) {
		printf("print %s\n", curr->name);
		if (curr->type == TYPE_FOL)
			print_folder(curr->childs);

		curr = curr->next_sib;
	}
	printf("up\n");
}

int check_deleted_files(char *folder)
{

	struct file *curr = header;

	while (curr != NULL &&
		strcmp(curr->name, folder) != 0)
		curr = curr->next_sib;
	if (curr == NULL) {
		printf("Couldn't found folder for check\n");
		return;
	}

	if (curr->visited) {
		curr->visited = 0;
		check_deleted(curr->childs);
	} else {
		if (header == curr)
			header = curr->next_sib;
		delete_file(curr);
		return 1;
	}
	return 0;
}

void check_deleted(struct file *parent)
{

	struct file *curr = parent;
	struct file *next;

	while (curr != NULL) {
		next = curr->next_sib;
		if (curr->visited) {
			curr->visited = 0;

			if (curr->type == TYPE_FOL)
				check_deleted(curr->childs);

		} else {

			printf("NOTE! %s deleted\n", curr->name);
			delete_file(curr);
		}

		curr = next;
	}
}

void delete_file(struct file *curr)
{

	if (curr == NULL)
		return;
	free(curr->name);
	if (curr->parent != NULL && curr->parent->childs == curr)
		curr->parent->childs = curr->next_sib;
	if (curr->prev_sib != NULL)
		curr->prev_sib->next_sib = curr->next_sib;
	if (curr->next_sib != NULL)
		curr->next_sib->prev_sib = curr->prev_sib;
	if (curr->type == TYPE_FOL) {
		struct file *child = curr->childs;
		struct file *next;

		while (child != NULL) {
			next = child->next_sib;
			delete_file(child);
			child = next;
		}
	} else {
		free(curr->hash);
	}
	free(curr);
}

void delete_folder(char *folder)
{

	struct file *curr = header;

	while (curr != NULL &&
		strcmp(curr->name, folder) != 0)
		curr = curr->next_sib;
	if (curr == NULL) {
		printf("Couldn't found folder for deleting\n");
		return;
	}

	if (header == curr)
		header = curr->next_sib;

	delete_file(curr);
}
