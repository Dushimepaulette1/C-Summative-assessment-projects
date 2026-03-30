/*
 * ============================================================
 *  PROJECT 5: Multi-threaded Web Scraper
 *  Language : C (POSIX threads — pthreads)
 *  Author   : [Your Name]
 * ============================================================
 *
 *  HOW IT WORKS:
 *  - Reads a list of URLs (from a file or hardcoded list)
 *  - Creates ONE thread per URL
 *  - Each thread downloads its URL's HTML content in PARALLEL
 *    (all threads run at the same time — that's concurrency!)
 *  - Each thread saves output to its own file: output_0.txt, etc.
 *  - Handles errors if a URL is unreachable
 *
 *  WHY THREADS?
 *  Without threads: Download URL 1, wait, download URL 2, wait...
 *  With threads:    Download ALL URLs simultaneously → much faster!
 *
 *  DEPENDENCIES:
 *  - libcurl  (for downloading web pages)
 *  - pthreads (built into Linux)
 *
 *  HOW TO INSTALL libcurl (on Ubuntu/Debian):
 *  sudo apt-get install libcurl4-openssl-dev
 *
 *  HOW TO COMPILE:
 *  gcc -o scraper project5_scraper.c -lpthread -lcurl
 *
 *  HOW TO RUN:
 *  ./scraper
 *  (or with a custom URL list file: ./scraper urls.txt)
 * ============================================================
 */

#include <stdio.h>      // printf, fopen, fclose, fprintf
#include <stdlib.h>     // malloc, free, exit
#include <string.h>     // strcpy, strlen, snprintf
#include <pthread.h>    // POSIX threads: pthread_t, pthread_create, pthread_join
#include <curl/curl.h>  // libcurl: for HTTP downloading

/* ── Constants ───────────────────────────────────────────── */
#define MAX_URL_LEN     512   // Maximum URL string length
#define MAX_URLS        20    // Maximum number of URLs to scrape
#define OUTPUT_DIR      "scraped_output"  // Folder for saved files

/* ── Structure: ThreadArgs ────────────────────────────────
 * Each thread needs to know:
 *   - Which URL to download
 *   - Its thread number (so it saves to the right file)
 * We bundle these into a struct and pass it to the thread.
 */
typedef struct {
    int  thread_id;           // Thread number (0, 1, 2, ...)
    char url[MAX_URL_LEN];    // The URL this thread should fetch
    int  success;             // Did the download succeed? 1=yes, 0=no
    long response_code;       // HTTP status code (200=OK, 404=Not Found, etc.)
    long bytes_downloaded;    // How many bytes were downloaded
} ThreadArgs;

/* ── Structure: WriteBuffer ───────────────────────────────
 * libcurl delivers data in chunks.
 * We use this buffer to accumulate all chunks into one string.
 */
typedef struct {
    char*  data;    // Pointer to the accumulated content
    size_t size;    // Total bytes received so far
} WriteBuffer;

/* ============================================================
 *  FUNCTION: write_chunk_to_buffer
 *  Purpose : libcurl calls this every time it receives data.
 *            We append each chunk to our WriteBuffer.
 *
 *  This is a CALLBACK — libcurl calls it for us automatically!
 *
 *  Parameters (required by libcurl's API):
 *    ptr      → the new chunk of data
 *    size     → always 1 (libcurl convention)
 *    nmemb    → how many bytes in this chunk
 *    userdata → our WriteBuffer (cast from void*)
 *
 *  Returns: number of bytes handled (must equal size * nmemb)
 * ============================================================ */
static size_t write_chunk_to_buffer(void* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t chunk_size = size * nmemb;
    WriteBuffer* buf  = (WriteBuffer*)userdata;

    /* Grow the buffer to fit the new chunk
     * realloc(existing_ptr, new_size) resizes the memory block.
     * We add +1 for the null terminator at the end.
     */
    char* new_data = realloc(buf->data, buf->size + chunk_size + 1);
    if (new_data == NULL) {
        printf("[Thread ERROR] Memory allocation failed in write callback.\n");
        return 0;  // Tell libcurl something went wrong
    }

    buf->data = new_data;

    /* Copy the new chunk onto the end of the existing buffer */
    memcpy(buf->data + buf->size, ptr, chunk_size);
    buf->size += chunk_size;

    /* Always null-terminate so we can treat it as a C string */
    buf->data[buf->size] = '\0';

    return chunk_size;  // Must return the number of bytes processed
}

/* ============================================================
 *  FUNCTION: save_content_to_file
 *  Purpose : Write the downloaded HTML to a text file.
 *  Params  : thread_id (for filename), url, html content
 * ============================================================ */
void save_content_to_file(int thread_id, const char* url, const char* content, long bytes) {
    char filename[256];

    /* Build filename like: scraped_output/output_0.txt */
    snprintf(filename, sizeof(filename), "%s/output_%d.txt", OUTPUT_DIR, thread_id);

    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("[Thread %d] ERROR: Cannot create output file '%s'\n", thread_id, filename);
        return;
    }

    /* Write a header section with metadata */
    fprintf(file, "==========================================================\n");
    fprintf(file, "  Thread ID : %d\n", thread_id);
    fprintf(file, "  URL       : %s\n", url);
    fprintf(file, "  Bytes     : %ld\n", bytes);
    fprintf(file, "==========================================================\n\n");

    /* Write the actual HTML content */
    fprintf(file, "%s", content);

    fclose(file);
    printf("[Thread %d] Saved %ld bytes → %s\n", thread_id, bytes, filename);
}

/* ============================================================
 *  FUNCTION: thread_worker
 *  Purpose : This is the function each thread runs.
 *            Every thread runs this same function but with
 *            different data (different URL, different ID).
 *
 *  Params  : arg → a void* pointer to ThreadArgs struct
 *  Returns : NULL (pthreads requirement)
 *
 *  KEY CONCEPT: Each thread gets its own stack and runs
 *  independently and simultaneously with other threads!
 * ============================================================ */
void* thread_worker(void* arg) {
    /* Cast the void* argument back to our ThreadArgs struct */
    ThreadArgs* args = (ThreadArgs*)arg;

    printf("[Thread %d] Starting download: %s\n", args->thread_id, args->url);

    /* ── Initialize libcurl for this thread ── */
    CURL* curl = curl_easy_init();
    if (curl == NULL) {
        printf("[Thread %d] ERROR: Failed to initialize libcurl.\n", args->thread_id);
        args->success = 0;
        pthread_exit(NULL);  // Exit this thread (not the whole program!)
    }

    /* ── Set up our write buffer ── */
    WriteBuffer buffer;
    buffer.data = malloc(1);  // Start with 1 byte (will grow as needed)
    buffer.size = 0;
    if (buffer.data) buffer.data[0] = '\0';

    /* ── Configure libcurl options ── */
    curl_easy_setopt(curl, CURLOPT_URL, args->url);

    /* Tell libcurl to use our callback to receive data */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_chunk_to_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &buffer);

    /* Follow redirects (e.g., HTTP → HTTPS) */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    /* Timeout after 10 seconds so we don't wait forever */
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    /* Set a user-agent string (some servers reject requests without one) */
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (MultiThreadedScraper/1.0)");

    /* ── Perform the download ── */
    CURLcode result = curl_easy_perform(curl);

    /* ── Check the result ── */
    if (result != CURLE_OK) {
        /* Download failed — log the error but don't crash */
        printf("[Thread %d] ERROR downloading '%s': %s\n",
               args->thread_id, args->url, curl_easy_strerror(result));
        args->success = 0;
    } else {
        /* Download succeeded! Get the HTTP response code */
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &args->response_code);
        args->bytes_downloaded = (long)buffer.size;
        args->success = 1;

        printf("[Thread %d] Downloaded %s — HTTP %ld — %ld bytes\n",
               args->thread_id, args->url, args->response_code, args->bytes_downloaded);

        /* Save the content to a file */
        if (buffer.data != NULL && buffer.size > 0) {
            save_content_to_file(args->thread_id, args->url, buffer.data, args->bytes_downloaded);
        }
    }

    /* ── Clean up this thread's resources ── */
    curl_easy_cleanup(curl);
    if (buffer.data) free(buffer.data);

    /* pthread_exit(NULL) ends this thread gracefully */
    pthread_exit(NULL);
}

/* ============================================================
 *  FUNCTION: load_urls_from_file
 *  Purpose : Read URLs from a text file (one URL per line)
 *  Returns : number of URLs loaded
 * ============================================================ */
int load_urls_from_file(const char* filename, char urls[][MAX_URL_LEN]) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("URL file '%s' not found. Using default URLs.\n", filename);
        return 0;
    }

    int count = 0;
    while (count < MAX_URLS && fgets(urls[count], MAX_URL_LEN, file) != NULL) {
        /* Remove trailing newline */
        urls[count][strcspn(urls[count], "\n")] = '\0';

        /* Skip empty lines */
        if (strlen(urls[count]) > 0) {
            count++;
        }
    }

    fclose(file);
    printf("Loaded %d URL(s) from '%s'.\n", count, filename);
    return count;
}

/* ============================================================
 *  FUNCTION: print_summary
 *  Purpose : Show a results table after all threads complete
 * ============================================================ */
void print_summary(ThreadArgs* args, int count) {
    printf("\n============================================================\n");
    printf("                 SCRAPING SUMMARY\n");
    printf("============================================================\n");
    printf("%-5s %-40s %-6s %-8s %-10s\n",
           "ID", "URL", "HTTP", "Bytes", "Status");
    printf("------------------------------------------------------------\n");

    int success_count = 0;
    for (int i = 0; i < count; i++) {
        /* Truncate long URLs for display */
        char short_url[41];
        strncpy(short_url, args[i].url, 40);
        short_url[40] = '\0';
        if (strlen(args[i].url) > 40) {
            short_url[37] = '.';
            short_url[38] = '.';
            short_url[39] = '.';
        }

        printf("%-5d %-40s %-6ld %-8ld %s\n",
               args[i].thread_id,
               short_url,
               args[i].response_code,
               args[i].bytes_downloaded,
               args[i].success ? "✓ OK" : "✗ FAILED");

        if (args[i].success) success_count++;
    }

    printf("------------------------------------------------------------\n");
    printf("Result: %d/%d URLs downloaded successfully.\n", success_count, count);
    printf("Output files saved in: %s/\n", OUTPUT_DIR);
    printf("============================================================\n");
}

/* ============================================================
 *  MAIN PROGRAM
 * ============================================================ */
int main(int argc, char* argv[]) {
    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║   Multi-threaded Web Scraper (pthreads)  ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* ── Default URLs to scrape if no file is provided ── */
    char urls[MAX_URLS][MAX_URL_LEN] = {
        "http://example.com",
        "http://httpbin.org/get",
        "http://httpbin.org/ip",
        "http://httpbin.org/user-agent",
    };
    int url_count = 4;

    /* If user provided a URL file as argument, load from it */
    if (argc >= 2) {
        int loaded = load_urls_from_file(argv[1], urls);
        if (loaded > 0) url_count = loaded;
    }

    /* ── Create output directory for scraped files ── */
    /* system() runs a shell command */
    system("mkdir -p " OUTPUT_DIR);
    printf("Output directory: %s/\n\n", OUTPUT_DIR);

    /* ── Initialize libcurl globally (must be done once) ── */
    curl_global_init(CURL_GLOBAL_ALL);

    /* ── Allocate arrays for threads and their arguments ── */
    pthread_t*  threads = malloc(url_count * sizeof(pthread_t));
    ThreadArgs* args    = malloc(url_count * sizeof(ThreadArgs));

    if (threads == NULL || args == NULL) {
        printf("[ERROR] Memory allocation failed.\n");
        return 1;
    }

    printf("Starting %d threads...\n\n", url_count);

    /* ── CREATE THREADS ──
     * pthread_create() starts a new thread running thread_worker()
     * Each thread gets its own ThreadArgs with a unique URL.
     *
     * At this point, ALL threads run in PARALLEL!
     */
    for (int i = 0; i < url_count; i++) {
        /* Fill in arguments for this thread */
        args[i].thread_id      = i;
        args[i].success        = 0;
        args[i].response_code  = 0;
        args[i].bytes_downloaded = 0;
        strncpy(args[i].url, urls[i], MAX_URL_LEN - 1);
        args[i].url[MAX_URL_LEN - 1] = '\0';

        /* pthread_create(thread_handle, attributes, function, argument)
         * This launches thread_worker in a new parallel thread.
         */
        int rc = pthread_create(&threads[i], NULL, thread_worker, &args[i]);

        if (rc != 0) {
            printf("[ERROR] Failed to create thread %d for URL: %s\n", i, urls[i]);
            args[i].success = 0;
        } else {
            printf("[MAIN] Thread %d launched for: %s\n", i, urls[i]);
        }
    }

    printf("\n[MAIN] All threads launched. Waiting for completion...\n\n");

    /* ── JOIN THREADS ──
     * pthread_join() waits for a thread to finish.
     * We MUST join each thread so the main program doesn't exit
     * before threads complete their downloads.
     */
    for (int i = 0; i < url_count; i++) {
        pthread_join(threads[i], NULL);
        printf("[MAIN] Thread %d finished.\n", i);
    }

    printf("\n[MAIN] All threads complete.\n");

    /* ── Print Results Summary ── */
    print_summary(args, url_count);

    /* ── Clean Up ── */
    free(threads);
    free(args);
    curl_global_cleanup();  // Clean up libcurl

    return 0;
}