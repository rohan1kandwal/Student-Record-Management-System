#ifndef DATABASE_H
#define DATABASE_H

#include "student.h"
#include <stdbool.h>

/* Head pointer for student linked list is managed inside database.c */

/* Load database from file (returns true on success).
   If file does not exist yet, function will create empty DB in memory. */
bool db_load(const char *filename);

/* Save current database to file (binary). */
bool db_save(const char *filename);

/* Add student to DB. Returns true on success, false if roll duplicate or memory error. */
bool db_add_student(Student *s);

/* Delete student by roll. Returns true if deleted. */
bool db_delete_by_roll(int roll);

/* Update student by roll. Returns true on success. */
bool db_update_student(int roll, const char *new_name, double new_marks[], double new_attendance);

/* Search by roll, returns pointer to Student (do not free). NULL if not found. */
Student *db_search_by_roll(int roll);

/* Search by name (first match). Returns pointer or NULL. */
Student *db_search_by_name(const char *name);

/* Print all students */
void db_print_all(void);

/* Free all in-memory student list (does not save). */
void db_free_all(void);

#endif /* DATABASE_H */
