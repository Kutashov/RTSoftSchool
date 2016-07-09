#ifndef FOLDER_TREE_H
#define FOLDER_TREE_H

const int TYPE_REG = 1;
const int TYPE_FOL = 2;

struct file {
	char *name;
	int type;
	char *hash;
	struct file *parent;
	struct file *childs;
	struct file *prev_sib;
	struct file *next_sib;
	int visited;
};

void add_folder(char *name, int log);
void add_folder_to(struct file *parent, struct file *curr, int log);
void add_file(char *name, char *hash, int log);
void add_file_to(struct file *parent, struct file *curr, int log);
void print_tree(void);
void print_folder(struct file *parent);
char *dirname(char *path);
char *basename(char *path);
struct file *get_child(struct file *parent, char *childs_name);
int check_deleted_files(char *folder);
void check_deleted(struct file *parent);
void delete_file(struct file *file);
void delete_folder(char *folder);

#endif
