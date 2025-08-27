#define _POSIX_C_SOURCE 200809L
#include "prompt.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <pwd.h>


// struct passwd {
//     char   *pw_name;   // username ("rudy")
//     char   *pw_passwd; // password hash (usually "x" for shadow file)
//     uid_t   pw_uid;    // numeric UID
//     gid_t   pw_gid;    // numeric GID
//     char   *pw_gecos;  // full name / info
//     char   *pw_dir;    // home directory path
//     char   *pw_shell;  // login shell
// };


void prompt_print(const char *home_dir){
    int uid=(int)getuid();
    struct passwd *pw=getpwuid(uid);
    char *username=pw->pw_name;

    char hostname[HOST_NAME_MAX+1];
    if(gethostname(hostname,sizeof(hostname))!=0){
        strcpy(hostname,"unknown");
    }

    char cwd[PATH_MAX];
    if(getcwd(cwd,sizeof(cwd))==NULL){
        strcpy(cwd,"?");
    }
   // printf("cwd %s\n",cwd);
    char display_path[PATH_MAX];

    if(strncmp(cwd,home_dir,strlen(home_dir))==0){
        snprintf(display_path,sizeof(display_path),"~%s",cwd+strlen(home_dir));
    }
    else{
        snprintf(display_path,sizeof(display_path),"%s",cwd);
    }

    printf("%s@%s:%s>",username,hostname,display_path);


}

