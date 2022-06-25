// SPDX-License-Identifier: GPL-2.0

/*
 * Test program which uses the raw ext4 set_fsuuid ioctl directly.
 * SYNOPSIS:
 *   $0 COMMAND MOUNT_POINT [UUID]
 *
 * COMMAND must be either "get" or "set".
 * The UUID must be a 16 octet represented as 32 hexadecimal digits in canonical
 * textual representation, e.g. output from `uuidgen`.
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <uuid/uuid.h>
#include <linux/fs.h>
#include "global.h"

struct fsuuid {
        size_t len;
        __u8 *b;
};

#ifndef EXT4_IOC_GETFSUUID
#define EXT4_IOC_GETFSUUID      _IOR('f', 44, struct fsuuid)
#endif

#ifndef EXT4_IOC_SETFSUUID
#define EXT4_IOC_SETFSUUID      _IOW('f', 45, struct fsuuid)
#endif

int main(int argc, char **argv)
{
	int	error, fd;
        struct fsuuid fsuuid;

	if (argc < 3) {
		fprintf(stderr, "Invalid arguments\n");
		return 1;
	}

	fd = open(argv[2], O_RDONLY);
	if (!fd) {
		perror(argv[2]);
		return 1;
	}

	BUILD_BUG_ON(sizeof(uuid_t) % 16);
	fsuuid.len = 16;
	fsuuid.b = calloc(fsuuid.len, sizeof(__u8));

	if (strcmp(argv[1], "get") == 0) {
		uuid_t uuid;
		char uuid_str[37];

		if (ioctl(fd, EXT4_IOC_GETFSUUID, &fsuuid)) {
			close(fd);
			fprintf(stderr, "%s while trying to get fs uuid\n",
				strerror(errno));
			return 1;
		}

		memcpy(&uuid, fsuuid.b, sizeof(uuid));
		uuid_unparse(uuid, uuid_str);
		printf("%s", uuid_str);
	} else if (strcmp(argv[1], "set") == 0) {
		if (argc != 4) {
			fprintf(stderr, "UUID argument missing.\n");
			return 1;
		}

		if (strlen(argv[3]) != 36) {
			fprintf(stderr, "Invalid UUID. The UUID should be in "
				"canonical format. Example: "
				"8c628557-6987-42b2-ba16-b7cc79ddfb43\n");
			return 1;
		}

		uuid_t uuid;
		error = uuid_parse(argv[3], uuid);
		if (error < 0) {
			fprintf(stderr, "%s: invalid UUID.\n", argv[0]);
			return 1;
		}

		memcpy(fsuuid.b, uuid, sizeof(uuid));
		if(ioctl(fd, EXT4_IOC_SETFSUUID, &fsuuid)) {
			close(fd);
			fprintf(stderr, "%s while trying to set fs uuid\n",
				strerror(errno));
			return 1;
		}
	} else {
		fprintf(stderr, "Invalid command\n");
	}

	close(fd);
	return 0;
}
