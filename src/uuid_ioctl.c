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

struct fsuuid {
	__u32   fu_len;
	__u32   fu_flags;
	__u8    fu_uuid[];
};

#define EXT4_IOC_SETFSUUID_FLAG_BLOCKING 0x1

#ifndef EXT4_IOC_GETFSUUID
#define EXT4_IOC_GETFSUUID      _IOR('f', 44, struct fsuuid)
#endif

#ifndef EXT4_IOC_SETFSUUID
#define EXT4_IOC_SETFSUUID      _IOW('f', 44, struct fsuuid)
#endif

int main(int argc, char **argv)
{
	int error, fd;
	struct fsuuid *fsuuid = NULL;

	if (argc < 3) {
		fprintf(stderr, "Invalid arguments\n");
		return 1;
	}

	fd = open(argv[2], O_RDONLY);
	if (!fd) {
		perror(argv[2]);
		return 1;
	}

	fsuuid = malloc(sizeof(*fsuuid) + sizeof(uuid_t));
	if (!fsuuid) {
		perror("malloc");
		return 1;
	}
	fsuuid->fu_len = sizeof(uuid_t);

	if (strcmp(argv[1], "get") == 0) {
		uuid_t uuid;
		char uuid_str[37];

		if (ioctl(fd, EXT4_IOC_GETFSUUID, fsuuid)) {
			fprintf(stderr, "%s while trying to get fs uuid\n",
				strerror(errno));
			return 1;
		}

		memcpy(&uuid, fsuuid->fu_uuid, sizeof(uuid));
		uuid_unparse(uuid, uuid_str);
		printf("%s", uuid_str);
	} else if (strcmp(argv[1], "set") == 0) {
		uuid_t uuid;

		if (argc != 4) {
			fprintf(stderr, "UUID argument missing.\n");
			return 1;
		}

		error = uuid_parse(argv[3], uuid);
		if (error < 0) {
			fprintf(stderr, "Invalid UUID. The UUID should be in "
				"canonical format. Example: "
				"8c628557-6987-42b2-ba16-b7cc79ddfb43\n");
			return 1;
		}

		memcpy(&fsuuid->fu_uuid, uuid, sizeof(uuid));
		fsuuid->fu_flags = EXT4_IOC_SETFSUUID_FLAG_BLOCKING;
		if (ioctl(fd, EXT4_IOC_SETFSUUID, fsuuid)) {
			fprintf(stderr, "%s while trying to set fs uuid\n",
				strerror(errno));
			return 1;
		}
	} else {
		fprintf(stderr, "Invalid command\n");
		return 1;
	}

	return 0;
}
