#include "ghezi.h"

int revert_to_commit(const char *id, const char *message, bool do_commit){
    if(get_silent())
        return 0;
    char keep_cwd[1024];
    if(getcwd(keep_cwd, sizeof(keep_cwd)) == NULL)
        return 1;
    if(chdir_ghezi())
        return 1;
    FILE *f = fopen(string_concat("commits/", id, string_concat2("/", commit_branch)), "r");
    char cur_bra[1024];
    fscanf(f, "%s", cur_bra);
    fclose(f);
    if(is_in_file(merged_branchs, cur_bra)){
        if(chdir(keep_cwd))
            return 1;
        return fprintf(stderr, "commit %s has been merged. revert faild\n", id), 0;
    }
    char *msg = malloc(MAX_COMMIT_MESSAGE_SIZE + 10);
    if(do_commit && !strlen(message)){
        FILE *f = fopen(string_concat("commits/", id, string_concat2("/", commit_message)), "r");
        if(f == NULL)
            return 1;
        fscanf(f, "%s", msg);
        fclose(f);
    }
    else
        strcpy(msg, message);
    f = fopen(branch_name, "r");
    if(f == NULL)
        return 1;
    char cur_br[1024], keep_id[1024];
    fscanf(f, "%s", cur_br);
    fclose(f);
    f = fopen(last_commit, "r");
    if(f == NULL)
        return 1;
    fscanf(f, "%s", keep_id);
    fclose(f);

    if(checkout_to_commit(id))
        return 1;

    if(chdir_ghezi())
        return 1;
    f = fopen(branch_name, "w");
    fprintf(f, "%s", cur_br);
    fclose(f);
    
    if(do_commit){
        if(commit(msg, true, false))
            return 1;
    }
    else{
        if(chdir_ghezi())
            return 1;
        f = fopen(last_commit, "w");
        fprintf(f, "%s", keep_id);
        fclose(f);
    }
    if(chdir(keep_cwd))
        return 1;
    return 0;

}

int revert_to_head(int step, const char *msg){
    if(get_silent())
        return 0;
    char *id = get_head_x_commit(step);
    if(strlen(id) == 0)
        return 0;
    return revert_to_commit(id, msg, true);
}