#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define SNAP_DIR ".snap"
#define INDEX_FILE ".snap/index.txt"
#define MAX_LINE 512

void die(const char *msg) {
    perror(msg);
    exit(1);
}

int exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

void cmd_init(void) {
    if (exists(SNAP_DIR)) {
        printf("snap: already initialized\n");
        return;
    }

    if (mkdir(SNAP_DIR, 0755) != 0)
        die("mkdir .snap");

    FILE *f = fopen(INDEX_FILE, "w");
    if (!f) die("create index");

    fclose(f);
    printf("Initialized snap repository\n");
}

int next_id(void) {
    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) return 1;

    char line[MAX_LINE];
    int id = 0;

    while (fgets(line, sizeof(line), f))
        id++;

    fclose(f);
    return id + 1;
}

void timestamp(char *buf, size_t size) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm);
}

void cmd_save(const char *msg) {
    if (!exists(SNAP_DIR)) {
        printf("snap: not initialized (run `snap init`)\n");
        return;
    }

    int id = next_id();
    char snapname[128];
    snprintf(snapname, sizeof(snapname), "%04d", id);

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", SNAP_DIR, snapname);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "rsync -a --delete "
        "--exclude '.snap' --exclude '.git' "
        "./ %s",
        path
    );

    if (system(cmd) != 0) {
        printf("snap: rsync failed\n");
        return;
    }

    FILE *f = fopen(INDEX_FILE, "a");
    if (!f) die("open index");

    char ts[64];
    timestamp(ts, sizeof(ts));

    fprintf(f, "%s | %s\n", snapname, msg ? msg : "");
    fclose(f);

    printf("Saved snapshot %s\n", snapname);
}

void cmd_list(void) {
    if (!exists(INDEX_FILE)) {
        printf("snap: no snapshots\n");
        return;
    }

    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) die("open index");

    char line[MAX_LINE];
    printf("ID     Message\n");
    printf("-----  ----------------------------\n");

    while (fgets(line, sizeof(line), f)) {
        char *id = strtok(line, "|");
        char *msg = strtok(NULL, "\n");

        if (!msg) msg = "";
        printf("%-5s  %s\n", id, msg + 1);
    }

    fclose(f);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: snap <init|save|list> [message]\n");
        return 1;
    }

    if (strcmp(argv[1], "init") == 0) {
        cmd_init();
    } else if (strcmp(argv[1], "save") == 0) {
        cmd_save(argc > 2 ? argv[2] : "");
    } else if (strcmp(argv[1], "list") == 0) {
        cmd_list();
    } else {
        printf("snap: unknown command '%s'\n", argv[1]);
    }

    return 0;
}
