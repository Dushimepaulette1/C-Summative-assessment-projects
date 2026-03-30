/*
 * ============================================================
 *  PROJECT 4: Data Analysis Toolkit — Function Pointers
 *  Language : C
 *  Author   : [Your Name]
 * ============================================================
 *
 *  HOW IT WORKS:
 *  - Stores a dataset of numbers in a dynamic array
 *  - Uses FUNCTION POINTERS to call the right operation
 *    when the user picks from a menu — no giant if-else chain!
 *  - Uses CALLBACK FUNCTIONS to filter, transform, and sort
 *
 *  KEY CONCEPT — Function Pointer:
 *  Instead of:     if (choice==1) compute_sum();
 *  We do:          operations[choice].func(dataset, size);
 *  This is cleaner, scalable, and is what function pointers are for!
 *
 *  HOW TO COMPILE:
 *  gcc -o toolkit project4_toolkit.c -lm
 *  ./toolkit
 * ============================================================
 */

#include <stdio.h>     // printf, scanf, fopen, fclose
#include <stdlib.h>    // malloc, realloc, free
#include <string.h>    // strcpy
#include <math.h>      // sqrt (for standard deviation)
#include <float.h>     // FLT_MAX, FLT_MIN

/* ── Dataset Constants ───────────────────────────────────── */
#define DATA_FILE "dataset.txt"

/* ── Global Dataset Variables ────────────────────────────── */
double* dataset    = NULL;  // Dynamic array of numbers
int     data_size  = 0;     // How many numbers are stored

/* ============================================================
 *  SECTION 1: OPERATION FUNCTIONS
 *  These are the functions we will call through function pointers.
 *  Each one does a specific data analysis task.
 * ============================================================ */

/* ── Operation 1: Sum and Average ── */
void op_sum_and_average(double* data, int size) {
    if (size == 0) { printf("[ERROR] Dataset is empty.\n"); return; }

    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        sum += data[i];
    }
    printf("  Sum     : %.4f\n", sum);
    printf("  Average : %.4f\n", sum / size);
}

/* ── Operation 2: Min and Max ── */
void op_min_max(double* data, int size) {
    if (size == 0) { printf("[ERROR] Dataset is empty.\n"); return; }

    double min_val = data[0];
    double max_val = data[0];
    int    min_idx = 0;
    int    max_idx = 0;

    for (int i = 1; i < size; i++) {
        if (data[i] < min_val) { min_val = data[i]; min_idx = i; }
        if (data[i] > max_val) { max_val = data[i]; max_idx = i; }
    }

    printf("  Minimum : %.4f  (at index %d)\n", min_val, min_idx);
    printf("  Maximum : %.4f  (at index %d)\n", max_val, max_idx);
}

/* ── Operation 3: Display Dataset ── */
void op_display(double* data, int size) {
    if (size == 0) { printf("Dataset is empty.\n"); return; }

    printf("  Dataset (%d elements):\n  [ ", size);
    for (int i = 0; i < size; i++) {
        printf("%.2f", data[i]);
        if (i < size - 1) printf(", ");
    }
    printf(" ]\n");
}

/* ── Operation 4: Standard Deviation ── */
void op_std_deviation(double* data, int size) {
    if (size < 2) { printf("[ERROR] Need at least 2 values.\n"); return; }

    // Step 1: Calculate mean (average)
    double sum = 0.0;
    for (int i = 0; i < size; i++) sum += data[i];
    double mean = sum / size;

    // Step 2: Sum of squared differences from mean
    double sq_diff_sum = 0.0;
    for (int i = 0; i < size; i++) {
        double diff = data[i] - mean;
        sq_diff_sum += diff * diff;
    }

    // Step 3: Square root of (squared diffs / count)
    double std_dev = sqrt(sq_diff_sum / size);

    printf("  Mean              : %.4f\n", mean);
    printf("  Standard Deviation: %.4f\n", std_dev);
}

/* ── Operation 5: Save to File ── */
void op_save_to_file(double* data, int size) {
    if (size == 0) { printf("[ERROR] Nothing to save.\n"); return; }

    FILE* file = fopen(DATA_FILE, "w");  // "w" = write text mode
    if (file == NULL) {
        printf("[ERROR] Cannot open file for writing.\n");
        return;
    }

    fprintf(file, "%d\n", size);  // Write count on first line
    for (int i = 0; i < size; i++) {
        fprintf(file, "%.6f\n", data[i]);  // One number per line
    }

    fclose(file);
    printf("  Saved %d values to '%s'.\n", size, DATA_FILE);
}

/* ============================================================
 *  SECTION 2: CALLBACK FUNCTIONS
 *  These are passed AS ARGUMENTS to other functions.
 *  They tell the processor "how" to filter or transform data.
 * ============================================================ */

/* ── Callback: Filter — keep values ABOVE a threshold ── */
int callback_filter_above(double value, double threshold) {
    return value > threshold;  // Returns 1 (true) or 0 (false)
}

/* ── Callback: Filter — keep values BELOW a threshold ── */
int callback_filter_below(double value, double threshold) {
    return value < threshold;
}

/* ── Callback: Transform — multiply each value by a scale ── */
double callback_scale(double value, double factor) {
    return value * factor;
}

/* ── Callback: Transform — add a constant to each value ── */
double callback_shift(double value, double amount) {
    return value + amount;
}

/* ── Callback: Comparator for sorting (ascending) ──
 * Used by our sort function — returns negative, 0, or positive
 * depending on whether a < b, a == b, or a > b.
 */
int callback_compare_ascending(const void* a, const void* b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    if (da < db) return -1;
    if (da > db) return  1;
    return 0;
}

/* ── Callback: Comparator for sorting (descending) ── */
int callback_compare_descending(const void* a, const void* b) {
    // Just reverse the ascending comparison
    return callback_compare_ascending(b, a);
}

/* ============================================================
 *  SECTION 3: HIGHER-ORDER FUNCTIONS
 *  These functions ACCEPT callback functions as parameters.
 *  This is the key pattern for callbacks in C!
 * ============================================================ */

/*
 * FUNCTION: apply_filter
 * Accepts a callback function pointer: int (*filter_fn)(double, double)
 * Keeps elements where filter_fn returns 1 (true).
 */
void apply_filter(int (*filter_fn)(double, double), double threshold) {
    if (data_size == 0) { printf("[ERROR] Dataset is empty.\n"); return; }

    /* Build a new array of only the values that pass the filter */
    double* filtered = malloc(data_size * sizeof(double));
    if (filtered == NULL) { printf("[ERROR] Memory allocation failed.\n"); return; }

    int new_size = 0;
    for (int i = 0; i < data_size; i++) {
        // Call the callback to decide if this value should be kept
        if (filter_fn(dataset[i], threshold)) {
            filtered[new_size++] = dataset[i];
        }
    }

    // Replace the global dataset with the filtered result
    free(dataset);
    dataset   = filtered;
    data_size = new_size;

    printf("  Filter applied. Remaining elements: %d\n", data_size);
    op_display(dataset, data_size);
}

/*
 * FUNCTION: apply_transform
 * Accepts a callback function pointer: double (*transform_fn)(double, double)
 * Applies the transformation to EVERY element in the dataset.
 */
void apply_transform(double (*transform_fn)(double, double), double param) {
    if (data_size == 0) { printf("[ERROR] Dataset is empty.\n"); return; }

    for (int i = 0; i < data_size; i++) {
        // Replace each value with the callback's result
        dataset[i] = transform_fn(dataset[i], param);
    }

    printf("  Transformation applied to all %d elements.\n", data_size);
    op_display(dataset, data_size);
}

/*
 * FUNCTION: sort_with_comparator
 * Uses qsort() from the C standard library.
 * qsort() accepts a comparator CALLBACK to decide ordering.
 */
void sort_with_comparator(int (*compare_fn)(const void*, const void*)) {
    if (data_size == 0) { printf("[ERROR] Dataset is empty.\n"); return; }

    // qsort(array, count, element_size, comparator_function)
    qsort(dataset, data_size, sizeof(double), compare_fn);

    printf("  Dataset sorted.\n");
    op_display(dataset, data_size);
}

/* ============================================================
 *  SECTION 4: FUNCTION POINTER DISPATCH TABLE
 *  This is the CORE of Project 4.
 *
 *  Instead of:
 *    if (choice == 1) op_sum_and_average(...);
 *    else if (choice == 2) op_min_max(...);
 *    ...
 *
 *  We store function pointers in an array (dispatch table)
 *  and call them by index. Clean, scalable!
 * ============================================================ */

/* Define a type for operation functions to keep things readable */
typedef void (*OperationFn)(double*, int);

/* Structure holding one menu option + its function pointer */
typedef struct {
    const char* label;   // What to show in the menu
    OperationFn func;    // The function to call
} Operation;

/* The dispatch table — maps menu index to function */
Operation operations[] = {
    { "Compute Sum and Average",    op_sum_and_average  },
    { "Find Minimum and Maximum",   op_min_max          },
    { "Display Dataset",            op_display          },
    { "Compute Standard Deviation", op_std_deviation    },
    { "Save Dataset to File",       op_save_to_file     },
};

/* How many operations are in the table (auto-calculated) */
#define NUM_OPERATIONS (int)(sizeof(operations) / sizeof(operations[0]))

/* ============================================================
 *  SECTION 5: DATASET MANAGEMENT FUNCTIONS
 * ============================================================ */

/* ── Create / add values to dataset ── */
void create_dataset() {
    printf("How many values to enter: ");
    int n;
    if (scanf("%d", &n) != 1 || n <= 0) {
        while (getchar() != '\n');
        printf("[ERROR] Invalid number.\n");
        return;
    }
    while (getchar() != '\n');

    // Expand the dataset using realloc
    double* temp = realloc(dataset, (data_size + n) * sizeof(double));
    if (temp == NULL) {
        printf("[ERROR] Memory allocation failed.\n");
        return;
    }
    dataset = temp;

    for (int i = 0; i < n; i++) {
        printf("  Value %d: ", i + 1);
        if (scanf("%lf", &dataset[data_size]) != 1) {
            while (getchar() != '\n');
            printf("[ERROR] Invalid number. Skipping.\n");
            continue;
        }
        while (getchar() != '\n');
        data_size++;
    }
    printf("[SUCCESS] %d value(s) added. Total: %d\n", n, data_size);
}

/* ── Load dataset from file ── */
void load_dataset_from_file() {
    FILE* file = fopen(DATA_FILE, "r");
    if (file == NULL) {
        printf("[ERROR] Cannot open '%s'. Does it exist?\n", DATA_FILE);
        return;
    }

    int count = 0;
    fscanf(file, "%d", &count);

    if (count <= 0) {
        printf("[ERROR] File is empty or invalid.\n");
        fclose(file);
        return;
    }

    // Allocate fresh memory for the loaded data
    free(dataset);
    dataset = malloc(count * sizeof(double));
    if (dataset == NULL) {
        printf("[ERROR] Memory allocation failed.\n");
        fclose(file);
        return;
    }

    data_size = 0;
    for (int i = 0; i < count; i++) {
        if (fscanf(file, "%lf", &dataset[data_size]) == 1) {
            data_size++;
        }
    }
    fclose(file);
    printf("[SUCCESS] Loaded %d value(s) from '%s'.\n", data_size, DATA_FILE);
}

/* ── Search for a value ── */
void search_value() {
    if (data_size == 0) { printf("[ERROR] Dataset is empty.\n"); return; }

    double target;
    printf("Enter value to search: ");
    if (scanf("%lf", &target) != 1) {
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');

    int found = 0;
    for (int i = 0; i < data_size; i++) {
        if (dataset[i] == target) {
            printf("  Found %.4f at index %d\n", target, i);
            found = 1;
        }
    }
    if (!found) printf("  Value %.4f not found in dataset.\n", target);
}

/* ── Reset / clear the dataset ── */
void reset_dataset() {
    free(dataset);
    dataset   = NULL;
    data_size = 0;
    printf("Dataset cleared.\n");
}

/* ============================================================
 *  SECTION 6: INTERACTIVE MENU
 * ============================================================ */

void print_separator() { printf("--------------------------------------------\n"); }
void print_header(const char* t) {
    printf("\n============================================\n  %s\n============================================\n", t);
}

void show_main_menu() {
    print_header("DATA ANALYSIS TOOLKIT — MENU");
    printf("  Dataset size: %d element(s)\n\n", data_size);
    printf("  [DATA MANAGEMENT]\n");
    printf("  1. Create / Add values to dataset\n");
    printf("  2. Load dataset from file\n");
    printf("  3. Reset (clear) dataset\n\n");
    printf("  [ANALYSIS OPERATIONS — via Function Pointer Dispatch]\n");
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        printf("  %d. %s\n", i + 4, operations[i].label);
    }
    printf("\n  [CALLBACK OPERATIONS]\n");
    int next = 4 + NUM_OPERATIONS;
    printf("  %d. Filter — keep values above threshold\n",  next);
    printf("  %d. Filter — keep values below threshold\n",  next + 1);
    printf("  %d. Transform — scale (multiply) all values\n", next + 2);
    printf("  %d. Transform — shift (add constant)\n",      next + 3);
    printf("  %d. Sort ascending\n",                        next + 4);
    printf("  %d. Sort descending\n",                       next + 5);
    printf("  %d. Search for value\n",                      next + 6);
    printf("  %d. Exit\n",                                  next + 7);
    print_separator();
    printf("Choice: ");
}

/* ============================================================
 *  MAIN PROGRAM
 * ============================================================ */
int main() {
    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║  Data Analysis Toolkit (Function Ptrs)  ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    int choice;
    int exit_choice = 4 + NUM_OPERATIONS + 7;  // Dynamically calculated

    while (1) {
        show_main_menu();

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("[ERROR] Please enter a valid number.\n");
            continue;
        }
        while (getchar() != '\n');

        /* ── Data Management Options ── */
        if (choice == 1) { create_dataset(); continue; }
        if (choice == 2) { load_dataset_from_file(); continue; }
        if (choice == 3) { reset_dataset(); continue; }

        /* ── Function Pointer Dispatch Table Operations ──
         * Choices 4 to (4 + NUM_OPERATIONS - 1) are dispatched
         * through the operations[] table automatically.
         */
        int op_idx = choice - 4;  // Convert menu choice to array index
        if (op_idx >= 0 && op_idx < NUM_OPERATIONS) {
            print_header(operations[op_idx].label);

            /* NULL check: make sure the function pointer is valid */
            if (operations[op_idx].func == NULL) {
                printf("[ERROR] Operation function is NULL!\n");
            } else {
                /* ★ THIS IS THE FUNCTION POINTER CALL ★
                 * We're calling a function stored in our dispatch table.
                 */
                operations[op_idx].func(dataset, data_size);
            }
            continue;
        }

        /* ── Callback-Based Options ── */
        int cb_start = 4 + NUM_OPERATIONS;
        double param;

        if (choice == cb_start) {
            printf("Enter threshold (keep values ABOVE): ");
            scanf("%lf", &param); while (getchar() != '\n');
            apply_filter(callback_filter_above, param);
        }
        else if (choice == cb_start + 1) {
            printf("Enter threshold (keep values BELOW): ");
            scanf("%lf", &param); while (getchar() != '\n');
            apply_filter(callback_filter_below, param);
        }
        else if (choice == cb_start + 2) {
            printf("Enter scale factor (multiply by): ");
            scanf("%lf", &param); while (getchar() != '\n');
            apply_transform(callback_scale, param);
        }
        else if (choice == cb_start + 3) {
            printf("Enter shift amount (add to each): ");
            scanf("%lf", &param); while (getchar() != '\n');
            apply_transform(callback_shift, param);
        }
        else if (choice == cb_start + 4) {
            sort_with_comparator(callback_compare_ascending);
        }
        else if (choice == cb_start + 5) {
            sort_with_comparator(callback_compare_descending);
        }
        else if (choice == cb_start + 6) {
            search_value();
        }
        else if (choice == exit_choice) {
            free(dataset);   // Release memory before exit
            dataset = NULL;
            printf("Goodbye!\n");
            return 0;
        }
        else {
            printf("[ERROR] Invalid option. Please try again.\n");
        }
    }
}