#ifndef INTRINSICS_H
#define INTRINSICS_H

// Intrinsic command handlers
int cmd_hop(int argc, char **argv);
int cmd_reveal(int argc, char **argv);
int cmd_log(int argc, char **argv);
int cmd_activities(int argc, char **argv);
int cmd_ping(int argc, char **argv);
int cmd_fg(int argc, char **argv);
int cmd_bg(int argc, char **argv);

// Log management
void log_add_command(const char *cmd);

#endif // INTRINSICS_H
