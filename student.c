#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "student.h"

/* Create a new student struct and copy data into it */
Student *create_student(const char *name, int roll, double marks[], double attendance) {
    Student *s = (Student *) malloc(sizeof(Student));
    if (!s) return NULL;
    strncpy(s->name, name, NAME_LEN-1);
    s->name[NAME_LEN-1] = '\0';
    s->roll = roll;
    for (int i = 0; i < NUM_SUBJECTS; ++i) s->marks[i] = marks[i];
    s->attendance = attendance;
    s->next = NULL;
    return s;
}

/* Print the student record in readable format */
void print_student(const Student *s) {
    if (!s) return;
    printf("--------------------------------------------------\n");
    printf("Name       : %s\n", s->name);
    printf("Roll No.   : %d\n", s->roll);
    printf("Attendance : %.2lf%%\n", s->attendance);
    const char *subjects[NUM_SUBJECTS] = {"Mathematics", "Physics", "Chemistry", "ComputerScience", "English"};
    for (int i = 0; i < NUM_SUBJECTS; ++i) {
        printf("%-14s: %.2lf\n", subjects[i], s->marks[i]);
    }
    printf("Average    : %.2lf\n", student_average(s));
    printf("--------------------------------------------------\n");
}

/* Compute average marks */
double student_average(const Student *s) {
    if (!s) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < NUM_SUBJECTS; ++i) sum += s->marks[i];
    return sum / NUM_SUBJECTS;
}

/* Count subjects below a threshold (e.g., failing subjects) */
int student_count_below(const Student *s, double threshold) {
    if (!s) return 0;
    int cnt = 0;
    for (int i = 0; i < NUM_SUBJECTS; ++i) if (s->marks[i] < threshold) ++cnt;
    return cnt;
}

/* Validate marks and attendance are within 0-100 inclusive */
bool validate_marks_and_attendance(const double marks[], double attendance) {
    if (attendance < 0.0 || attendance > 100.0) return false;
    for (int i = 0; i < NUM_SUBJECTS; ++i) {
        if (marks[i] < 0.0 || marks[i] > 100.0) return false;
    }
    return true;
}
