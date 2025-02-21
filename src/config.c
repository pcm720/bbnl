#include "config.h"
#include "common.h"
#include <ps2sdkapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// When launched from mounted PFS partition
static char pfsPostfixStr[] = ":pfs:";
// When launched from partition header
static char patinfoPostfixStr[] = ":PATINFO";
static char apaPartitionPrefix[] = "hdd0:PP.";
static char bbnlCfgPrefix[] = BDM_MOUNTPOINT "/bbnl/";

// Parses the line into LauncherConfig
void parseLine(LauncherConfig *info, char *line);

// Generates config name from APA partition path
// and parses configuration file from the exFAT partition into LauncherConfig
// Returns NULL on failure
LauncherConfig *parseConfig(char *partitionPath) {
  char buf[PATH_MAX];

  // Make sure the partition path is valid
  if (strncmp(partitionPath, apaPartitionPrefix, sizeof(apaPartitionPrefix) - 1)) {
    printf("ERROR: Unsupported partition name\n");
    return NULL;
  }
  printf("Got partition path %s\n", partitionPath);

  // Check for PATINFO postfix first
  char *pathPostfix = strstr(partitionPath, patinfoPostfixStr);
  if (!pathPostfix)
    // If the path doesn't have PATINFO, check for PFS postfix
    pathPostfix = strstr(partitionPath, pfsPostfixStr);

  // Terminate the string on partition name
  if (pathPostfix)
    *pathPostfix = '\0';

  // Generate config path from partition name without the APA partition prefix
  snprintf(buf, PATH_MAX, "%s%s.cfg", bbnlCfgPrefix, partitionPath + sizeof(apaPartitionPrefix) - 1);

  printf("Loading configuration file from %s\n", buf);
  // Open the configuration file
  FILE *fd = fopen(buf, "rb");
  if (!fd) {
    printf("ERROR: Failed to open the configuration file\n");
    return NULL;
  }

  // Reinitialize buffer and initialize config
  memset(&buf, 0, PATH_MAX);
  LauncherConfig *lConfig = malloc(sizeof(LauncherConfig));

  // Set defaults
  lConfig->fileName = NULL;
  lConfig->titleID = NULL;
  lConfig->type = DISC_TYPE_NONE;
  lConfig->launcher = LAUNCHER_OPL;

  // Parse file
  while (fgets(buf, sizeof(buf), fd) != NULL) {
    parseLine(lConfig, buf);
  }

  if (!lConfig->fileName || !lConfig->titleID || lConfig->type == DISC_TYPE_NONE) {
    printf("ERROR: Invalid configuration\n");
    freeConfig(lConfig);
    lConfig = NULL;
  }

  fclose(fd);
  return lConfig;
}

// Releases memory used by config, including the passed pointer
void freeConfig(LauncherConfig *config) {
  if (!config->fileName)
    free(config->fileName);

  if (!config->titleID)
    free(config->titleID);

  free(config);
}

// Parses the line into LauncherConfig
void parseLine(LauncherConfig *info, char *line) {
  // Find argument name
  char *val = strchr(line, '=');
  if (!val) {
    printf("WARN: Value delimiter not found\n");
    return;
  }

  // Terminate argument and advance pointer to point to value
  *val = '\0';
  val++;

  // Remove newline from value
  char *newline = NULL;
  if ((newline = strchr(val, '\r')) != NULL)
    *newline = '\0';
  if ((newline = strchr(val, '\n')) != NULL)
    *newline = '\0';

  // Parse argument and value
  if (!strcmp(line, "file_name")) {
    printf("File name: %s\n", val);
    info->fileName = strdup(val);
    return;
  }
  if (!strcmp(line, "title_id")) {
    printf("Title ID: %s\n", val);
    info->titleID = strdup(val);
    return;
  }
  if (!strcmp(line, "disc_type")) {
    printf("Disc type: %s\n", val);
    if (!strcmp(val, "DVD")) {
      info->type = DISC_TYPE_DVD;
    } else if (!strcmp(val, "CD")) {
      info->type = DISC_TYPE_CD;
    } else if (!strcmp(val, "POPS")) {
      info->type = DISC_TYPE_POPS;
      info->launcher = LAUNCHER_POPS;
    }
    return;
  }
  if (!strcmp(line, "launcher")) {
    printf("Launcher: %s\n", val);
    if (!strcmp(val, "NEUTRINO")) {
      info->launcher = LAUNCHER_NEUTRINO;
    } else if (!strcmp(val, "POPS")) {
      info->launcher = LAUNCHER_POPS;
    } else { // Use OPL as default launcher
      info->launcher = LAUNCHER_OPL;
    }
    return;
  }
  printf("WARN: Unsupported argument %s\n", line);
}
