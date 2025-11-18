#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "student.h"
#include "database.h"
#include "openai_ai.h"

#define DB_FILENAME "students.dat"

/* Read a line from stdin and trim newline */
static void read_line(char *buf, int size) {
    if (!fgets(buf, size, stdin)) {
        buf[0] = '\0';
        return;
    }
    size_t len = strlen(buf);
    if (len && buf[len-1] == '\n') buf[len-1] = '\0';
}

/* Read double safely with prompt */
static double read_double_prompt(const char *prompt) {
    char buf[128];
    double val;
    while (1) {
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        if (sscanf(buf, "%lf", &val) == 1) return val;
        printf("Invalid number, try again.\n");
    }
}

/* Read int safely */
static int read_int_prompt(const char *prompt) {
    char buf[128];
    int val;
    while (1) {
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        if (sscanf(buf, "%d", &val) == 1) return val;
        printf("Invalid integer, try again.\n");
    }
}

/* Add new student interactive */
static void ui_add_student(void) {
    char name[NAME_LEN];
    int roll = read_int_prompt("Enter roll number: ");
    printf("Enter name: ");
    read_line(name, sizeof(name));
    double marks[NUM_SUBJECTS];
    const char *subjects[NUM_SUBJECTS] = {"Mathematics", "Physics", "Chemistry", "ComputerScience", "English"};
    for (int i = 0; i < NUM_SUBJECTS; ++i) {
        char prompt[120];
        snprintf(prompt, sizeof(prompt), "Enter marks for %s (0-100): ", subjects[i]);
        marks[i] = read_double_prompt(prompt);
    }
    double attendance = read_double_prompt("Enter attendance percentage (0-100): ");

    if (!validate_marks_and_attendance(marks, attendance)) {
        printf("Invalid marks/attendance values. Aborting add.\n");
        return;
    }

    Student *s = create_student(name, roll, marks, attendance);
    if (!s) {
        printf("Memory allocation failed.\n");
        return;
    }
    if (!db_add_student(s)) {
        printf("Failed to add student: maybe roll number already exists.\n");
        free(s);
    } else {
        if (!db_save(DB_FILENAME)) {
            printf("Warning: failed to save database to file.\n");
        }
        printf("Student added successfully.\n");
    }
}

/* Update existing */
static void ui_update_student(void) {
    int roll = read_int_prompt("Enter roll number to update: ");
    Student *s = db_search_by_roll(roll);
    if (!s) {
        printf("Student with roll %d not found.\n", roll);
        return;
    }
    printf("Current record:\n");
    print_student(s);
    char name[NAME_LEN];
    printf("Enter new name (leave blank to keep current): ");
    read_line(name, sizeof(name));
    double marks[NUM_SUBJECTS];
    const char *subjects[NUM_SUBJECTS] = {"Mathematics", "Physics", "Chemistry", "ComputerScience", "English"};
    for (int i = 0; i < NUM_SUBJECTS; ++i) {
        char buf[128], prompt[128];
        snprintf(prompt, sizeof(prompt), "Enter new marks for %s (or blank to keep %.2lf): ", subjects[i], s->marks[i]);
        printf("%s", prompt);
        read_line(buf, sizeof(buf));
        if (strlen(buf) == 0) marks[i] = s->marks[i];
        else {
            if (sscanf(buf, "%lf", &marks[i]) != 1) {
                printf("Invalid input, keeping previous value %.2lf\n", s->marks[i]);
                marks[i] = s->marks[i];
            }
        }
    }
    char buf[128];
    printf("Enter new attendance (or blank to keep %.2lf): ", s->attendance);
    read_line(buf, sizeof(buf));
    double attendance;
    if (strlen(buf) == 0) attendance = s->attendance;
    else attendance = atof(buf);

    if (!validate_marks_and_attendance(marks, attendance)) {
        printf("Invalid marks/attendance values. Aborting update.\n");
        return;
    }
    const char *newnameptr = (strlen(name) == 0) ? NULL : name;
    if (db_update_student(roll, newnameptr, marks, attendance)) {
        if (!db_save(DB_FILENAME)) printf("Warning: failed to save database.\n");
        printf("Student updated successfully.\n");
    } else {
        printf("Failed to update student.\n");
    }
}

/* Delete student */
static void ui_delete_student(void) {
    int roll = read_int_prompt("Enter roll number to delete: ");
    if (db_delete_by_roll(roll)) {
        if (!db_save(DB_FILENAME)) printf("Warning: failed to save database.\n");
        printf("Student deleted.\n");
    } else {
        printf("Student with roll %d not found.\n", roll);
    }
}

/* Search and display */
static void ui_search_student(void) {
    printf("Search by: 1) Roll  2) Name\nChoose option: ");
    int opt = read_int_prompt("");
    if (opt == 1) {
        int roll = read_int_prompt("Enter roll: ");
        Student *s = db_search_by_roll(roll);
        if (s) print_student(s);
        else printf("Not found.\n");
    } else {
        char name[NAME_LEN];
        printf("Enter exact name: ");
        read_line(name, sizeof(name));
        Student *s = db_search_by_name(name);
        if (s) print_student(s);
        else printf("Not found.\n");
    }
}

/* List all */
static void ui_list_all(void) {
    db_print_all();
}

/* AI actions: predict risk and suggest career for a student */
static void ui_ai_module(void) {
    int roll = read_int_prompt("Enter roll number for AI analysis: ");
    Student *s = db_search_by_roll(roll);
    if (!s) {
        printf("Student not found.\n");
        return;
    }
    char explanation[256];
    ai_explain(s, explanation, sizeof(explanation));
    RiskLevel r = ai_predict_risk(s);
    const char *risk_text = (r == RISK_HIGH) ? "HIGH" : (r == RISK_MEDIUM ? "MEDIUM" : "LOW");
    const char *career = ai_suggest_career(s);
    printf("AI Analysis for %s (Roll %d):\n", s->name, s->roll);
    printf("Risk Level      : %s\n", risk_text);
    printf("Suggested field : %s\n", career);
    printf("Explanation     : %s\n", explanation);
}

/* Print menu */
static void print_menu(void) {
    printf("\n=== Student Record Management System ===\n");
    printf("1. Add Student\n");
    printf("2. Update Student\n");
    printf("3. Delete Student\n");
    printf("4. Search Student\n");
    printf("5. List all Students\n");
    printf("6. AI Analysis for Student (Risk + Career Suggestion)\n");
    printf("7. Exit\n");
    printf("----------------------------------------\n");
}

/* Main loop */
int main(void) {
    if (!db_load(DB_FILENAME)) {
        printf("Warning: failed to load database file. Starting with empty DB.\n");
    }

    while (1) {
        print_menu();
        int choice = read_int_prompt("Enter choice: ");
        switch (choice) {
            case 1: ui_add_student(); break;
            case 2: ui_update_student(); break;
            case 3: ui_delete_student(); break;
            case 4: ui_search_student(); break;
            case 5: ui_list_all(); break;
            case 6: ui_ai_module(); break;
            case 7:
                if (!db_save(DB_FILENAME)) {
                    printf("Warning: failed to save database on exit.\n");
                }
                db_free_all();
                printf("Exiting. Goodbye!\n");
                return 0;
            default:
                printf("Invalid choice, try again.\n");
        }
    }
    return 0;
}
