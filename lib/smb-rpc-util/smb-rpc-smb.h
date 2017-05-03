//
//  smb-rpc-smb.h
//  Photoframe
//
//  Created by Martijn Vernooij on 29/04/2017.
//
//

#ifndef smb_rpc_smb_h
#define smb_rpc_smb_h

#include <stdio.h>

typedef struct smb_rpc_smb_data smb_rpc_smb_data;

typedef enum {
	smb_rpc_dirent_type_file = 1,
	smb_rpc_dirent_type_dir = 2,
} smb_rpc_dirent_type;

typedef struct smb_rpc_dirent {
	smb_rpc_dirent_type type;
	char *name;
	
	// excluding the terminating \0
	size_t namelen;
} smb_rpc_dirent;

smb_rpc_smb_data *smb_rpc_smb_new_data();
// Must make sure to close everything associaled with this context,
// otherwise freeing it will fail
void smb_rpc_smb_free_data(smb_rpc_smb_data *data);

#endif /* smb_rpc_smb_h */
