/*
 * Copyright (C) 2013 Cloudius Systems, Ltd.
 *
 * This work is open source software, licensed under the terms of the
 * BSD license as described in the LICENSE file in the top-level directory.
 */

// OSV does not support different users.
// In this file we implement various user-handling functions in traditional
// libc, in the most sensible way we can given that limitation. In particular:
// 1. getuid(), getgid() and friends always return 0.
// 2. Every user name resolves to user id 0.
// 3. setuid() (and similar) to any other user id but 0 fails on assertion.

#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <errno.h>
#include <grp.h>

#include <osv/debug.hh>
#include "libc.hh"

uid_t getuid()
{
    return 0;
}

gid_t getgid()
{
    return 0;
}

gid_t getegid(void)
{
    return 0;
}

static char username[] = "osv";
static char password[] = "";
static char gecos[] = "OSV User";
static char homedir[] = "/";
static char shell[] = "?";
static char *group_members[] = { username };

static struct passwd single_user = {
        username, password, 0, 0, gecos, homedir, shell
};

struct passwd *getpwnam(const char *name)
{
   return &single_user;
}

int getpwuid_r(uid_t uid, struct passwd *pwd,
               char *buf, size_t buflen, struct passwd **result)
{
    auto alloc = [&](int n) { auto tmp = buf; buf += n; return tmp; };
    auto save = [&](const char* s) { return strcpy(alloc(strlen(s) + 1), s); };

    pwd->pw_name = save(username);
    pwd->pw_passwd = save(password);
    pwd->pw_uid = 0;
    pwd->pw_gid = 0;
    pwd->pw_gecos = save(gecos);
    pwd->pw_dir = save(homedir);
    pwd->pw_shell = save(shell);
    *result = pwd;
    return 0;
}

struct passwd* getpwuid(uid_t uid)
{
    static struct passwd ret;
    static char buf[300];
    struct passwd *p;
    int e;

    e = getpwuid_r(uid, &ret, buf, sizeof(buf), &p);
    if (e == 0) {
        return &ret;
    } else {
        return libc_error_ptr<passwd>(e);
    }
}

int setuid(uid_t uid)
{
    assert(uid == 0);
    return 0;
}

int setgid(gid_t gid)
{
    assert(gid == 0);
    return 0;
}

int setresuid(uid_t ruid, uid_t euid, uid_t suid)
{
    if ( (ruid == (uid_t)-1 || ruid == 0) &&
         (euid == (uid_t)-1 || euid == 0) &&
         (suid == (uid_t)-1 || suid == 0)) {
        return 0;
    } else {
        errno = EPERM;
        return -1;
    }
}
int setresgid(gid_t rgid, gid_t egid, gid_t sgid)
{
    if ( (rgid == (gid_t)-1 || rgid == 0) &&
         (egid == (gid_t)-1 || egid == 0) &&
         (sgid == (gid_t)-1 || sgid == 0)) {
        return 0;
    } else {
        errno = EPERM;
        return -1;
    }
}

static struct group single_group = {
    username, password, getgid(), group_members
};

struct group *getgrnam(const char *name)
{
    if (!strcmp(name, single_group.gr_name)) {
        return NULL;
    }
    return &single_group;
}

int
getgrnam_r (const char *name, struct group *grp, char *buffer, size_t buflen,
            struct group **result)
{
    if (!strcmp(name, single_group.gr_name)) {
        *result = NULL;
        return 0;
    }

    *grp = single_group;
    *result = grp;

    return 0;
}

int
getgrgid_r (gid_t gid, struct group *grp, char *buffer, size_t buflen,
            struct group **result)
{
    if (gid != getgid()) {
        *result = NULL;
        return 0;
    }

    *grp = single_group;
    *result = grp;

    return 0;
}
