#ifndef _BOSH_CMD_H
#define _BOSH_CMD_H

/*
BOSH 命令头文件
*/

//cmd
void cmd_cls(uint32_t argc, char** argv);
void cmd_pwd(uint32_t argc, char** argv);
int cmd_cd(uint32_t argc, char** argv);
void cmd_ls(uint32_t argc, char** argv);
void cmd_help(uint32_t argc, char** argv);
int cmd_ps(uint32_t argc, char** argv);
int cmd_mkdir(uint32_t argc, char** argv);
int cmd_rmdir(uint32_t argc, char** argv);
int cmd_rm(uint32_t argc, char** argv);
int cmd_cat(int argc, char *argv[]);
int cmd_echo(int argc, char *argv[]);
int cmd_type(int argc, char *argv0[]);
void cmd_dir(uint32_t argc, char** argv);
void cmd_ver(uint32_t argc, char** argv);
void cmd_time(uint32_t argc, char** argv);
void cmd_date(uint32_t argc, char** argv);
int cmd_rename(uint32_t argc, char** argv);
int cmd_move(uint32_t argc, char** argv);
int cmd_copy(uint32_t argc, char** argv);
void cmd_reboot(uint32_t argc, char** argv);
void cmd_exit(uint32_t argc, char** argv);
void cmd_free(uint32_t argc, char** argv);
void cmd_lsdisk(uint32_t argc, char** argv);
void cmd_ls_sub(char *pathname, int detail);
int cmd_kill(uint32_t argc, char** argv);

int buildin_cmd(int cmd_argc, char **cmd_argv);

#endif  //_BOSH_CMD_H
