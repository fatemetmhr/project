#include "ghezi.h"

bool silent = false;

bool get_silent(){
    return silent;
}

int main(int argc, char *argvv[]) {

    char ** argv = (char **)malloc(100 * sizeof(char *));
    for(int i = 0; i < argc; i++){
        argv[i] = malloc(1024);
        strcpy(argv[i], argvv[i]);
    }
    
    if (argc < 2)
        return invalid_command(), silent;

    if(!strcmp(argv[1], "-SILENT")){
        silent = true;
        for(int i = 2; i < argc; i++)
            argv[i - 1] = argv[i];
        argc--;
    }

    if(!silent && argc == 2){
        char *command = find_in_map_with_space(string_concat(general_configs_path, "/", general_alias), argv[1]);
        if(strlen(command))
            return system(string_concat2("ghezi ", command));
    }

    if(!strcmp(argv[1], "init")){
        if(argc > 2)
            return invalid_command(), silent;
        return run_init(argc, argv);
    }

    if(!strcmp(argv[1], "config")){
        if (argc < 3)
            return invalid_command(), silent;
        if(!strcmp(argv[2], "reset"))
            return reset_general_config();
        if (argc < 4)
            return invalid_command(), silent;
        if(!strcmp(argv[2], "-global")){
            if (argc != 5)
                return invalid_command(), silent;
            if(!strcmp(argv[3], "user.name"))
                return update_general_config(argv[4], 0);
            if(!strcmp(argv[3], "user.email"))
                return update_general_config(argv[4], 1);
            if(!remove_prefix(argv[3], "alias.")){
                if(!is_valid_command(argv[4])){
                    if(silent)
                        return 1;
                    return fprintf(stderr, "Command \"%s\" isn't valid, can't make alias!", argv[4]), 0;
                }
                return set_general_alias(argv[3], argv[4]);
            }
            return invalid_command(), silent;
        }
        if(!silent && chdir_ghezi())
            return fprintf(stderr, "no repo found! use -global flag if you want to change general configs\n"), 0;
        if(argc != 4)
            return invalid_command(), silent;
        if(!strcmp(argv[2], "user.name"))
            return update_config_single(argv[3], 0);
        if(!strcmp(argv[2], "user.email"))
            return update_config_single(argv[3], 1);
        if(!remove_prefix(argv[2], "alias.")){
            if(!is_valid_command(argv[3])){
                if(silent)
                    return 1;
                return fprintf(stderr, "Command \"%s\" isn't valid, can't make alias!", argv[3]), 0;
            }
            return set_alias(argv[2], argv[3]);
        }
        return invalid_command(), silent;
    }

    // check if any ghezi repo exists
    char cwd[1024];
    if(getcwd(cwd, sizeof(cwd)) == NULL)
        return 1;
    if(!silent){
        int res;
        if((res = chdir_ghezi()) == 1)
            return fprintf(stderr, "no ghezi repo found!"), 0;
        else if(res)
            return 1;
        if(chdir(cwd))
            return 1;
    }
    
    if(!silent && argc == 2){
        char *command = find_in_map_with_space(string_concat(get_ghezi_wd(), "/", alias_name), argv[1]);
        if(strlen(command))
            return system(string_concat2("ghezi ", command));
    }

    if(!strcmp(argv[1], "add")){
        if(argc < 3)
            return invalid_command(), silent;
        if(!strcmp(argv[2], "-n")){
            if(argc != 4)
                return invalid_command(), silent;
            return show_stage_status_recursive("~ ", get_num(argv[3]));
        }
        if(!strcmp(argv[2], "-redo")){
            if(argc != 3)
                return invalid_command(), silent;
            return redo_add();
        }
        if(silent)
            return 0;
        int st = 2;
        if(!strcmp(argv[2], "-f"))
            st = 3;
        else if(argc != 3)
            return invalid_command(), 0;
        bool wild = false;
        for(int i = 0; i < argc; i++)
            wild |= is_wildcard(argv[i]);
        if(wild)
            return system(string_concat_master(argc, argv, 2, "-f"));
        if(shift_stage_history(1))
            return 1;
        while(st < argc){
            if(chdir(argv[st]) != 0){
                if(add_file(argv[st]))
                    return 1;
            }
            else{
                if(add_dir())
                    return 1;
                if(chdir(".."))
                    return 1;
            }
            st++;
        }
        return 0;
    }

    if(!strcmp(argv[1], "reset")){
        if(argc < 3)
            return invalid_command(), silent;
        if(silent)
            return 0;
        bool wild = false;
        for(int i = 0; i < argc; i++)
            wild |= is_wildcard(argv[i]);
        if(!strcmp(argv[2], "-undo")){
            if(argc != 3)
                return invalid_command(), 0;
            return shift_stage_history(-1);
        }
        int st = 2;
        if(!strcmp(argv[2], "-f"))
            st = 3;
        else if(argc != 3)
            return invalid_command(), 0;
        if(wild)
            return system(string_concat_master(argc, argv, 2, "-f"));
        while(st < argc){
            if(chdir(argv[st]) != 0){
                if(reset_file(argv[st]))
                    return 1;
            }
            else{
                if(reset_dir())
                    return 1;
                if(chdir(".."))
                    return 1;
            }
            st++;
        }
        return 0;
    }

    if(!strcmp(argv[1], "status")){
        if(argc > 2)
            return invalid_command(), silent;
        if(silent)
            return 0;
        status(false);
        return 0;
    }

    if(!strcmp(argv[1], "commit")){
        if(argc < 3 || argc > 5)
            return invalid_command(), silent;
        if(argc < 4){
            if(silent)
                return 1;
            return fprintf(stderr, "please set a message(or shortcut) for your commit\n"), 0;
        }
        bool forced = false;
        if(!strcmp(argv[2], "-f")){
            forced = true;
            for(int i = 3; i < argc; i++)
                argv[i - 1] = argv[i];
            argc--;
        }

        if(argc > 4)
            return invalid_command(), silent;
        if(!strcmp(argv[2], "-m")){
            if(strlen(argv[3]) > MAX_COMMIT_MESSAGE_SIZE){
                if(silent)
                    return 1;
                return fprintf(stderr, "too long message!\n"), 0;
            }
            if(!forced && !silent && !is_in_head())
                return fprintf(stderr, "Commiting is only available in the HEAD of a branch!\n"), 0;
            if(!silent && run_pre_commit(true))
                return fprintf(stderr, "Some hooks failed, commit can not be done.\n"), 0;
            return commit(argv[3], forced, false);
        }
        if(!strcmp(argv[2], "-s")){
            char *msg = find_short_cut(argv[3]);
            if(!silent && msg[0] == '\0')
                return fprintf(stderr, "shortcut %s does not exist\n", argv[3]), 0;
            if(!forced && !silent && !is_in_head())
                return fprintf(stderr, "Commiting is only available in the HEAD of a branch!\n"), 0;
            if(!silent && run_pre_commit(true))
                return fprintf(stderr, "Some hooks failed, commit can not be done.\n"), 0;
            return commit(msg, forced, false);
        }
        return invalid_command(), silent;
    }

    if(!strcmp(argv[1], "set")){
        if(argc != 6 || strcmp(argv[2], "-m") || strcmp(argv[4], "-s"))
            return invalid_command(), silent;
        if(strlen(argv[3]) > MAX_COMMIT_MESSAGE_SIZE){
            if(silent)
                return 1;
            return fprintf(stderr, "too long commit message!\n"), 0;
        }
        return set_message_shortcut(argv[3], argv[5]);
    }

    if(!strcmp(argv[1], "remove")){
        if(argc != 4 || strcmp(argv[2], "-s"))
            return invalid_command(), silent;
        return remove_message_shortcut(argv[3]);
    }

    if(!strcmp(argv[1], "replace")){
        if(argc != 6 || strcmp(argv[2], "-m") || strcmp(argv[4], "-s"))
            return invalid_command(), silent;
        if(strlen(argv[3]) > MAX_COMMIT_MESSAGE_SIZE){
            if(silent)
                return 1;
            return fprintf(stderr, "too long commit message!\n"), 0;
        }
        return replace_message_shortcut(argv[3], argv[5]);
    }

    if(!strcmp(argv[1], "branch")){
        if(argc > 3 || argc < 2)
            return invalid_command(), silent;
        if(argc == 2)
            return show_all_branchs();
        return make_branch(argv[2]);
    }

    if(!strcmp(argv[1], "log")){
        if(argc < 3)
            return show_all_logs((int)1e9);
        if(!strcmp(argv[2], "-search")){
            int n = argc - 3;
            char **words = (char **)malloc(n * sizeof(char *));
            for(int i = 0; i < n; i++)
                words[i] = argv[3 + i];
            return show_all_word_match_commits(n, words);
        }
        if(argc != 4)
            return invalid_command(), silent;
        if(!strcmp(argv[2], "-n"))
            return show_all_logs(get_num(argv[3]));
        if(!strcmp(argv[2], "-branch"))
            return show_all_branch_commits(argv[3]);
        if(!strcmp(argv[2], "-author"))
            return show_all_personal_commits(argv[3]);
        if(!strcmp(argv[2], "-since"))
            return show_all_during_commits(make_tm_from_date(argv[3]), make_tm_from_date("987684/0/0"));
        if(!strcmp(argv[2], "-before"))
            return show_all_during_commits(make_tm_from_date("0/0/0"), make_tm_from_date(argv[3]));
        return invalid_command(), silent;
    }

    if(!strcmp(argv[1], "checkout")){
        bool forced = false;
        if(argc >= 2 && !strcmp(argv[2], "-f")){
            forced = true;
            for(int i = 3; i < argc; i++)
                argv[i - 1] = argv[i];
            argc--;
        }
        if(argc != 3)
            return invalid_command(), silent;
        if(!forced && !silent && status(true))
            return printf("Some files has been changed but not commited. Checkout failed!\n"), 0;
        if(!strcmp(argv[2], "HEAD"))
            return checkout_to_head(0);
        if(!remove_prefix(argv[2], "HEAD-")){
            int x = get_num(argv[2]);
            if(x == -1)
                return invalid_command(), 0;
            return checkout_to_head(x);
        }
        if(is_commit_id_valid(argv[2]))
            return checkout_to_commit(argv[2]);
        return checkout_to_branch(argv[2]);
    }
    
    if(!strcmp(argv[1], "revert")){
        char *msg = malloc(1024);
        msg[0] = '\0';
        if(argc < 3)
            return invalid_command(), silent;
        if(!strcmp(argv[2], "-n")){
            if(argc != 4)
                return invalid_command(), silent;
            if(!silent && status(true))
                return printf("Some files has been changed but not commited. Revert failed!\n"), 0;
            if(!silent && !is_in_head())
                return fprintf(stderr, "Reverting is only available in the HEAD of a branch!\n"), 0;
            return revert_to_commit(argv[3], msg, false);
        }
        if(!strcmp(argv[2], "-m")){
            if(argc != 5)
                return invalid_command(), silent;
            strcpy(msg, argv[3]);
            argc = 3;
            argv[2] = argv[4];
            if(strlen(msg) > MAX_COMMIT_MESSAGE_SIZE){
                if(silent)
                    return 1;
                return fprintf(stderr, "too long message!\n"), 0;
            }
        }
        if(argc != 3)
            return invalid_command(), silent;
        if(!silent && status(true))
            return printf("Some files has been changed but not commited. Revert failed!\n"), 0;
        if(!silent && !is_in_head())
            return fprintf(stderr, "Reverting is only available in the HEAD of a branch!\n"), 0;
        if(!remove_prefix(argv[2], "HEAD-")){
            int x = get_num(argv[2]);
            if(x == -1)
                return invalid_command(), silent;
            return revert_to_head(x, msg);
        }
        return revert_to_commit(argv[2], msg, true);
    }

    if(!strcmp(argv[1], "tag")){
        if(argc == 2)
            return show_all_tags();
        if(!strcmp(argv[2], "-a")){
            if(argc < 4)
                return invalid_command(), silent;
            bool overwrite = false;
            char *msg = malloc(1024);
            msg[0] = ' ';
            msg[1] = '\0';
            char *id = malloc(1024);
            id[0] = '\0';
            if(argc > 4 && !strcmp(argv[4], "-m")){
                if(argc < 6)
                    return invalid_command(), silent;
                strcpy(msg, argv[5]);
                for(int i = 6; i < argc; i++)
                    argv[i - 2] = argv[i];
                argc -= 2;
            }
            if(argc > 4 && !strcmp(argv[4], "-c")){
                if(argc < 6)
                    return invalid_command(), silent;
                strcpy(id, argv[5]);
                for(int i = 6; i < argc; i++)
                    argv[i - 2] = argv[i];
                argc -= 2;
            }
            if(argc > 4 && !strcmp(argv[4], "-f")){
                overwrite = true;
                for(int i = 5; i < argc; i++)
                    argv[i - 1] = argv[i];
                argc--;
            }
            if(argc != 4)
                return invalid_command(), silent;
            if(silent)
                return 0;
            if(!strlen(id))
                id = get_current_commit_id();
            return add_tag(argv[3], id, msg, overwrite);

        }
        if(!strcmp(argv[2], "show")){
            if(argc != 4)
                return invalid_command(), silent;
            return print_tag(argv[3]);

        }
        return invalid_command(), silent;
    }

    if(!strcmp(argv[1], "diff")){
        if(argc < 5)
            return invalid_command(), silent;
        if(!strcmp(argv[2], "-f")){
            int begin1 = 1, begin2 = 1;
            int end1 = MAX_FILE_SIZE, end2 = MAX_FILE_SIZE;
            if(argc > 5 && !strcmp(argv[5], "-line1")){
                int pt = find_char_in_string(argv[6], '-');
                if(pt == -1 || pt == 0 || pt == strlen(argv[6]) - 1)
                    return invalid_command(), silent;
                argv[6][pt] = '\0';
                begin1 = get_num(argv[6]);
                end1 = get_num(argv[6] + pt + 1);
                if(begin1 == -1 || end1 == -1)
                    return invalid_command(), silent;
                for(int i = 7; i < argc; i++)
                    argv[i - 2] = argv[i];
                argc -= 2;
            }
            if(argc > 5 && !strcmp(argv[5], "-line2")){
                int pt = find_char_in_string(argv[6], '-');
                if(pt == -1 || pt == 0 || pt == strlen(argv[6]) - 1)
                    return invalid_command(), silent;
                argv[6][pt] = '\0';
                begin2 = get_num(argv[6]);
                end2 = get_num(argv[6] + pt + 1);
                if(begin2 == -1 || end2 == -1)
                    return invalid_command(), silent;
                for(int i = 7; i < argc; i++)
                    argv[i - 2] = argv[i];
                argc -= 2;
            }
            if(argc != 5)
                return invalid_command(), silent;
            file_diff_checker(argv[3], argv[4], begin1, end1, begin2, end2, false, false, "", "");
            return 0;
        }
        if(!strcmp(argv[2], "-c")){
            if(argc != 5)
                return invalid_command(), silent;
            commit_diff_checker(argv[3], argv[4], false, false, false, "", "");
            return 0;
        }
        return invalid_command(), silent;
    }

    if(!strcmp(argv[1], "merge")){
        if(argc != 5 || strcmp(argv[2], "-b"))
            return invalid_command(), silent;
       return  merge_branch(argv[3], argv[4]);
    }

    if(!strcmp(argv[1], "pre-commit")){
        if(argc == 2){
            run_pre_commit(false);
            return 0;
        }
        if(!strcmp(argv[2], "-f")){
            if(argc == 3)
                return invalid_command(), silent;
            if(silent)
                return 0;
            for(int i = 3; i < argc; i++){
                char *res = find_in_map(string_concat(get_ghezi_wd(), "/", stage_name), abs_path(argv[i]));
                if(!strlen(res) || !strcmp(res, "NULL"))
                    printf("file %s does not exist or isn't staged\n", argv[i]);
                else{
                    printf("file %s:\n", argv[i]);
                    run_pre_commit_for_file(res, get_file_format(argv[i]), false);
                }
            }
            return 0;
        }
        if(!strcmp(argv[2], "hooks")){
            if(argc != 4 || strcmp(argv[3], "list"))
                return invalid_command(), silent;
            return show_all_hooks();
        }
        if(!strcmp(argv[2], "applied")){
            if(argc != 4 || strcmp(argv[3], "hooks"))
                return invalid_command(), silent;
            return show_hook_list();
        }
        if(!strcmp(argv[2], "add")){
            if(argc != 5 || strcmp(argv[3], "hook"))
                return invalid_command(), silent;
            return add_hook(argv[4]);
        }
        if(!strcmp(argv[2], "remove")){
            if(argc != 5 || strcmp(argv[3], "hook"))
                return invalid_command(), silent;
            return remove_hook(argv[4]);
        }
        return invalid_command(), silent;
    }

    if(!strcmp(argv[1], "grep")){
        char *file = malloc(1024);
        if(argc < 6 || strcmp(argv[2], "-f") || strcmp(argv[4], "-p"))
            return invalid_command(), silent;
        strcpy(file, argv[3]);
        if(argc >= 8 && !strcmp(argv[6], "-c")){
            if(!silent){
                if(!is_in_file(string_concat(get_ghezi_wd(), "/", all_commits), argv[7]))
                    return fprintf(stderr, "no such commit exist\n"), 0;
                file = find_in_map(string_concat(get_ghezi_wd(), "/commits/", string_concat(argv[7], "/", commit_paths)), abs_path(argv[3]));
                if(!strlen(file) || !strcmp(file, "NULL"))
                    return fprintf(stderr, "this file did not exist in commit %s, grep failed\n", argv[7]), 0;
            }
            for(int i = 8; i < argc; i++)
                argv[i - 2] = argv[i];
            argc -= 2;
        }
        bool line = false;
        if(argc == 7 && !strcmp(argv[6], "-n")){
            line = true;
            argc--;
        }
        if(argc != 6)
            return invalid_command(), silent;
        return grep(file, argv[5], line);

    }

    return invalid_command(), silent;
}