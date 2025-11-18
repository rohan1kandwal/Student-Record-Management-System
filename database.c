#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"
#include "student.h"   // ensure print_student / student_average prototypes are available

static Student *db_head = NULL; /* internal linked list head */

/* comparator for qsort: ascending by roll using pointers */
static int cmp_ptr_roll_asc(const void *a, const void *b) {
    const Student *const *pa = (const Student *const *)a;
    const Student *const *pb = (const Student *const *)b;
    if ((*pa)->roll < (*pb)->roll) return -1;
    if ((*pa)->roll > (*pb)->roll) return 1;
    return 0;
}

/* Internal helper to find duplicate roll */
static bool roll_exists(int roll) {
    Student *cur = db_head;
    while (cur) {
        if (cur->roll == roll) return true;
        cur = cur->next;
    }
    return false;
}

/* Load database from binary file */
bool db_load(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        /* No file yet, that's fine - start with empty DB */
        db_head = NULL;
        return true;
    }

    /* Clear current in-memory list first */
    db_free_all();

    while (1) {
        Student temp;
        size_t read = fread(&temp, sizeof(Student), 1, f);
        if (read != 1) break;
        /* allocate new node and copy */
        Student *node = (Student *)malloc(sizeof(Student));
        if (!node) {
            fclose(f);
            return false;
        }
        memcpy(node, &temp, sizeof(Student));
        node->next = db_head;
        db_head = node;
    }
    fclose(f);
    return true;
}

/* Save DB into binary file */
bool db_save(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return false;
    Student *cur = db_head;
    while (cur) {
        if (fwrite(cur, sizeof(Student), 1, f) != 1) {
            fclose(f);
            return false;
        }
        cur = cur->next;
    }
    fclose(f);
    return true;
}

/* Add student if roll not present */
bool db_add_student(Student *s) {
    if (!s) return false;
    if (roll_exists(s->roll)) return false;
    s->next = db_head;
    db_head = s;
    return true;
}

/* Delete by roll */
bool db_delete_by_roll(int roll) {
    Student *cur = db_head;
    Student *prev = NULL;
    while (cur) {
        if (cur->roll == roll) {
            if (prev) prev->next = cur->next;
            else db_head = cur->next;
            free(cur);
            return true;
        }
        prev = cur;
        cur = cur->next;
    }
    return false;
}

/* Update student matched by roll */
bool db_update_student(int roll, const char *new_name, double new_marks[], double new_attendance) {
    Student *s = db_search_by_roll(roll);
    if (!s) return false;
    if (new_name) {
        strncpy(s->name, new_name, NAME_LEN-1);
        s->name[NAME_LEN-1] = '\0';
    }
    if (new_marks) {
        for (int i = 0; i < NUM_SUBJECTS; ++i) s->marks[i] = new_marks[i];
    }
    s->attendance = new_attendance;
    return true;
}

/* Search by roll */
Student *db_search_by_roll(int roll) {
    Student *cur = db_head;
    while (cur) {
        if (cur->roll == roll) return cur;
        cur = cur->next;
    }
    return NULL;
}

/* Search by name (case-sensitive first match) */
Student *db_search_by_name(const char *name) {
    Student *cur = db_head;
    while (cur) {
        if (strcmp(cur->name, name) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

/* Print all (sorted ascending by roll) */
void db_print_all(void) {
    Student *cur = db_head;
    if (!cur) {
        printf("No student records available.\n");
        return;
    }

    /* Count nodes */
    int count = 0;
    for (Student *t = cur; t; t = t->next) ++count;

    /* Create array of pointers */
    Student **arr = (Student **)malloc(sizeof(Student *) * count);
    if (!arr) {
        /* Fallback: print unsorted list if allocation fails */
        for (Student *t = cur; t; t = t->next) print_student(t);
        return;
    }

    int i = 0;
    for (Student *t = cur; t; t = t->next) arr[i++] = t;

    qsort(arr, (size_t)count, sizeof(Student *), cmp_ptr_roll_asc);

    /* Print sorted */
    for (i = 0; i < count; ++i) print_student(arr[i]);

    free(arr);
}

/* Free all nodes */
void db_free_all(void) {
    Student *cur = db_head;
    while (cur) {
        Student *n = cur->next;
        free(cur);
        cur = n;
    }
    db_head = NULL;
}
