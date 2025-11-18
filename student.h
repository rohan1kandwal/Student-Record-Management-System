#ifndef STUDENT_H
#define STUDENT_H

#include <stdbool.h>

#define NAME_LEN 100
#define NUM_SUBJECTS 5

/* Structure that holds a student's record */
typedef struct Student {
    char name[NAME_LEN];
    int roll;
    double marks[NUM_SUBJECTS]; /* Order: Mathematics, Physics, Chemistry, ComputerScience, English */
    double attendance; /* percentage 0 - 100 */
    struct Student *next; /* for linked list in database */
} Student;

/* Create and return a new student (dynamically allocated).
   Caller responsible for freeing via database deletion functions. */
Student *create_student(const char *name, int roll, double marks[], double attendance);

/* Print a single student record in a friendly format */
void print_student(const Student *s);

/* Compute average marks for a student */
double student_average(const Student *s);

/* Count how many subjects have marks below threshold */
int student_count_below(const Student *s, double threshold);

/* Validate marks (0-100) and attendance (0-100) */
bool validate_marks_and_attendance(const double marks[], double attendance);

#endif /* STUDENT_H */
