#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define DATA_FILE "dataset.txt"


double* dataset   = NULL;
int     data_size = 0;


void op_sum_and_average(double* data, int size) {
    if (size == 0) { printf("[ERROR] Dataset is empty.\n"); return; }

    double sum = 0.0;
    for (int i = 0; i < size; i++) sum += data[i];

    printf("  Sum     : %.4f\n", sum);
    printf("  Average : %.4f\n", sum / size);
}

void op_min_max(double* data, int size) {
    if (size == 0) { printf("[ERROR] Dataset is empty.\n"); return; }

    double min_val = data[0], max_val = data[0];
    int    min_idx = 0,       max_idx = 0;

    for (int i = 1; i < size; i++) {
        if (data[i] < min_val) { min_val = data[i]; min_idx = i; }
        if (data[i] > max_val) { max_val = data[i]; max_idx = i; }
    }

    printf("  Minimum : %.4f  (at index %d)\n", min_val, min_idx);
    printf("  Maximum : %.4f  (at index %d)\n", max_val, max_idx);
}

void op_display(double* data, int size) {
    if (size == 0) { printf("Dataset is empty.\n"); return; }

    printf("  Dataset (%d elements):\n  [ ", size);
    for (int i = 0; i < size; i++) {
        printf("%.2f", data[i]);
        if (i < size - 1) printf(", ");
    }
    printf(" ]\n");
}

void op_std_deviation(double* data, int size) {
    if (size < 2) { printf("[ERROR] Need at least 2 values.\n"); return; }

    double sum = 0.0;
    for (int i = 0; i < size; i++) sum += data[i];
    double mean = sum / size;

    double sq_diff_sum = 0.0;
    for (int i = 0; i < size; i++) {
        double diff = data[i] - mean;
        sq_diff_sum += diff * diff;
    }

    printf("  Mean              : %.4f\n", mean);
    printf("  Standard Deviation: %.4f\n", sqrt(sq_diff_sum / size));
}

void op_save_to_file(double* data, int size) {
    if (size == 0) { printf("[ERROR] Nothing to save.\n"); return; }

    FILE* file = fopen(DATA_FILE, "w");
    if (file == NULL) {
        printf("[ERROR] Cannot open file for writing.\n");
        return;
    }

    fprintf(file, "%d\n", size);
    for (int i = 0; i < size; i++) fprintf(file, "%.6f\n", data[i]);

    fclose(file);
    printf("  Saved %d values to '%s'.\n", size, DATA_FILE);
}



int callback_filter_above(double value, double threshold) { return value > threshold; }
int callback_filter_below(double value, double threshold) { return value < threshold; }


double callback_scale(double value, double factor) { return value * factor; }
double callback_shift(double value, double amount) { return value + amount; }

int callback_compare_ascending(const void* a, const void* b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    if (da < db) return -1;
    if (da > db) return  1;
    return 0;
}

int callback_compare_descending(const void* a, const void* b) {
    return callback_compare_ascending(b, a);
}


void apply_filter(int (*filter_fn)(double, double), double threshold) {
    if (data_size == 0) { printf("[ERROR] Dataset is empty.\n"); return; }

    double* filtered = malloc(data_size * sizeof(double));
    if (filtered == NULL) { printf("[ERROR] Memory allocation failed.\n"); return; }

    int new_size = 0;
    for (int i = 0; i < data_size; i++) {
        if (filter_fn(dataset[i], threshold)) {
            filtered[new_size++] = dataset[i];
        }
    }

    free(dataset);
    dataset   = filtered;
    data_size = new_size;

    printf("  Filter applied. Remaining elements: %d\n", data_size);
    op_display(dataset, data_size);
}


void apply_transform(double (*transform_fn)(double, double), double param) {
    if (data_size == 0) { printf("[ERROR] Dataset is empty.\n"); return; }

    for (int i = 0; i < data_size; i++) {
        dataset[i] = transform_fn(dataset[i], param);
    }

    printf("  Transformation applied to all %d elements.\n", data_size);
    op_display(dataset, data_size);
}

void sort_with_comparator(int (*compare_fn)(const void*, const void*)) {
    if (data_size == 0) { printf("[ERROR] Dataset is empty.\n"); return; }
    qsort(dataset, data_size, sizeof(double), compare_fn);
    printf("  Dataset sorted.\n");
    op_display(dataset, data_size);
}


typedef void (*OperationFn)(double*, int);

typedef struct {
    const char* label;
    OperationFn func;
} Operation;

Operation operations[] = {
    { "Compute Sum and Average",    op_sum_and_average },
    { "Find Minimum and Maximum",   op_min_max         },
    { "Display Dataset",            op_display         },
    { "Compute Standard Deviation", op_std_deviation   },
    { "Save Dataset to File",       op_save_to_file    },
};

#define NUM_OPERATIONS (int)(sizeof(operations) / sizeof(operations[0]))


void create_dataset() {
    printf("How many values to enter: ");
    int n;
    if (scanf("%d", &n) != 1 || n <= 0) {
        while (getchar() != '\n');
        printf("[ERROR] Invalid number.\n");
        return;
    }
    while (getchar() != '\n');

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

    free(dataset);
    dataset = malloc(count * sizeof(double));
    if (dataset == NULL) {
        printf("[ERROR] Memory allocation failed.\n");
        fclose(file);
        return;
    }

    data_size = 0;
    for (int i = 0; i < count; i++) {
        if (fscanf(file, "%lf", &dataset[data_size]) == 1) data_size++;
    }
    fclose(file);
    printf("[SUCCESS] Loaded %d value(s) from '%s'.\n", data_size, DATA_FILE);
}

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

void reset_dataset() {
    free(dataset);
    dataset   = NULL;
    data_size = 0;
    printf("Dataset cleared.\n");
}



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

    printf("  [ANALYSIS — dispatched via function pointer table]\n");
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        printf("  %d. %s\n", i + 4, operations[i].label);
    }

    printf("\n  [CALLBACK OPERATIONS]\n");
    int next = 4 + NUM_OPERATIONS;
    printf("  %d.  Filter — keep values above threshold\n",   next);
    printf("  %d.  Filter — keep values below threshold\n",   next + 1);
    printf("  %d.  Transform — scale (multiply) all values\n",next + 2);
    printf("  %d.  Transform — shift (add constant)\n",       next + 3);
    printf("  %d.  Sort ascending\n",                         next + 4);
    printf("  %d.  Sort descending\n",                        next + 5);
    printf("  %d.  Search for value\n",                       next + 6);
    printf("  %d.  Exit\n",                                   next + 7);
    print_separator();
    printf("Choice: ");
}

int main() {
    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║  Data Analysis Toolkit (Function Ptrs)  ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    int choice;
    int exit_choice = 4 + NUM_OPERATIONS + 7;

    while (1) {
        show_main_menu();

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("[ERROR] Please enter a valid number.\n");
            continue;
        }
        while (getchar() != '\n');

        if (choice == 1) { create_dataset();          continue; }
        if (choice == 2) { load_dataset_from_file();  continue; }
        if (choice == 3) { reset_dataset();            continue; }

       
        int op_idx = choice - 4;
        if (op_idx >= 0 && op_idx < NUM_OPERATIONS) {
            print_header(operations[op_idx].label);
            if (operations[op_idx].func == NULL) {
                printf("[ERROR] Operation function is NULL!\n");
            } else {
                operations[op_idx].func(dataset, data_size);
            }
            continue;
        }
\
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
        else if (choice == cb_start + 4) { sort_with_comparator(callback_compare_ascending);  }
        else if (choice == cb_start + 5) { sort_with_comparator(callback_compare_descending); }
        else if (choice == cb_start + 6) { search_value(); }
        else if (choice == exit_choice) {
            free(dataset);
            dataset = NULL;
            printf("Goodbye!\n");
            return 0;
        }
        else {
            printf("[ERROR] Invalid option. Please try again.\n");
        }
    }
}
