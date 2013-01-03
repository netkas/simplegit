#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <git2.h>
#include "errors.h"
#include "git-checkout.h"
#include "git-support.h"

static int conflict_cb(const char *conflicting_path, const git_oid *index_oid, unsigned int index_mode, unsigned int wd_mode, void *payload)
{
	fprintf(stderr,"Conflict in %s\n",conflicting_path);
	return 1;
}

int cmd_checkout(git_repository *repo, int argc, char **argv)
{
	int i, err, rc;
	char *branch;
	git_reference *branch_ref;
	git_checkout_opts checkout_opts;

	branch = NULL;
	rc = EXIT_FAILURE;

	for (i=1;i<argc;i++)
	{
		if (argv[i][0] != '-')
		{
			if (!branch) branch = argv[i];
		} else
		{
		}
	}

	/* Try local branch */
	if (git_branch_lookup(&branch_ref,repo,branch,GIT_BRANCH_LOCAL) != 0)
	{
		/* No, try remote branch */
		char remote_buf[256];
		snprintf(remote_buf,sizeof(remote_buf),"%s/%s","origin",branch);
		if (git_branch_lookup(&branch_ref,repo,remote_buf,GIT_BRANCH_REMOTE) == 0)
		{
			/* Exists, now try to create a local one */
			int err;
			git_object *commit;
			if ((err = git_reference_peel(&commit,branch_ref,GIT_OBJ_COMMIT)) == 0)
			{
				git_commit *comm;
				if ((rc = git_commit_lookup(&comm,repo,git_object_id(commit)) == 0))
				{
					if (git_branch_create(&branch_ref, repo, branch, comm, 0) == 0)
					{
						/* TODO: Alter tracking config */
					} else
					{
						fprintf(stderr,"Error code %d\n",err);
						libgit_error();
						goto out;
					}
				} else
				{
					fprintf(stderr,"Error code %d\n",err);
					libgit_error();
					goto out;
				}
			} else
			{
				fprintf(stderr,"Error code %d\n",err);
				libgit_error();
				goto out;
			}
		} else
		{
			branch_ref = NULL;
		}
	}

	printf("Checking out %s\n",branch_ref?git_reference_name(branch_ref):branch);
	if ((err = git_repository_set_head(repo,branch_ref?git_reference_name(branch_ref):branch)) != 0)
	{
		fprintf(stderr,"Error code %d\n",err);
		libgit_error();
		goto out;
	}

	/* Default options. Note by default we perform a dry checkout */
	memset(&checkout_opts,0,sizeof(checkout_opts));
	checkout_opts.version = GIT_CHECKOUT_OPTS_VERSION;
	checkout_opts.conflict_cb = conflict_cb;

	checkout_opts.checkout_strategy = GIT_CHECKOUT_FORCE|GIT_CHECKOUT_REMOVE_UNTRACKED;//GIT_CHECKOUT_SAFE|GIT_CHECKOUT_UPDATE_UNTRACKED;
	if (git_checkout_head(repo,&checkout_opts) != 0)
	{
		libgit_error();
		goto out;
	}

	rc = EXIT_SUCCESS;
out:
	if (branch_ref)
		git_reference_free(branch_ref);

	return rc;
}
