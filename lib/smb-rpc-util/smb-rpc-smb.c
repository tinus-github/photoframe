//
//  smb-rpc-smb.c
//  Photoframe
//
//  Created by Martijn Vernooij on 29/04/2017.
//
//

#include "smb-rpc-smb.h"
#include "smb-rpc-auth.h"
#include <libsmbclient.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define SMB_RPC_DEBUG

struct smb_rpc_smb_data {
	SMBCCTX *smb_context;
	smb_rpc_dirent dirent;
};

static smb_rpc_smb_data *active_smb_data = NULL;
static int smb_rpc_inited = 0;

static void activate_smb_data(smb_rpc_smb_data *d)
{
	if (active_smb_data != d) {
		smbc_set_context(d->smb_context);
		active_smb_data = d;
	}
}

smb_rpc_smb_data *smb_rpc_smb_new_data()
{
	smb_rpc_smb_data *ret = calloc(1, sizeof(smb_rpc_smb_data));
	
	ret->smb_context = smbc_new_context();
	assert (ret->smb_context);
	
	errno = 0;
	if (!smbc_init_context(ret->smb_context)) {
		fprintf(stderr, "Failed to init smb context, errno=%d\n", errno);
		abort();
	}
	
	if (!smb_rpc_inited) {
		smbc_set_context(ret->smb_context);
		active_smb_data = ret;

		errno = 0;
		if (smbc_init(NULL, 0)) {
			fprintf(stderr, "Failed to init smb, errno=%d\n", errno);
			abort();
		}
		smbc_setFunctionAuthData(ret->smb_context, smb_rpc_auth_get_auth_fn);
		smbc_setOptionDebugToStderr(ret->smb_context, 1);
#ifdef SMB_RPC_DEBUG
		smbc_setDebug(ret->smb_context, 6);
#endif
		smb_rpc_inited = 1;
	}
	
	return ret;
}

void smb_rpc_smb_free_data(smb_rpc_smb_data *data)
{
	if (data == active_smb_data) {
		smbc_set_context(NULL);
		active_smb_data = NULL;
	}
	if (data->smb_context) {
		errno = 0;
		if (smbc_free_context(data->smb_context, 0)) {
			fprintf(stderr, "Failed to close context, errno=%d\n", errno);
			abort();
		}
		data->smb_context = NULL;
	}
	free(data);
}

int smb_rpc_open_file(smb_rpc_smb_data *data, const char *url)
{
	activate_smb_data(data);

	errno = 0;
	int ret = smbc_open(url, O_RDONLY, 0);
	
	if (ret >= 0) {
		errno = 0;
	}
	
	return ret;
}

off_t smb_rpc_seek_file(smb_rpc_smb_data *data, int fd, off_t offset, int whence)
{
	activate_smb_data(data);
	
	errno = 0;
	off_t ret = smbc_lseek(fd, offset, whence);
	
	if (ret != -1) {
		errno = 0;
	}
	
	return ret;
}

// can only fail due to programming errors
int smb_rpc_close_file(smb_rpc_smb_data *data, int fd)
{
	activate_smb_data(data);

	errno = 0;
	int ret = smbc_close(fd);
	if (ret) {
		fprintf(stderr, "Failed to close file, errno=%d\n", errno);
	} else {
		errno = 0;
	}
	return ret;
}

ssize_t smb_rpc_read_file(smb_rpc_smb_data *data, int fd, void *buf, size_t bufsize)
{
	activate_smb_data(data);
	
	errno = 0;
	ssize_t ret = smbc_read(fd, buf, bufsize);
	
	if (ret >= 0) {
		errno = 0;
	}
	return ret;
}

int smb_rpc_open_dir(smb_rpc_smb_data *data, const char *url)
{
	activate_smb_data(data);
	
	errno = 0;
	int ret = smbc_opendir(url);
	
	if (ret >= 0) {
		errno = 0;
	}
	
	return ret;
}

void smb_rpc_close_dir(smb_rpc_smb_data *data, int fd)
{
	activate_smb_data(data);
	
	errno = 0;
	
	if (smbc_closedir(fd)) {
		fprintf(stderr, "Failed to close directory, errno=%d\n", errno);
		abort();
	}
	errno = 0;
}

const smb_rpc_dirent *smb_rpc_read_dir(smb_rpc_smb_data *data, int fd)
{
	activate_smb_data(data);
	
	errno = 0;
	struct smbc_dirent *smbc_d = smbc_readdir(fd);
	
	if (!smbc_d) {
		return NULL;
	}
	
	switch (smbc_d->smbc_type) {
		case SMBC_FILE:
			data->dirent.type = smb_rpc_dirent_type_file;
			break;
		default:
			data->dirent.type = smb_rpc_dirent_type_dir;
			break;
	}
	
	data->dirent.name = smbc_d->name;
	data->dirent.namelen = smbc_d->namelen;

	errno = 0;
	return &data->dirent;
}