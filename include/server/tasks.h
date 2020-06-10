#ifndef SO_TASKS_H
#define SO_TASKS_H

typedef struct task {
    pid_t pid;
    long long taskID;
    char *task;
} Task;

typedef struct tasks {
    Task *tasks;
    size_t size;
    size_t capacity;
} Tasks;

Tasks tasks_create();
void tasks_add(Tasks *this, Task task);
void tasks_list(Tasks const *this, int fd);
bool kill_task(Tasks *this, long long tid);
void free_tasks(Tasks *this);

#endif //SO_TASKS_H
