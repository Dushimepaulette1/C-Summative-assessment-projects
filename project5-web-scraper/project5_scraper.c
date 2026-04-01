#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>

#define MAX_URL_LEN  512
#define MAX_URLS     20
#define OUTPUT_DIR   "scraped_output"


typedef struct {
    int  thread_id;
    char url[MAX_URL_LEN];
    int  success;
    long response_code;
    long bytes_downloaded;
} ThreadArgs;

typedef struct {
    char*  data;
    size_t size;
} WriteBuffer;


static size_t write_chunk_to_buffer(void* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t chunk_size = size * nmemb;
    WriteBuffer* buf  = (WriteBuffer*)userdata;

    char* new_data = realloc(buf->data, buf->size + chunk_size + 1);
    if (new_data == NULL) {
        printf("[Thread ERROR] Memory allocation failed in write callback.\n");
        return 0;
    }

    buf->data = new_data;
    memcpy(buf->data + buf->size, ptr, chunk_size);
    buf->size += chunk_size;
    buf->data[buf->size] = '\0';

    return chunk_size;
}

void save_content_to_file(int thread_id, const char* url, const char* content, long bytes) {
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/output_%d.txt", OUTPUT_DIR, thread_id);

    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("[Thread %d] ERROR: Cannot create output file '%s'\n", thread_id, filename);
        return;
    }

    fprintf(file, "==========================================================\n");
    fprintf(file, "  Thread ID : %d\n", thread_id);
    fprintf(file, "  URL       : %s\n", url);
    fprintf(file, "  Bytes     : %ld\n", bytes);
    fprintf(file, "==========================================================\n\n");
    fprintf(file, "%s", content);

    fclose(file);
    printf("[Thread %d] Saved %ld bytes -> %s\n", thread_id, bytes, filename);
}


void* thread_worker(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;

    printf("[Thread %d] Starting download: %s\n", args->thread_id, args->url);

    CURL* curl = curl_easy_init();
    if (curl == NULL) {
        printf("[Thread %d] ERROR: Failed to initialize libcurl.\n", args->thread_id);
        args->success = 0;
        pthread_exit(NULL);
    }

    WriteBuffer buffer;
    buffer.data = malloc(1);
    buffer.size = 0;
    if (buffer.data) buffer.data[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL,           args->url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_chunk_to_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,      "Mozilla/5.0 (MultiThreadedScraper/1.0)");

    CURLcode result = curl_easy_perform(curl);

    if (result != CURLE_OK) {
        printf("[Thread %d] ERROR downloading '%s': %s\n",
               args->thread_id, args->url, curl_easy_strerror(result));
        args->success = 0;
    } else {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &args->response_code);
        args->bytes_downloaded = (long)buffer.size;
        args->success = 1;

        printf("[Thread %d] Downloaded %s — HTTP %ld — %ld bytes\n",
               args->thread_id, args->url, args->response_code, args->bytes_downloaded);

        if (buffer.data != NULL && buffer.size > 0) {
            save_content_to_file(args->thread_id, args->url, buffer.data, args->bytes_downloaded);
        }
    }

    curl_easy_cleanup(curl);
    if (buffer.data) free(buffer.data);

    pthread_exit(NULL);
}

int load_urls_from_file(const char* filename, char urls[][MAX_URL_LEN]) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("URL file '%s' not found. Using default URLs.\n", filename);
        return 0;
    }

    int count = 0;
    while (count < MAX_URLS && fgets(urls[count], MAX_URL_LEN, file) != NULL) {
        urls[count][strcspn(urls[count], "\n")] = '\0';
        if (strlen(urls[count]) > 0) count++;
    }

    fclose(file);
    printf("Loaded %d URL(s) from '%s'.\n", count, filename);
    return count;
}

void print_summary(ThreadArgs* args, int count) {
    printf("\n============================================================\n");
    printf("                 SCRAPING SUMMARY\n");
    printf("============================================================\n");
    printf("%-5s %-40s %-6s %-8s %-10s\n", "ID", "URL", "HTTP", "Bytes", "Status");
    printf("------------------------------------------------------------\n");

    int success_count = 0;
    for (int i = 0; i < count; i++) {
        char short_url[41];
        strncpy(short_url, args[i].url, 40);
        short_url[40] = '\0';
        if (strlen(args[i].url) > 40) {
            short_url[37] = '.'; short_url[38] = '.'; short_url[39] = '.';
        }

        printf("%-5d %-40s %-6ld %-8ld %s\n",
               args[i].thread_id, short_url,
               args[i].response_code, args[i].bytes_downloaded,
               args[i].success ? "OK" : "FAILED");

        if (args[i].success) success_count++;
    }

    printf("------------------------------------------------------------\n");
    printf("Result: %d/%d URLs downloaded successfully.\n", success_count, count);
    printf("Output files saved in: %s/\n", OUTPUT_DIR);
    printf("============================================================\n");
}

int main(int argc, char* argv[]) {
    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║   Multi-threaded Web Scraper (pthreads)  ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    char urls[MAX_URLS][MAX_URL_LEN] = {
        "http://example.com",
        "http://httpbin.org/get",
        "http://httpbin.org/ip",
        "http://httpbin.org/user-agent",
    };
    int url_count = 4;

    if (argc >= 2) {
        int loaded = load_urls_from_file(argv[1], urls);
        if (loaded > 0) url_count = loaded;
    }

    system("mkdir -p " OUTPUT_DIR);
    printf("Output directory: %s/\n\n", OUTPUT_DIR);

    curl_global_init(CURL_GLOBAL_ALL);

    pthread_t*  threads = malloc(url_count * sizeof(pthread_t));
    ThreadArgs* args    = malloc(url_count * sizeof(ThreadArgs));

    if (threads == NULL || args == NULL) {
        printf("[ERROR] Memory allocation failed.\n");
        return 1;
    }

    printf("Starting %d threads...\n\n", url_count);

    
    for (int i = 0; i < url_count; i++) {
        args[i].thread_id       = i;
        args[i].success         = 0;
        args[i].response_code   = 0;
        args[i].bytes_downloaded = 0;
        strncpy(args[i].url, urls[i], MAX_URL_LEN - 1);
        args[i].url[MAX_URL_LEN - 1] = '\0';

        int rc = pthread_create(&threads[i], NULL, thread_worker, &args[i]);
        if (rc != 0) {
            printf("[ERROR] Failed to create thread %d for URL: %s\n", i, urls[i]);
            args[i].success = 0;
        } else {
            printf("[MAIN] Thread %d launched for: %s\n", i, urls[i]);
        }
    }

    printf("\n[MAIN] All threads launched. Waiting for completion...\n\n");


    for (int i = 0; i < url_count; i++) {
        pthread_join(threads[i], NULL);
        printf("[MAIN] Thread %d finished.\n", i);
    }

    printf("\n[MAIN] All threads complete.\n");

    print_summary(args, url_count);

    free(threads);
    free(args);
    curl_global_cleanup();

    return 0;
}
