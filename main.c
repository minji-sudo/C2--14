#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_SIZES   6
#define NUM_ALGOS   5
#define REPEAT      20       
#define VALUE_RANGE 100000   
#define SEED        2024      


static long long g_cmp = 0;  
static int g_depth = 0;     
static int g_maxDepth = 0;   


#define FRAME_OVERHEAD (2 * sizeof(void *))


#define FRAME_QUICK     (sizeof(int *) + 3 * sizeof(int) /* lo, hi, mode */ + sizeof(int) /* p */ + FRAME_OVERHEAD)
#define FRAME_PARTITION (sizeof(int *) + 2 * sizeof(int) /* lo, hi */ + 4 * sizeof(int) /* pivot, i, j, tmp */ + FRAME_OVERHEAD)
#define FRAME_MSORT     (2 * sizeof(int *) + 2 * sizeof(int) /* lo, hi */ + sizeof(int) /* mid */ + FRAME_OVERHEAD)
#define FRAME_MERGE     (2 * sizeof(int *) + 3 * sizeof(int) /* lo, mid, hi */ + 3 * sizeof(int) /* i, j, k */ + FRAME_OVERHEAD)
#define FRAME_HEAPSORT  (sizeof(int *) + sizeof(int) /* n */ + 2 * sizeof(int) /* i, end */ + FRAME_OVERHEAD)
#define FRAME_SIFT      (sizeof(int *) + 2 * sizeof(int) /* n, i */ + 3 * sizeof(int) /* child, hold, parent */ + FRAME_OVERHEAD)


static void swap_int(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}


static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static int is_sorted(const int *a, int n)
{
    for (int i = 1; i < n; i++)
        if (a[i - 1] > a[i])
            return 0;
    return 1;
}


static void sift_down(int *a, int n, int i)
{
    int hold = a[i];
    int child;

    while ((child = 2 * i + 1) < n) {

        if (child + 1 < n) {
            g_cmp++;
            if (a[child] < a[child + 1])
                child++;
        }
    
        g_cmp++;
        if (hold >= a[child])
            break;
        a[i] = a[child];
        i = child;
    }
    a[i] = hold;
}

static void heap_sort(int *a, int n)
{
  
    for (int i = n / 2 - 1; i >= 0; i--)
        sift_down(a, n, i);

   
    for (int end = n - 1; end > 0; end--) {
        swap_int(&a[0], &a[end]);
        sift_down(a, end, 0);
    }
}

/* ---------------- 2. 병합 정렬 ---------------- */
static void merge(int *a, int *tmp, int lo, int mid, int hi)
{
    int i = lo, j = mid + 1, k = lo;

    while (i <= mid && j <= hi) {
        g_cmp++;
        if (a[i] <= a[j])
            tmp[k++] = a[i++];
        else
            tmp[k++] = a[j++];
    }
    while (i <= mid) tmp[k++] = a[i++];
    while (j <= hi)  tmp[k++] = a[j++];

    for (k = lo; k <= hi; k++)
        a[k] = tmp[k];
}

static void merge_sort_rec(int *a, int *tmp, int lo, int hi)
{
    g_depth++;
    if (g_depth > g_maxDepth) g_maxDepth = g_depth;

    if (lo < hi) {
        int mid = (lo + hi) / 2;
        merge_sort_rec(a, tmp, lo, mid);
        merge_sort_rec(a, tmp, mid + 1, hi);
        merge(a, tmp, lo, mid, hi);
    }
    g_depth--;
}

/* ---------------- 3. 퀵 정렬 ---------------- */
enum { PIVOT_FIRST = 0, PIVOT_RANDOM = 1, PIVOT_MEDIAN3 = 2 };


static void choose_pivot(int *a, int lo, int hi, int mode)
{
    if (mode == PIVOT_FIRST) {
        swap_int(&a[lo], &a[hi]);
    } else if (mode == PIVOT_RANDOM) {
        int r = lo + rand() % (hi - lo + 1);
        swap_int(&a[r], &a[hi]);
    } else {
        int mid = (lo + hi) / 2;
        g_cmp++; if (a[lo] > a[mid]) swap_int(&a[lo], &a[mid]);
        g_cmp++; if (a[lo] > a[hi])  swap_int(&a[lo], &a[hi]);
        g_cmp++; if (a[mid] > a[hi]) swap_int(&a[mid], &a[hi]);
        swap_int(&a[mid], &a[hi]);
    }
}

static int partition(int *a, int lo, int hi, int mode)
{
    choose_pivot(a, lo, hi, mode);
    int pivot = a[hi];
    int i = lo - 1;

    for (int j = lo; j < hi; j++) {
        g_cmp++;
        if (a[j] <= pivot) {
            i++;
            swap_int(&a[i], &a[j]);
        }
    }
    swap_int(&a[i + 1], &a[hi]);
    return i + 1;
}

static void quick_sort_rec(int *a, int lo, int hi, int mode)
{
    g_depth++;
    if (g_depth > g_maxDepth) g_maxDepth = g_depth;

    if (lo < hi) {
        int p = partition(a, lo, hi, mode);
        quick_sort_rec(a, lo, p - 1, mode);
        quick_sort_rec(a, p + 1, hi, mode);
    }
    g_depth--;
}

/* ---------------- 알고리즘 실행  ---------------- */

static size_t run_algo(int algo, int *a, int n)
{
    g_depth = 0;
    g_maxDepth = 0;

    switch (algo) {
    case 0: 
        heap_sort(a, n);
        return FRAME_HEAPSORT + FRAME_SIFT;

    case 1: { 
        int *tmp = (int *)malloc(sizeof(int) * n);
        if (tmp == NULL) {
            fprintf(stderr, "메모리 할당 실패\n");
            exit(1);
        }
        merge_sort_rec(a, tmp, 0, n - 1);
        free(tmp);
        return sizeof(int) * (size_t)n + (size_t)g_maxDepth * FRAME_MSORT + FRAME_MERGE;
    }
    default: { 
        int mode = algo - 2;
        quick_sort_rec(a, 0, n - 1, mode);
        return (size_t)g_maxDepth * FRAME_QUICK + FRAME_PARTITION;
    }
    }
}

int main(void)
{
    const int sizes[NUM_SIZES] = {1000, 2000, 4000, 8000, 16000, 20000};
    const char *names[NUM_ALGOS] = {
        "Heap Sort", "Merge Sort", "Quick (First)", "Quick (Random)", "Quick (Median3)"
    };
    int allOk = 1;

    srand(SEED);

    for (int s = 0; s < NUM_SIZES; s++) {
        int n = sizes[s];
        int *origin = (int *)malloc(sizeof(int) * n); 
        int *work   = (int *)malloc(sizeof(int) * n); 
        if (origin == NULL || work == NULL) {
            fprintf(stderr, "메모리 할당 실패\n");
            return 1;
        }

     
        for (int i = 0; i < n; i++)
            origin[i] = rand() % VALUE_RANGE;

        printf("Data Size : %d\n\n", n);
        printf("%-16s%-15s%-12s%s\n", "Algorithm", "Comparisons", "Time(ms)", "Memory(Bytes)");
        printf("--------------------------------------------------------\n");

        for (int algo = 0; algo < NUM_ALGOS; algo++) {
            long long cmpTotal = 0;  
            size_t mem = 0;         
            double total = 0.0;

            for (int r = 0; r < REPEAT; r++) {
                memcpy(work, origin, sizeof(int) * n);  
                g_cmp = 0;

                double t0 = now_ms();
                size_t m = run_algo(algo, work, n);
                double t1 = now_ms();

                total += (t1 - t0);
                cmpTotal += g_cmp;
                if (m > mem) mem = m;
                if (!is_sorted(work, n)) {
                    allOk = 0;
                    printf("[오류] %s 정렬 결과가 올바르지 않음 (n=%d)\n", names[algo], n);
                }
            }

      
            long long cmp = (cmpTotal + REPEAT / 2) / REPEAT;
            printf("%-16s%-15lld%-12.4f%zu\n", names[algo], cmp, total / REPEAT, mem);
        }
        printf("\n");

        free(origin);
        free(work);
    }

    if (allOk)
        printf("모든 정렬 결과가 오름차순으로 올바르게 정렬되었습니다.\n");

    return 0;
}
