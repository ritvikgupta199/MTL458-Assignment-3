#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define INIT_TRACE_LEN 32
#define RANDOM_SEED 5635

enum STRATEGIES {
    OPT, FIFO, CLOCK, LRU, RANDOM,
};
struct TraceEntry {
    int pfn;
    char rw;
};
struct PageTableEntry{
    int pfn;
    int entry_time, last_access_time;
    bool valid, free, dirty;
};

struct TraceEntry* trace;
struct PageTableEntry* pages;
int trace_len = -1, pages_len = -1, strategy = -1;
int time = 0;
bool is_verbose = false;


void setup(int argc, char** argv);
void read_file(char* filename);
char** get_tokens(char* line);
int get_strategy(char* strategy);
void process_memory_access(int pfn, char rw, int idx);

void add_page(int pfn, bool dirty);
int get_free_idx();
int get_pfn_idx(int pfn);
int evict_page(int pfn);
void print_verbose(int add_pfn, int evict_pfn, bool evict_dirty);

int opt_evict();
int fifo_evict();
int clock_evict();
int lru_evict();
int random_evict();

// args of format ./frame <input_file> <n> <strategy>
int main(int argc, char** argv) {
    setup(argc, argv);

    char* filename = argv[1];
    read_file(filename);

    for (int i = 0; i < trace_len; i++) {
        // printf("%d ** %c\n", trace[i].pfn, trace[i].rw);
        process_memory_access(trace[i].pfn, trace[i].rw, i);
        time++;
    }
}

void process_memory_access(int pfn, char rw, int idx) {
    int pfn_idx = get_pfn_idx(pfn);
    if (rw == 'R') {
        if (pfn_idx == -1) {
            add_page(pfn, false);
        }
    } else {
        if (pfn_idx == -1) {
            add_page(pfn, true);
        } else {
            pages[pfn_idx].dirty = true;
        }
    }
    pages[pfn_idx].last_access_time = time;
}

void add_page(int pfn, bool dirty) {
    int free_idx = get_free_idx();
    // printf("pfn: %d free_idx: %d\n", pfn, free_idx);
    if (free_idx == -1) {
        // Page Fault
        // Evict Page
        free_idx = evict_page(pfn);
    }
    pages[free_idx].pfn = pfn;
    pages[free_idx].valid = true;
    pages[free_idx].dirty = dirty;
    pages[free_idx].free = false;
    pages[free_idx].entry_time = time;
}

int evict_page(int pfn) {
    int evict_idx = -1;
    if (strategy == OPT) {
        evict_idx = opt_evict();
    } else if (strategy == FIFO) {
        evict_idx = fifo_evict();
    } else if (strategy == CLOCK) {
        evict_idx = clock_evict();
    } else if (strategy == LRU) {
        evict_idx = lru_evict();
    } else if (strategy == RANDOM) {
        evict_idx = random_evict();
    } else {
        printf("Invalid Strategy\n");
        exit(1);
    }
    if (is_verbose) {
        print_verbose(pfn, pages[evict_idx].pfn, pages[evict_idx].dirty);
    }
    pages[evict_idx].free = true;
    return evict_idx;
}

void print_verbose(int add_pfn, int evict_pfn, bool evict_dirty) {
    if (evict_dirty) {
        printf("Page %d was read from disk, page %d was written to the disk.\n", add_pfn, evict_pfn);
    } else {
        printf("Page %d was read from disk, page %d was dropped (it was not dirty).\n", add_pfn, evict_pfn);
    }
}

int opt_evict(){
    exit(-1);
}

int fifo_evict(){
    int min_time = time, min_idx = -1;
    for (int i = 0; i < pages_len; i++) {
        if (pages[i].valid && pages[i].entry_time < min_time) {
            min_time = pages[i].entry_time;
            min_idx = i;
        }
    }
    return min_idx;
}

int clock_evict(){
    exit(-1);
}

int lru_evict() {
    int min_time = time, min_idx = -1;
    for (int i = 0; i < pages_len; i++) {
        if (pages[i].valid && pages[i].last_access_time < min_time) {
            min_time = pages[i].last_access_time;
            min_idx = i;
        }
    }
    return min_idx;
}

int random_evict(){
    int rand_idx = rand() % pages_len;
    while (pages[rand_idx].free) {
        rand_idx = rand() % pages_len;
    }
    return rand_idx;
}

int get_free_idx() {
    for (int i = 0; i < pages_len; i++) {
        if (pages[i].free)
            return i;
    }
    return -1;
}

int get_pfn_idx(pfn) {
    for (int i = 0; i < pages_len; i++) {
        if (pages[i].pfn == pfn && !pages[i].free)
            return i;
    }
    return -1;
}

void setup(int argc, char** argv){
    srand(RANDOM_SEED);
    if (argc != 4) {
        if (argc == 5) {
            is_verbose = (strcmp(argv[4], "-verbose") == 0);
        } else {
            printf("Usage: ./frames <input_file> <n> <strategy>\n");
        }
    }
    pages_len = atoi(argv[2]);
    pages = malloc(sizeof(struct PageTableEntry) * pages_len);
    for (int i = 0; i < pages_len; i++) {
        pages[i].free = true;
    }
    strategy = get_strategy(argv[3]);
}

void read_file(char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("File does not exist");
        exit(1);
    }
    char line[1024];
    int i = 0, t_size = INIT_TRACE_LEN;
    trace = (struct TraceEntry*)malloc(sizeof(struct TraceEntry) * INIT_TRACE_LEN);
    while (fgets(line, 1024, file) != NULL) {
        char** tokens = get_tokens(line);
        int pfn = (int) strtol(tokens[0], NULL, 16);
        pfn = pfn >> 12;
        char rw = tokens[1][0];
        if (i == t_size - 1) {
            t_size += INIT_TRACE_LEN;
            tokens = realloc(tokens, t_size * sizeof(struct TraceEntry));
        }
        trace[i].pfn = pfn;
        trace[i++].rw = rw;
    }
    trace_len = i;
    fclose(file);
}

char** get_tokens(char* input_str){
    char** tokens = malloc(2 * sizeof(char*));
    int i = 0;
    char* p = strtok(input_str, " ");
    tokens[0] = p;
    p = strtok(NULL, " ");
    tokens[1] = p;
    return tokens;
}

int get_strategy(char* strategy) {
    if (strcmp(strategy, "OPT") == 0)
        return OPT;
    else if (strcmp(strategy, "FIFO") == 0)
        return FIFO;
    else if (strcmp(strategy, "CLOCK") == 0)
        return CLOCK;
    else if (strcmp(strategy, "LRU") == 0)
        return LRU;
    else if (strcmp(strategy, "RANDOM") == 0)
        return RANDOM;
    else {
        printf("Invalid strategy");
        exit(1);
    }
}