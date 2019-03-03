#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

// Longest word in the provided dictionary is actually 31 characters, but let's have a space for \0.
#define MAX_WORD_LENGTH 32

// Dictionary contains upto 6 anagrams of any given word.
// Let's allow some extra space just in case.
#define MAX_ANAGRAMS 6
#define MAX_RESULT_LENGTH 10 * MAX_WORD_LENGTH * MAX_ANAGRAMS

#define MAX_CHAR 255

// Equivalent to tolower, but occasionally had better performance during benchmarking.
unsigned char to_lower[256] = {
          0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
         20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,
         40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
         60,  61,  62, 63, 64, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
        113, 114, 115, 116, 117, 118, 119, 120, 121, 122,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100,
        101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
        121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
        141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
        161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
        181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 228, 197, 198, 199, 200,
        233, 202, 203, 204, 205, 206, 207, 240, 209, 210, 211, 212, 245, 246, 215, 216, 217, 218, 219, 252,
        221, 254, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
        241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255 };

const unsigned long long MODULO = ((unsigned long long) 1 << 50) - 1;

unsigned long long primes[256] = {
        2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107,
        109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
        233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359,
        367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491,
        499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641,
        643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787,
        797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911, 919, 929, 937, 941,
        947, 953, 967, 971, 977, 983, 991, 997, 1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069,
        1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213,
        1217, 1223, 1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291, 1297, 1301, 1303, 1307, 1319, 1321,
        1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459, 1471, 1481,
        1483, 1487, 1489, 1493, 1499, 1511, 1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, 1597, 1601,
        1607, 1609, 1613, 1619};

char g_query_max_char_counts[MAX_CHAR + 1] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1,
    0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 3, 3, 4, 6, 4, 3, 6, 8, 3, 5,
    5, 5, 5, 7, 4, 1, 4, 7, 6, 7, 4, 2, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 3, 0, 0,
    0, 0, 4, 4, 0, 0, 0, 0, 0, 4, 0, 2, 0 };

// Class variables were allowed. Thus allocate bigger arrays globally.
char g_anagrams[MAX_RESULT_LENGTH];
char g_query[2 * MAX_WORD_LENGTH];
long g_query_char_counts[MAX_CHAR + 1];


// Command line probably gives args in UTF8. We only care about "õÕäÄöÖüÜžŽšŠéÉ" which we'll need to fix, because they
// are only non-ASCII letters in the dictionary file.
unsigned char* fix_utf8(char* ch) {
    unsigned char* result = &g_query[0];
    while (*ch != '\0') {
        unsigned char uch = *ch;
        if (uch == 195) {
            uch = (unsigned char) *++ch;
            switch(uch) {
                case 181:   // õ
                    *result++ = 245;
                    break;
                case 149:   // Õ
                    *result++ = 213;
                    break;
                case 164:   // ä
                    *result++ = 228;
                    break;
                case 132:   // Ä
                    *result++ = 196;
                    break;
                case 182:   // ö
                    *result++ = 246;
                    break;
                case 150:   // Ö
                    *result++ = 214;
                    break;
                case 188:   // ü
                    *result++ = 252;
                    break;
                case 156:   // Ü
                    *result++ = 220;
                    break;
                case 169:   // é
                    *result++ = 233;
                    break;
                case 137:   // É
                    *result++ = 201;
                    break;
                default:
                    *result++ = 195;
                    *result++ = uch;
            }
        } else if (uch == 197) {
            uch = (unsigned char) *++ch;
            switch(uch) {
                case 190:   // ž
                    *result++ = 254;
                    break;
                case 189:   // Ž
                    *result++ = 222;
                    break;
                case 161:   // š
                    *result++ = 240;
                    break;
                case 160:   // Š
                    *result++ = 208;
                    break;
                default:
                    *result++ = 197;
                    *result++ = uch;
            }
        } else
            *result++ = *ch;
        ch++;
    }
    *result = '\0';
    return &g_query[0];
}

double timediff_us(struct timespec t0, struct timespec t) {
    double d_sec = (t.tv_sec - t0.tv_sec) * 1000000.0;
    double d_nsec = (t.tv_nsec - t0.tv_nsec) / 1000.0;
    return d_sec + d_nsec;
}

unsigned char* to_next_word(unsigned char* ch) {
    while (*ch++ != '\n');
    return ch;
}

char* find_first(char* start, char* end, unsigned char key) {
    unsigned char* lo = start;
    unsigned char* hi = end;
    char* result = start;
    while (lo + MAX_WORD_LENGTH <= hi - MAX_WORD_LENGTH) {
        unsigned char* mi = (hi - lo) / 2 + lo;
        unsigned char* ch = to_next_word(mi);
        if (*ch < key) {
            lo = ch + 1;
            result = ch;
        } else
            hi = ch - 1;
    }
    return result;
}

char* find_last(char* start, char* end, unsigned char key) {
    unsigned char* lo = start;
    unsigned char* hi = end;
    char* result = end;
    while (lo + MAX_WORD_LENGTH <= hi - MAX_WORD_LENGTH) {
        unsigned char* mi = (hi - lo) / 2 + lo;
        unsigned char* ch = to_next_word(mi);
        if (*ch <= key)
            lo = ch + 1;
        else {
            hi = ch - 1;
            result = ch;
        }
    }
    return result;
}

int main(int argc, char* argv[]) {
    struct timespec start;
    clock_gettime(CLOCK_REALTIME, &start);

    int infile = open(argv[1], O_RDONLY);
    struct stat fileStat;
    if (fstat(infile, &fileStat) < 0) {
        printf("Failed to read dictionary.\n");
        return 1;
    }
    long buffer_len = fileStat.st_size;
    unsigned char* buffer = (unsigned char*) mmap(0, buffer_len, PROT_READ, MAP_PRIVATE, infile, 0);

    char* anach = &g_anagrams[0];

    unsigned char* query = fix_utf8(argv[2]);
    unsigned long long query_hash = 1;
    int too_many_chars = 0;
    unsigned char query_minch = 255, query_maxch = 0;
    for (unsigned char* ch = query; *ch != '\0'; ch++) {
        unsigned char uch = to_lower[*ch];
        query_hash = (query_hash * primes[uch]) & MODULO;
        if (query_maxch < uch)
            query_maxch = uch;
        if (query_minch > uch)
            query_minch = uch;
        g_query_char_counts[uch]++;
        if (--g_query_max_char_counts[uch] < 0) {
            too_many_chars = 1;
            break;
        }
    }

    if (too_many_chars == 0) {
        char anagram_found = 0;
        char can_find_anagram = 1;
        char* buffer_end = buffer + buffer_len;
        unsigned long long word_hash = 0;
        char* ch = buffer;
        for (; ch < buffer_end; ch += 2) {
            if (*ch >= 'a')
                break;
            char* word = ch;
            word_hash = 1;
            do {
                unsigned char uch = to_lower[(unsigned char) *ch];
                word_hash = (word_hash * primes[uch]) & MODULO;
            } while (*++ch != '\r');

            if (word_hash == query_hash) {
                if (anagram_found == 0) {
                    char* ach = anach;
                    *ach++ = ',';
                    while (word < ch) {
                        g_query_char_counts[(unsigned char) *word]--;
                        if (g_query_char_counts[(unsigned char) *word] < 0) {
                            can_find_anagram = 0;
                            break;
                        }
                        *ach++ = *word++;
                    }
                    if (can_find_anagram == 0)
                        break;
                    anagram_found = 1;
                    anach = ach;
                } else {
                    *anach++ = ',';
                    while (word < ch)
                        *anach++ = *word++;
                }
            }
        }

        if (can_find_anagram > 0) {
            char* search_start = find_first(ch, buffer_end, query_minch);
            char* search_end = find_last(ch, buffer_end, query_maxch);
            for (char* ch = search_start; ch < search_end; ch += 2) {
                char* word = ch;
                word_hash = 1;
                do {
                    unsigned char uch = to_lower[(unsigned char) *ch];
                    word_hash = (word_hash * primes[uch]) & MODULO;
                } while (*++ch != '\r');

                if (word_hash == query_hash) {
                    if (anagram_found == 0) {
                        char* ach = anach;
                        *ach++ = ',';
                        while (word < ch) {
                            g_query_char_counts[(unsigned char) *word]--;
                            if (g_query_char_counts[(unsigned char) *word] < 0) {
                                can_find_anagram = 0;
                                break;
                            }
                            *ach++ = *word++;
                        }
                        if (can_find_anagram == 0)
                            break;
                        anagram_found = 1;
                        anach = ach;
                    } else {
                        *anach++ = ',';
                        while (word < ch)
                            *anach++ = *word++;
                    }
                }
            }
        }
    }

    *anach = '\0';
    struct timespec finish;
    clock_gettime(CLOCK_REALTIME, &finish);
    printf("%.2f%s\n", timediff_us(start, finish), &g_anagrams[0]);
    return 0;
}
