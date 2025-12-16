#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

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
  if (!f)
    die("create index");

  fclose(f);
  printf("Initialized snap repository\n");
}

int next_id(void) {
  FILE *f = fopen(INDEX_FILE, "r");
  if (!f)
    return 1;

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
           path);

  if (system(cmd) != 0) {
    printf("snap: rsync failed\n");
    return;
  }

  FILE *f = fopen(INDEX_FILE, "a");
  if (!f)
    die("open index");

  char ts[64];
  timestamp(ts, sizeof(ts));

  fprintf(f, "%s | %s | %s\n", snapname, ts, msg ? msg : "");
  fclose(f);

  printf("Saved snapshot %s\n", snapname);
}

void cmd_list(void) {
  if (!exists(INDEX_FILE)) {
    printf("snap: no snapshots\n");
    return;
  }

  FILE *f = fopen(INDEX_FILE, "r");
  if (!f)
    die("open index");

  char line[MAX_LINE];
  printf("ID     Timestamp             Message\n");
  printf("-----  --------------------  ----------------------------\n");

  while (fgets(line, sizeof(line), f)) {
    char line_copy[MAX_LINE];
    strncpy(line_copy, line, MAX_LINE);

    char *id = strtok(line_copy, "|");
    char *ts = strtok(NULL, "|");
    char *msg = strtok(NULL, "\n");

    if (!id)
      continue;
    if (!ts)
      ts = "";
    if (!msg)
      msg = "";

    // Trim leading spaces
    while (*id == ' ')
      id++;
    while (*ts == ' ')
      ts++;
    while (*msg == ' ')
      msg++;

    printf("%-5s  %-20s  %s\n", id, ts, msg);
  }

  fclose(f);
}

void cmd_restore(const char *id) {
  if (!exists(SNAP_DIR)) {
    printf("snap: not initialized\n");
    return;
  }

  char path[256];
  snprintf(path, sizeof(path), "%s/%s", SNAP_DIR, id);

  if (!exists(path)) {
    printf("snap: snapshot '%s' not found\n", id);
    return;
  }

  printf("Restoring snapshot %s...\n", id);

  char cmd[1024];
  snprintf(cmd, sizeof(cmd),
           "rsync -a --delete "
           "--exclude '.snap' --exclude '.git' "
           "%s/ ./",
           path);

  if (system(cmd) != 0) {
    printf("snap: restore failed\n");
    return;
  }

  printf("Restored snapshot %s\n", id);
}

void cmd_delete(const char *id) {
  if (!exists(SNAP_DIR)) {
    printf("snap: not initialized\n");
    return;
  }

  char path[256];
  snprintf(path, sizeof(path), "%s/%s", SNAP_DIR, id);

  if (!exists(path)) {
    printf("snap: snapshot '%s' not found\n", id);
    return;
  }

  // Delete the snapshot directory
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "rm -rf %s", path);

  if (system(cmd) != 0) {
    printf("snap: failed to delete snapshot directory\n");
    return;
  }

  // Remove entry from index
  FILE *f = fopen(INDEX_FILE, "r");
  if (!f)
    die("open index");

  FILE *tmp = fopen(INDEX_FILE ".tmp", "w");
  if (!tmp)
    die("create temp index");

  char line[MAX_LINE];
  while (fgets(line, sizeof(line), f)) {
    // Make a copy before strtok modifies it
    char line_copy[MAX_LINE];
    strncpy(line_copy, line, MAX_LINE);

    char *snap_id = strtok(line_copy, "|");

    // Trim whitespace from snap_id
    if (snap_id) {
      while (*snap_id == ' ')
        snap_id++;
    }

    // Only write the line if it's NOT the one we're deleting
    if (!snap_id || strcmp(snap_id, id) != 0) {
      fprintf(tmp, "%s", line);
    }
  }

  fclose(f);
  fclose(tmp);

  if (rename(INDEX_FILE ".tmp", INDEX_FILE) != 0)
    die("update index");

  printf("Deleted snapshot %s\n", id);
}

void cmd_status(void) {
  if (!exists(SNAP_DIR)) {
    printf("snap: not initialized\n");
    return;
  }

  FILE *f = fopen(INDEX_FILE, "r");
  if (!f) {
    printf("No snapshots\n");
    return;
  }

  int count = 0;
  char line[MAX_LINE];
  while (fgets(line, sizeof(line), f))
    count++;

  fclose(f);

  printf("Total snapshots: %d\n", count);

  // Calculate total size
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "du -sh %s 2>/dev/null | cut -f1", SNAP_DIR);

  FILE *du = popen(cmd, "r");
  if (du) {
    char size[64];
    if (fgets(size, sizeof(size), du)) {
      size[strcspn(size, "\n")] = 0;
      printf("Total size: %s\n", size);
    }
    pclose(du);
  }
}

void print_help(void) {
  printf("snap - directory snapshot manager\n\n");
  printf("Usage:\n");
  printf("  snap init              Initialize snapshot repository\n");
  printf("  snap save [message]    Save current directory state\n");
  printf("  snap list              List all snapshots\n");
  printf("  snap restore <id>      Restore a snapshot\n");
  printf("  snap delete <id>       Delete a snapshot\n");
  printf("  snap status            Show snapshot statistics\n");
  printf("  snap help              Show this help\n");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_help();
    return 1;
  }

  if (strcmp(argv[1], "init") == 0) {
    cmd_init();
  } else if (strcmp(argv[1], "save") == 0) {
    cmd_save(argc > 2 ? argv[2] : "");
  } else if (strcmp(argv[1], "list") == 0) {
    cmd_list();
  } else if (strcmp(argv[1], "restore") == 0) {
    if (argc < 3) {
      printf("snap: missing snapshot id\n");
      printf("usage: snap restore <id>\n");
      return 1;
    }
    cmd_restore(argv[2]);
  } else if (strcmp(argv[1], "delete") == 0) {
    if (argc < 3) {
      printf("snap: missing snapshot id\n");
      printf("usage: snap delete <id>\n");
      return 1;
    }
    cmd_delete(argv[2]);
  } else if (strcmp(argv[1], "status") == 0) {
    cmd_status();
  } else if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0) {
    print_help();
  } else {
    printf("snap: unknown command '%s'\n", argv[1]);
    print_help();
    return 1;
  }

  return 0;
}