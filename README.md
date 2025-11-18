# Student Record Management System (C)

Summary
-------
This repository is a small C-based Student Record Management System with a simple AI analysis integration. It provides a CLI menu to add, update, delete, search, list and analyze student records. The AI analysis calls the OpenAI Chat Completions API (if configured) and falls back to a local heuristic.

Files
-----
- [main.c](main.c) — CLI and program flow.
- [student.h](student.h) / [student.c](student.c) — `Student` data structure and helpers (creation, printing, validation, averages).
- [database.h](database.h) / [database.c](database.c) — in-memory linked-list DB and binary persistence.
- [openai_ai.h](openai_ai.h) / [openai_ai.c](openai_ai.c) — AI wrapper using libcurl + cJSON with local fallback.
- [.vscode/c_cpp_properties.json](.vscode/c_cpp_properties.json) — VSCode IntelliSense config for MSYS2/MinGW.

Key types and functions (with links)
-----------------------------------
Student model and utilities (see [student.h](student.h) / [student.c](student.c)):

- [`Student`](student.h) — struct representing a student record (name, roll, marks[], attendance, next).
- [`create_student`](student.h) — allocate and initialize a `Student`.
- [`print_student`](student.h) — pretty-print a student record to stdout.
- [`student_average`](student.h) — return average of the marks.
- [`student_count_below`](student.h) — count how many subject marks are below a threshold.
- [`validate_marks_and_attendance`](student.h) — ensure marks and attendance are in 0–100 range.

Database (in-memory linked list) API (see [database.h](database.h) / [database.c](database.c)):

- [`db_load`](database.h) — load DB from a binary file. If file absent, creates empty DB in memory.
- [`db_save`](database.h) — save DB to a binary file.
- [`db_add_student`](database.h) — add a `Student *` to DB (takes ownership).
- [`db_delete_by_roll`](database.h) — delete by roll number.
- [`db_update_student`](database.h) — update by roll (name, marks, attendance).
- [`db_search_by_roll`](database.h) — find student by roll (returns pointer inside DB).
- [`db_search_by_name`](database.h) — case-sensitive search by exact name.
- [`db_print_all`](database.h) — print all students sorted ascending by roll.
- [`db_free_all`](database.h) — free all in-memory nodes.

AI analysis (see [openai_ai.h](openai_ai.h) / [openai_ai.c](openai_ai.c)):

- [`RiskLevel`](openai_ai.h) — enum { RISK_LOW, RISK_MEDIUM, RISK_HIGH }.
- [`ai_predict_risk`](openai_ai.h) — returns `RiskLevel` using OpenAI or fallback heuristic.
- [`ai_suggest_career`](openai_ai.h) — returns a pointer to an internal static buffer with career suggestion.
- [`ai_explain`](openai_ai.h) — fills a buffer with a short explanation (calls OpenAI or fallback).

CLI and flow (see [main.c](main.c)):

- Main menu and UI helper functions:
  - `read_line`, `read_double_prompt`, `read_int_prompt` — safe user input helpers.
  - `ui_add_student` — interactive add (validates via [`validate_marks_and_attendance`](student.h)).
  - `ui_update_student` — interactive update (can keep existing fields).
  - `ui_delete_student` — interactive delete by roll.
  - `ui_search_student` — search by roll or exact name.
  - `ui_list_all` — list all students (calls [`db_print_all`](database.h)).
  - `ui_ai_module` — AI analysis for a student (calls [`ai_predict_risk`](openai_ai.h), [`ai_suggest_career`](openai_ai.h), [`ai_explain`](openai_ai.h)).

Design notes / important behaviors
---------------------------------
1. Data structure:
   - The DB is an internal singly linked list (`static Student *db_head` in [database.c](database.c)).
   - New students are inserted at the head by [`db_add_student`](database.h).
   - The list nodes are dynamically allocated (`malloc`) in [`create_student`](student.c) and the DB owns the memory after add; deletion and [`db_free_all`](database.h) free them.

2. Persistence format and portability:
   - [`db_save`](database.c) writes `sizeof(Student)` raw structs to a binary file.
   - [`db_load`](database.c) reads `Student` structs back, copies each into a newly `malloc`'d node and prepends to the list.
   - Caveats:
     - This binary format is not portable between architectures or compiler ABIs (padding, endianness, pointer values). For portability, prefer a text/JSON serialization of fields only.
     - The code writes the entire struct (including the `next` pointer) but when loading it resets `node->next = db_head`, so stored pointer values are ignored on load. Still, saving raw structs is fragile.

3. Sorting output:
   - [`db_print_all`](database.c) collects pointers into an array, uses `qsort` with `cmp_ptr_roll_asc` and prints sorted by roll.

4. Validation and safety:
   - [`validate_marks_and_attendance`](student.c) ensures marks and attendance are in [0,100].
   - User input is read with `fgets` wrappers and converted with `sscanf` / `atof` with fallbacks.

5. AI integration:
   - The AI module in [openai_ai.c](openai_ai.c):
     - Uses libcurl and cJSON to call the OpenAI Chat Completions endpoint (`OPENAI_URL`) and model (`MODEL_NAME` = "gpt-4o-mini").
     - Builds a system prompt + few-shot examples and instructs the assistant to return ONLY a single JSON object: {"risk": "...", "career": "...", "explanation": "..."}.
     - If `OPENAI_API_KEY` is not set or the call fails, [`ai_predict_risk`](openai_ai.h) falls back to a simple heuristic:
       - avg < 45 or attendance < 50 => HIGH
       - else if avg < 60 or attendance < 65 => MEDIUM
       - else => LOW
     - [`ai_suggest_career`](openai_ai.h) and [`ai_explain`](openai_ai.h) also fall back to "Unknown" / local messages on failure.
     - The implementation expects libcurl and cJSON development libraries to be available.

6. Error handling:
   - Many functions return `bool` for success/failure (e.g., [`db_save`](database.h), [`db_load`](database.h), [`db_add_student`](database.h)).
   - `db_load` returns true if the file did not exist (it initializes an empty DB).
   - Memory allocation failures are detected and reported (functions return false where appropriate).

Build & run (example)
---------------------
Dependencies:
- gcc (MinGW/MSYS2 on Windows suggested by the VSCode config).
- libcurl (dev headers and libs).
- cJSON (dev headers and libs).

Example build command (MSYS2/MinGW or Linux):
```sh
gcc -o student_app [main.c](http://_vscodecontentref_/0) [student.c](http://_vscodecontentref_/1) [database.c](http://_vscodecontentref_/2) [openai_ai.c](http://_vscodecontentref_/3) -lcurl -lcjson