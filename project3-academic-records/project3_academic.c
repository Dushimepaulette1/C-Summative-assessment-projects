/*
 * ============================================================
 *  PROJECT 3: Course Performance & Academic Records Analyzer
 *  Language : C
 *  Author   : [Your Name]
 * ============================================================
 *
 *  HOW IT WORKS:
 *  - Stores student records using C structures
 *  - Dynamically allocates memory (malloc/realloc) so we can
 *    handle ANY number of students — not a fixed size
 *  - Saves and loads records from a binary file
 *  - Lets you Add, View, Update, Delete student records (CRUD)
 *  - Search by ID or Name
 *  - Sort by GPA, Name, or ID
 *  - Generate performance reports (average, highest, lowest GPA)
 *
 *  HOW TO COMPILE AND RUN:
 *  gcc -o academic_analyzer project3_academic.c
 *  ./academic_analyzer
 * ============================================================
 */

#include <stdio.h>    // printf, scanf, fopen, fclose, etc.
#include <stdlib.h>   // malloc, realloc, free, exit
#include <string.h>   // strcpy, strcmp, strlen, strcasecmp
#include <ctype.h>    // tolower (for case-insensitive search)

/* ── Constants ─────────────────────────────────────────────── */
#define MAX_NAME_LEN   100   // Max characters in a name
#define MAX_COURSE_LEN 100   // Max characters in a course name
#define MAX_SUBJECTS   10    // Max number of subjects per student
#define DATA_FILE      "students.dat"  // Binary file for saving data

/* ── Structure: Student ─────────────────────────────────────
 * A structure bundles related data together.
 * Each student has all this info packed into one "object".
 */
typedef struct {
    int    id;                          // Unique student ID
    char   name[MAX_NAME_LEN];          // Full name
    char   course[MAX_COURSE_LEN];      // Enrolled course/program
    int    age;                         // Age in years
    float  grades[MAX_SUBJECTS];        // Array of subject grades
    int    num_subjects;                // How many subjects they have
    float  gpa;                         // Calculated GPA (average of grades)
} Student;

/* ── Global Variables ────────────────────────────────────────
 * These are used throughout all functions.
 * Using a pointer lets us resize the array dynamically.
 */
Student* students    = NULL;  // Pointer to our dynamic array of students
int      total_count = 0;     // How many students are currently stored

/* ── Function Prototypes (Declarations) ─────────────────────
 * We declare all functions here so C knows they exist
 * before we define them below.
 */
void    print_separator();
void    print_header(const char* title);
float   calculate_gpa(float grades[], int count);
void    add_student();
void    view_all_students();
void    view_student_by_index(int index);
void    update_student();
void    delete_student();
int     find_student_by_id(int id);
void    search_students();
void    sort_by_gpa();
void    sort_by_name();
void    sort_by_id();
void    generate_report();
void    save_to_file();
void    load_from_file();
void    free_memory();
int     id_exists(int id);
void    show_menu();

/* ============================================================
 *  FUNCTION: main
 *  Purpose : Entry point — shows the menu and handles choices
 * ============================================================ */
int main() {
    printf("\n╔══════════════════════════════════════╗\n");
    printf("║  Academic Records Analyzer (C)       ║\n");
    printf("╚══════════════════════════════════════╝\n");

    // Try to load any previously saved student data
    load_from_file();

    int choice;

    // Main program loop
    while (1) {
        show_menu();

        if (scanf("%d", &choice) != 1) {
            // If user typed something that's not a number
            while (getchar() != '\n'); // Clear the input buffer
            printf("[ERROR] Please enter a valid number.\n");
            continue;
        }
        while (getchar() != '\n'); // Clear leftover newline from input

        switch (choice) {
            case 1:  add_student();      break;
            case 2:  view_all_students(); break;
            case 3:  update_student();   break;
            case 4:  delete_student();   break;
            case 5:  search_students();  break;
            case 6:  sort_by_gpa();      break;
            case 7:  sort_by_name();     break;
            case 8:  sort_by_id();       break;
            case 9:  generate_report();  break;
            case 10: save_to_file();     break;
            case 11: load_from_file();   break;
            case 12:
                save_to_file();   // Auto-save before exiting
                free_memory();    // Release all allocated memory
                printf("Goodbye! All data saved.\n");
                return 0;
            default:
                printf("[ERROR] Invalid choice. Please enter 1-12.\n");
        }
    }
}

/* ============================================================
 *  FUNCTION: show_menu
 *  Purpose : Print the main interactive menu
 * ============================================================ */
void show_menu() {
    print_header("MAIN MENU");
    printf("  1.  Add new student\n");
    printf("  2.  View all students\n");
    printf("  3.  Update student record\n");
    printf("  4.  Delete student record\n");
    printf("  5.  Search (by ID or Name)\n");
    printf("  6.  Sort by GPA\n");
    printf("  7.  Sort by Name\n");
    printf("  8.  Sort by ID\n");
    printf("  9.  Generate performance report\n");
    printf("  10. Save records to file\n");
    printf("  11. Load records from file\n");
    printf("  12. Exit\n");
    print_separator();
    printf("Enter choice: ");
}

/* ============================================================
 *  FUNCTION: calculate_gpa
 *  Purpose : Average all grades to produce a GPA score
 *  Params  : array of grades, number of grades
 *  Returns : the average (float)
 * ============================================================ */
float calculate_gpa(float grades[], int count) {
    if (count == 0) return 0.0f;  // Avoid divide-by-zero

    float total = 0.0f;
    for (int i = 0; i < count; i++) {
        total += grades[i];
    }
    return total / count;  // Average
}

/* ============================================================
 *  FUNCTION: add_student
 *  Purpose : Collect data from user and add a new student
 *  Key concept: realloc() expands the array by 1 each time
 * ============================================================ */
void add_student() {
    print_header("ADD NEW STUDENT");

    Student new_student;  // Temporary student to fill in

    /* ── Get Student ID ── */
    printf("Enter Student ID (number): ");
    if (scanf("%d", &new_student.id) != 1) {
        while (getchar() != '\n');
        printf("[ERROR] Invalid ID. Must be a number.\n");
        return;
    }
    while (getchar() != '\n');

    // Check if this ID already exists (no duplicates allowed)
    if (id_exists(new_student.id)) {
        printf("[ERROR] Student ID %d already exists.\n", new_student.id);
        return;
    }

    /* ── Get Name ── */
    printf("Enter Full Name: ");
    fgets(new_student.name, MAX_NAME_LEN, stdin);
    // fgets includes a newline — remove it
    new_student.name[strcspn(new_student.name, "\n")] = '\0';

    /* ── Get Course ── */
    printf("Enter Course/Program: ");
    fgets(new_student.course, MAX_COURSE_LEN, stdin);
    new_student.course[strcspn(new_student.course, "\n")] = '\0';

    /* ── Get Age ── */
    printf("Enter Age: ");
    if (scanf("%d", &new_student.age) != 1 || new_student.age < 1 || new_student.age > 120) {
        while (getchar() != '\n');
        printf("[ERROR] Invalid age. Must be between 1 and 120.\n");
        return;
    }
    while (getchar() != '\n');

    /* ── Get Number of Subjects ── */
    printf("How many subjects (1-%d): ", MAX_SUBJECTS);
    if (scanf("%d", &new_student.num_subjects) != 1
        || new_student.num_subjects < 1
        || new_student.num_subjects > MAX_SUBJECTS) {
        while (getchar() != '\n');
        printf("[ERROR] Number of subjects must be 1 to %d.\n", MAX_SUBJECTS);
        return;
    }
    while (getchar() != '\n');

    /* ── Get Grades ── */
    for (int i = 0; i < new_student.num_subjects; i++) {
        printf("  Grade for subject %d (0-100): ", i + 1);
        if (scanf("%f", &new_student.grades[i]) != 1
            || new_student.grades[i] < 0
            || new_student.grades[i] > 100) {
            while (getchar() != '\n');
            printf("[ERROR] Grade must be between 0 and 100.\n");
            return;
        }
        while (getchar() != '\n');
    }

    // Calculate GPA from the entered grades
    new_student.gpa = calculate_gpa(new_student.grades, new_student.num_subjects);

    /* ── Expand the Dynamic Array ──
     * realloc grows our students array by 1 slot.
     * If students is NULL (first time), realloc acts like malloc.
     */
    Student* temp = realloc(students, (total_count + 1) * sizeof(Student));
    if (temp == NULL) {
        printf("[ERROR] Memory allocation failed! Could not add student.\n");
        return;
    }
    students = temp;  // Update global pointer to new array

    // Copy the new student into the last slot of the array
    students[total_count] = new_student;
    total_count++;

    printf("\n[SUCCESS] Student '%s' added. GPA: %.2f\n",
           new_student.name, new_student.gpa);
}

/* ============================================================
 *  FUNCTION: view_student_by_index
 *  Purpose : Print all details of one student
 * ============================================================ */
void view_student_by_index(int i) {
    printf("  ID      : %d\n",   students[i].id);
    printf("  Name    : %s\n",   students[i].name);
    printf("  Course  : %s\n",   students[i].course);
    printf("  Age     : %d\n",   students[i].age);
    printf("  Grades  : ");
    for (int j = 0; j < students[i].num_subjects; j++) {
        printf("%.1f  ", students[i].grades[j]);
    }
    printf("\n  GPA     : %.2f\n", students[i].gpa);
    print_separator();
}

/* ============================================================
 *  FUNCTION: view_all_students
 *  Purpose : Loop through and display every student
 * ============================================================ */
void view_all_students() {
    print_header("ALL STUDENT RECORDS");

    if (total_count == 0) {
        printf("No student records found.\n");
        return;
    }

    printf("Total students: %d\n\n", total_count);
    for (int i = 0; i < total_count; i++) {
        printf("Record #%d:\n", i + 1);
        view_student_by_index(i);
    }
}

/* ============================================================
 *  FUNCTION: find_student_by_id
 *  Purpose : Linear search — find the INDEX of a student by ID
 *  Returns : index in the array, or -1 if not found
 * ============================================================ */
int find_student_by_id(int id) {
    for (int i = 0; i < total_count; i++) {
        if (students[i].id == id) {
            return i;  // Found it! Return the array position
        }
    }
    return -1;  // Not found
}

/* ============================================================
 *  FUNCTION: id_exists
 *  Purpose : Check if a student ID is already taken
 *  Returns : 1 (true) if it exists, 0 (false) if not
 * ============================================================ */
int id_exists(int id) {
    return find_student_by_id(id) != -1;
}

/* ============================================================
 *  FUNCTION: update_student
 *  Purpose : Allow user to change an existing student's details
 * ============================================================ */
void update_student() {
    print_header("UPDATE STUDENT RECORD");

    if (total_count == 0) {
        printf("No students to update.\n");
        return;
    }

    int id;
    printf("Enter Student ID to update: ");
    if (scanf("%d", &id) != 1) { while (getchar() != '\n'); return; }
    while (getchar() != '\n');

    int idx = find_student_by_id(id);
    if (idx == -1) {
        printf("[ERROR] No student with ID %d found.\n", id);
        return;
    }

    printf("Updating: %s\n", students[idx].name);
    printf("Press Enter to keep existing value.\n\n");

    /* ── Update Name ── */
    char buffer[MAX_NAME_LEN];
    printf("New Name [%s]: ", students[idx].name);
    fgets(buffer, MAX_NAME_LEN, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    if (strlen(buffer) > 0) {
        strcpy(students[idx].name, buffer);
    }

    /* ── Update Course ── */
    printf("New Course [%s]: ", students[idx].course);
    fgets(buffer, MAX_COURSE_LEN, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    if (strlen(buffer) > 0) {
        strcpy(students[idx].course, buffer);
    }

    /* ── Update Grades ── */
    printf("Re-enter grades? (y/n): ");
    char ans[4];
    fgets(ans, 4, stdin);
    if (ans[0] == 'y' || ans[0] == 'Y') {
        printf("How many subjects (1-%d): ", MAX_SUBJECTS);
        scanf("%d", &students[idx].num_subjects);
        while (getchar() != '\n');
        for (int i = 0; i < students[idx].num_subjects; i++) {
            printf("  Grade for subject %d: ", i + 1);
            scanf("%f", &students[idx].grades[i]);
            while (getchar() != '\n');
        }
        // Recalculate GPA with new grades
        students[idx].gpa = calculate_gpa(students[idx].grades, students[idx].num_subjects);
    }

    printf("\n[SUCCESS] Student record updated. New GPA: %.2f\n", students[idx].gpa);
}

/* ============================================================
 *  FUNCTION: delete_student
 *  Purpose : Remove a student and shift the array to fill the gap
 *  Key concept: We shift elements left to close the gap,
 *               then shrink the array with realloc.
 * ============================================================ */
void delete_student() {
    print_header("DELETE STUDENT RECORD");

    if (total_count == 0) {
        printf("No students to delete.\n");
        return;
    }

    int id;
    printf("Enter Student ID to delete: ");
    if (scanf("%d", &id) != 1) { while (getchar() != '\n'); return; }
    while (getchar() != '\n');

    int idx = find_student_by_id(id);
    if (idx == -1) {
        printf("[ERROR] No student with ID %d found.\n", id);
        return;
    }

    printf("Are you sure you want to delete '%s'? (y/n): ", students[idx].name);
    char ans[4];
    fgets(ans, 4, stdin);
    if (ans[0] != 'y' && ans[0] != 'Y') {
        printf("Deletion cancelled.\n");
        return;
    }

    // Shift every student after idx one position to the left
    for (int i = idx; i < total_count - 1; i++) {
        students[i] = students[i + 1];
    }
    total_count--;

    // Shrink the array by 1 slot using realloc
    if (total_count > 0) {
        Student* temp = realloc(students, total_count * sizeof(Student));
        if (temp != NULL) students = temp;
    } else {
        // No students left — free everything
        free(students);
        students = NULL;
    }

    printf("[SUCCESS] Student deleted. Total records: %d\n", total_count);
}

/* ============================================================
 *  FUNCTION: search_students
 *  Purpose : Find students by ID or by name (partial match)
 * ============================================================ */
void search_students() {
    print_header("SEARCH STUDENTS");
    printf("1. Search by ID\n");
    printf("2. Search by Name\n");
    printf("Choice: ");

    int choice;
    if (scanf("%d", &choice) != 1) { while (getchar() != '\n'); return; }
    while (getchar() != '\n');

    if (choice == 1) {
        /* ── Search by ID ── */
        int id;
        printf("Enter Student ID: ");
        if (scanf("%d", &id) != 1) { while (getchar() != '\n'); return; }
        while (getchar() != '\n');

        int idx = find_student_by_id(id);
        if (idx == -1) {
            printf("No student with ID %d found.\n", id);
        } else {
            printf("\nFound:\n");
            view_student_by_index(idx);
        }
    }
    else if (choice == 2) {
        /* ── Search by Name (partial, case-insensitive) ── */
        char query[MAX_NAME_LEN];
        printf("Enter name (or part of name): ");
        fgets(query, MAX_NAME_LEN, stdin);
        query[strcspn(query, "\n")] = '\0';

        // Convert search query to lowercase for case-insensitive comparison
        for (int i = 0; query[i]; i++) query[i] = tolower(query[i]);

        int found = 0;
        for (int i = 0; i < total_count; i++) {
            char lower_name[MAX_NAME_LEN];
            strcpy(lower_name, students[i].name);
            for (int j = 0; lower_name[j]; j++) lower_name[j] = tolower(lower_name[j]);

            // strstr checks if query appears anywhere inside lower_name
            if (strstr(lower_name, query) != NULL) {
                if (!found) printf("\nSearch results:\n");
                view_student_by_index(i);
                found++;
            }
        }
        if (!found) printf("No students matching '%s' found.\n", query);
        else printf("Found %d result(s).\n", found);
    }
    else {
        printf("[ERROR] Invalid choice.\n");
    }
}

/* ============================================================
 *  FUNCTION: sort_by_gpa
 *  Purpose : Sort students from highest GPA to lowest
 *  Algorithm: Bubble Sort (simple, easy to understand)
 * ============================================================ */
void sort_by_gpa() {
    if (total_count < 2) { printf("Need at least 2 students to sort.\n"); return; }

    // Bubble Sort: repeatedly swap adjacent elements if in wrong order
    for (int i = 0; i < total_count - 1; i++) {
        for (int j = 0; j < total_count - 1 - i; j++) {
            // Sort descending (highest GPA first)
            if (students[j].gpa < students[j + 1].gpa) {
                Student temp       = students[j];
                students[j]        = students[j + 1];
                students[j + 1]    = temp;
            }
        }
    }
    printf("[SUCCESS] Students sorted by GPA (highest first).\n");
    view_all_students();
}

/* ============================================================
 *  FUNCTION: sort_by_name
 *  Purpose : Sort students alphabetically by name
 * ============================================================ */
void sort_by_name() {
    if (total_count < 2) { printf("Need at least 2 students to sort.\n"); return; }

    for (int i = 0; i < total_count - 1; i++) {
        for (int j = 0; j < total_count - 1 - i; j++) {
            // strcmp returns negative if j < j+1 alphabetically
            if (strcmp(students[j].name, students[j + 1].name) > 0) {
                Student temp    = students[j];
                students[j]     = students[j + 1];
                students[j + 1] = temp;
            }
        }
    }
    printf("[SUCCESS] Students sorted by Name (A-Z).\n");
    view_all_students();
}

/* ============================================================
 *  FUNCTION: sort_by_id
 *  Purpose : Sort students by ID (ascending)
 * ============================================================ */
void sort_by_id() {
    if (total_count < 2) { printf("Need at least 2 students to sort.\n"); return; }

    for (int i = 0; i < total_count - 1; i++) {
        for (int j = 0; j < total_count - 1 - i; j++) {
            if (students[j].id > students[j + 1].id) {
                Student temp    = students[j];
                students[j]     = students[j + 1];
                students[j + 1] = temp;
            }
        }
    }
    printf("[SUCCESS] Students sorted by ID (ascending).\n");
    view_all_students();
}

/* ============================================================
 *  FUNCTION: generate_report
 *  Purpose : Calculate and display class statistics
 * ============================================================ */
void generate_report() {
    print_header("PERFORMANCE REPORT");

    if (total_count == 0) {
        printf("No student records to analyze.\n");
        return;
    }

    float total_gpa = 0.0f;
    float highest   = students[0].gpa;
    float lowest    = students[0].gpa;
    int   top_idx   = 0;
    int   low_idx   = 0;

    for (int i = 0; i < total_count; i++) {
        total_gpa += students[i].gpa;

        if (students[i].gpa > highest) {
            highest = students[i].gpa;
            top_idx = i;
        }
        if (students[i].gpa < lowest) {
            lowest  = students[i].gpa;
            low_idx = i;
        }
    }

    float class_avg = total_gpa / total_count;

    /* ── Median GPA ── */
    // Sort a copy by GPA to find the median (middle value)
    // We use a simple method: create a temporary array of GPAs
    float* gpa_copy = malloc(total_count * sizeof(float));
    if (gpa_copy == NULL) {
        printf("[ERROR] Memory error during report generation.\n");
        return;
    }
    for (int i = 0; i < total_count; i++) gpa_copy[i] = students[i].gpa;

    // Bubble sort the copy
    for (int i = 0; i < total_count - 1; i++) {
        for (int j = 0; j < total_count - 1 - i; j++) {
            if (gpa_copy[j] > gpa_copy[j + 1]) {
                float t = gpa_copy[j]; gpa_copy[j] = gpa_copy[j+1]; gpa_copy[j+1] = t;
            }
        }
    }
    float median;
    if (total_count % 2 == 0) {
        // Even count: average the two middle values
        median = (gpa_copy[total_count/2 - 1] + gpa_copy[total_count/2]) / 2.0f;
    } else {
        // Odd count: take the exact middle
        median = gpa_copy[total_count / 2];
    }
    free(gpa_copy);  // Always free what you malloc!

    printf("Total Students     : %d\n",   total_count);
    printf("Class Average GPA  : %.2f\n", class_avg);
    printf("Highest GPA        : %.2f  (%s)\n", highest, students[top_idx].name);
    printf("Lowest GPA         : %.2f  (%s)\n", lowest,  students[low_idx].name);
    printf("Median GPA         : %.2f\n", median);

    /* ── Top 3 Students ── */
    print_separator();
    printf("Top 3 Students:\n");
    // Sort by GPA temporarily to show top 3
    sort_by_gpa();
    for (int i = 0; i < 3 && i < total_count; i++) {
        printf("  #%d: %s — GPA: %.2f\n", i+1, students[i].name, students[i].gpa);
    }
}

/* ============================================================
 *  FUNCTION: save_to_file
 *  Purpose : Write all student data to a binary file
 *  Why binary? Faster and more compact than text files.
 * ============================================================ */
void save_to_file() {
    FILE* file = fopen(DATA_FILE, "wb");  // "wb" = write binary
    if (file == NULL) {
        printf("[ERROR] Could not open file '%s' for writing.\n", DATA_FILE);
        return;
    }

    // First write the count so we know how many records to read later
    fwrite(&total_count, sizeof(int), 1, file);

    // Then write all student structs at once
    fwrite(students, sizeof(Student), total_count, file);

    fclose(file);
    printf("[SUCCESS] %d record(s) saved to '%s'.\n", total_count, DATA_FILE);
}

/* ============================================================
 *  FUNCTION: load_from_file
 *  Purpose : Read saved student data back from the binary file
 * ============================================================ */
void load_from_file() {
    FILE* file = fopen(DATA_FILE, "rb");  // "rb" = read binary
    if (file == NULL) {
        printf("(No save file found — starting fresh)\n");
        return;
    }

    // Read how many students were saved
    int count = 0;
    fread(&count, sizeof(int), 1, file);

    if (count <= 0) {
        fclose(file);
        return;
    }

    // Allocate memory for exactly that many students
    Student* temp = malloc(count * sizeof(Student));
    if (temp == NULL) {
        printf("[ERROR] Memory allocation failed when loading file.\n");
        fclose(file);
        return;
    }

    // Read all student records into our array
    fread(temp, sizeof(Student), count, file);
    fclose(file);

    // Replace current data with loaded data
    free(students);     // Release old memory
    students    = temp; // Point to new data
    total_count = count;

    printf("[SUCCESS] Loaded %d student record(s) from '%s'.\n", total_count, DATA_FILE);
}

/* ============================================================
 *  FUNCTION: free_memory
 *  Purpose : Release all dynamically allocated memory
 *  IMPORTANT: Always free() what you malloc() to avoid memory leaks!
 * ============================================================ */
void free_memory() {
    if (students != NULL) {
        free(students);   // Give the memory back to the OS
        students = NULL;  // Set to NULL so no accidental use
        total_count = 0;
    }
}

/* ── Helper display functions ─────────────────────────────── */
void print_separator() {
    printf("--------------------------------------------\n");
}

void print_header(const char* title) {
    printf("\n============================================\n");
    printf("  %s\n", title);
    printf("============================================\n");
}