#include "core/console.h"

#include "aurora/arch/x86_64/cpu.h"
#include "aurora/arch/x86_64/paging.h"
#include "aurora/boot/boot_info.h"
#include "core/debug/log.h"
#include "core/debug/serial.h"
#include "core/interrupt.h"
#include "core/libk/format.h"
#include "core/libk/string.h"
#include "core/memory.h"
#include "core/scheduler.h"
#include "core/syscall.h"

#include <stdint.h>

enum {
    MAX_COMMANDS = 96,
    MAX_INPUT = 256,
    MAX_ARGS = 32,
    MAX_HISTORY = 64,
    MAX_ALIAS = 32,
    MAX_FS_ENTRIES = 128,
    MAX_PATH = 96,
    MAX_CONTENT = 384,
};

typedef struct {
    const char* name;
    const char* help;
    console_command_fn fn;
} command_entry_t;

typedef struct {
    char from[32];
    char to[32];
    bool used;
} alias_entry_t;

typedef struct {
    char path[MAX_PATH];
    char content[MAX_CONTENT];
    bool used;
    bool is_dir;
} fs_entry_t;

static command_entry_t g_commands[MAX_COMMANDS];
static size_t g_command_count;
static char g_input[MAX_INPUT];
static size_t g_input_len;
static char g_history[MAX_HISTORY][MAX_INPUT];
static size_t g_history_head;
static size_t g_history_count;
static alias_entry_t g_aliases[MAX_ALIAS];
static fs_entry_t g_fs[MAX_FS_ENTRIES];
static char g_cwd[MAX_PATH] = "/";
static uint64_t g_boot_tick;

static void console_out(const char* text) {
    serial_write(text);
}

static void console_outln(const char* text) {
    serial_write(text);
    serial_write("\r\n");
}

static void console_printf(const char* fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    kvsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    console_out(buffer);
}

static void console_printfln(const char* fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    kvsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    console_outln(buffer);
}

static void history_push(const char* line) {
    if (line == 0 || line[0] == '\0') {
        return;
    }
    kstrncpy(g_history[g_history_head], line, MAX_INPUT - 1);
    g_history[g_history_head][MAX_INPUT - 1] = '\0';
    g_history_head = (g_history_head + 1u) % MAX_HISTORY;
    if (g_history_count < MAX_HISTORY) {
        g_history_count++;
    }
}

static int tokenize(char* line, const char** argv, int max_args) {
    int argc = 0;
    char* p = line;
    while (*p != '\0' && argc < max_args) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        if (*p == '\"') {
            p++;
            argv[argc++] = p;
            while (*p != '\0' && *p != '\"') {
                p++;
            }
            if (*p == '\"') {
                *p++ = '\0';
            }
        } else {
            argv[argc++] = p;
            while (*p != '\0' && *p != ' ' && *p != '\t') {
                p++;
            }
            if (*p != '\0') {
                *p++ = '\0';
            }
        }
    }
    return argc;
}

static const char* alias_lookup(const char* name) {
    for (size_t i = 0; i < MAX_ALIAS; i++) {
        if (g_aliases[i].used && kstrcmp(g_aliases[i].from, name) == 0) {
            return g_aliases[i].to;
        }
    }
    return name;
}

static void path_resolve(const char* in, char* out) {
    if (in == 0 || in[0] == '\0') {
        kstrncpy(out, g_cwd, MAX_PATH - 1);
        out[MAX_PATH - 1] = '\0';
        return;
    }
    if (in[0] == '/') {
        kstrncpy(out, in, MAX_PATH - 1);
        out[MAX_PATH - 1] = '\0';
        return;
    }
    if (kstrcmp(g_cwd, "/") == 0) {
        ksnprintf(out, MAX_PATH, "/%s", in);
    } else {
        ksnprintf(out, MAX_PATH, "%s/%s", g_cwd, in);
    }
}

static fs_entry_t* fs_find(const char* path) {
    for (size_t i = 0; i < MAX_FS_ENTRIES; i++) {
        if (g_fs[i].used && kstrcmp(g_fs[i].path, path) == 0) {
            return &g_fs[i];
        }
    }
    return 0;
}

static fs_entry_t* fs_alloc(void) {
    for (size_t i = 0; i < MAX_FS_ENTRIES; i++) {
        if (!g_fs[i].used) {
            return &g_fs[i];
        }
    }
    return 0;
}

static bool fs_create(const char* path, bool is_dir) {
    if (fs_find(path) != 0) {
        return false;
    }
    fs_entry_t* entry = fs_alloc();
    if (entry == 0) {
        return false;
    }
    entry->used = true;
    entry->is_dir = is_dir;
    kstrncpy(entry->path, path, MAX_PATH - 1);
    entry->path[MAX_PATH - 1] = '\0';
    entry->content[0] = '\0';
    return true;
}

static bool fs_remove(const char* path) {
    fs_entry_t* entry = fs_find(path);
    if (entry == 0 || kstrcmp(path, "/") == 0) {
        return false;
    }
    entry->used = false;
    entry->path[0] = '\0';
    entry->content[0] = '\0';
    return true;
}

static void fs_bootstrap(void) {
    for (size_t i = 0; i < MAX_FS_ENTRIES; i++) {
        g_fs[i].used = false;
    }
    fs_create("/", true);
    fs_create("/home", true);
    fs_create("/proc", true);
    fs_create("/tmp", true);
    fs_create("/home/readme.txt", false);
    fs_entry_t* readme = fs_find("/home/readme.txt");
    if (readme != 0) {
        kstrncpy(readme->content, "Welcome to Aurora console", MAX_CONTENT - 1);
        readme->content[MAX_CONTENT - 1] = '\0';
    }
}

static void cmd_help(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    console_printfln("commands (%u):", (unsigned)g_command_count);
    for (size_t i = 0; i < g_command_count; i++) {
        console_printfln("  %-10s %s", g_commands[i].name, g_commands[i].help);
    }
}

static void cmd_clear(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    console_out("\x1b[2J\x1b[H");
}

static void cmd_echo(int argc, const char** argv) {
    for (int i = 1; i < argc; i++) {
        console_out(argv[i]);
        if (i + 1 < argc) {
            console_out(" ");
        }
    }
    console_out("\r\n");
}

static void cmd_uptime(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    const uint64_t ticks = interrupt_ticks();
    console_printfln("uptime: %u ticks since boot", (unsigned)ticks - (unsigned)g_boot_tick);
}

static void cmd_ticks(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    console_printfln("ticks: %u", (unsigned)interrupt_ticks());
}

static void cmd_date(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    console_outln("date: rtc driver pending, using monotonic ticks");
}

static void cmd_mem(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    memory_stats_t stats = memory_stats();
    console_printfln("mem total=%u used=%u free=%u",
                     (unsigned)stats.total_bytes,
                     (unsigned)stats.used_bytes,
                     (unsigned)stats.free_bytes);
}

static void cmd_ps(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    scheduler_dump_tasks();
    console_outln("ps: task list dumped to log");
}

static void cmd_pwd(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    console_outln(g_cwd);
}

static void cmd_cd(int argc, const char** argv) {
    if (argc < 2) {
        console_outln("cd: missing path");
        return;
    }
    char path[MAX_PATH];
    path_resolve(argv[1], path);
    fs_entry_t* entry = fs_find(path);
    if (entry == 0 || !entry->is_dir) {
        console_outln("cd: no such directory");
        return;
    }
    kstrncpy(g_cwd, path, sizeof(g_cwd) - 1);
    g_cwd[sizeof(g_cwd) - 1] = '\0';
}

static void cmd_ls(int argc, const char** argv) {
    char path[MAX_PATH];
    if (argc >= 2) {
        path_resolve(argv[1], path);
    } else {
        path_resolve(0, path);
    }
    size_t matches = 0;
    for (size_t i = 0; i < MAX_FS_ENTRIES; i++) {
        if (!g_fs[i].used || kstrcmp(g_fs[i].path, path) == 0) {
            continue;
        }
        if (kstrncmp(g_fs[i].path, path, kstrlen(path)) == 0) {
            console_out(g_fs[i].is_dir ? "d " : "f ");
            console_outln(g_fs[i].path);
            matches++;
        }
    }
    if (matches == 0) {
        console_outln("(empty)");
    }
}

static void cmd_mkdir(int argc, const char** argv) {
    if (argc < 2) {
        console_outln("mkdir: missing path");
        return;
    }
    char path[MAX_PATH];
    path_resolve(argv[1], path);
    if (!fs_create(path, true)) {
        console_outln("mkdir: failed");
    }
}

static void cmd_rmdir(int argc, const char** argv) {
    if (argc < 2) {
        console_outln("rmdir: missing path");
        return;
    }
    char path[MAX_PATH];
    path_resolve(argv[1], path);
    if (!fs_remove(path)) {
        console_outln("rmdir: failed");
    }
}

static void cmd_touch(int argc, const char** argv) {
    if (argc < 2) {
        console_outln("touch: missing path");
        return;
    }
    char path[MAX_PATH];
    path_resolve(argv[1], path);
    if (fs_find(path) == 0) {
        if (!fs_create(path, false)) {
            console_outln("touch: failed");
        }
    }
}

static void cmd_write(int argc, const char** argv) {
    if (argc < 3) {
        console_outln("write: write <file> <text>");
        return;
    }
    char path[MAX_PATH];
    path_resolve(argv[1], path);
    fs_entry_t* file = fs_find(path);
    if (file == 0) {
        fs_create(path, false);
        file = fs_find(path);
    }
    if (file == 0 || file->is_dir) {
        console_outln("write: invalid file");
        return;
    }
    file->content[0] = '\0';
    for (int i = 2; i < argc; i++) {
        if (kstrlen(file->content) + kstrlen(argv[i]) + 2 >= MAX_CONTENT) {
            break;
        }
        if (i > 2) {
            kstrcat(file->content, " ");
        }
        kstrcat(file->content, argv[i]);
    }
}

static void cmd_append(int argc, const char** argv) {
    if (argc < 3) {
        console_outln("append: append <file> <text>");
        return;
    }
    char path[MAX_PATH];
    path_resolve(argv[1], path);
    fs_entry_t* file = fs_find(path);
    if (file == 0) {
        fs_create(path, false);
        file = fs_find(path);
    }
    if (file == 0 || file->is_dir) {
        console_outln("append: invalid file");
        return;
    }
    for (int i = 2; i < argc; i++) {
        if (kstrlen(file->content) + kstrlen(argv[i]) + 2 >= MAX_CONTENT) {
            break;
        }
        if (kstrlen(file->content) > 0) {
            kstrcat(file->content, " ");
        }
        kstrcat(file->content, argv[i]);
    }
}

static void cmd_cat(int argc, const char** argv) {
    if (argc < 2) {
        console_outln("cat: missing path");
        return;
    }
    char path[MAX_PATH];
    path_resolve(argv[1], path);
    fs_entry_t* file = fs_find(path);
    if (file == 0 || file->is_dir) {
        console_outln("cat: file not found");
        return;
    }
    console_outln(file->content);
}

static void cmd_rm(int argc, const char** argv) {
    if (argc < 2) {
        console_outln("rm: missing path");
        return;
    }
    char path[MAX_PATH];
    path_resolve(argv[1], path);
    if (!fs_remove(path)) {
        console_outln("rm: failed");
    }
}

static void cmd_cp(int argc, const char** argv) {
    if (argc < 3) {
        console_outln("cp: cp <src> <dst>");
        return;
    }
    char src_path[MAX_PATH];
    char dst_path[MAX_PATH];
    path_resolve(argv[1], src_path);
    path_resolve(argv[2], dst_path);
    fs_entry_t* src = fs_find(src_path);
    if (src == 0 || src->is_dir) {
        console_outln("cp: source missing");
        return;
    }
    fs_create(dst_path, false);
    fs_entry_t* dst = fs_find(dst_path);
    if (dst == 0) {
        console_outln("cp: destination failed");
        return;
    }
    kstrncpy(dst->content, src->content, MAX_CONTENT - 1);
    dst->content[MAX_CONTENT - 1] = '\0';
}

static void cmd_mv(int argc, const char** argv) {
    if (argc < 3) {
        console_outln("mv: mv <src> <dst>");
        return;
    }
    char src_path[MAX_PATH];
    char dst_path[MAX_PATH];
    path_resolve(argv[1], src_path);
    path_resolve(argv[2], dst_path);
    fs_entry_t* src = fs_find(src_path);
    if (src == 0) {
        console_outln("mv: source missing");
        return;
    }
    fs_entry_t* dst = fs_find(dst_path);
    if (dst == 0) {
        fs_create(dst_path, src->is_dir);
        dst = fs_find(dst_path);
    }
    if (dst == 0) {
        console_outln("mv: destination failed");
        return;
    }
    dst->is_dir = src->is_dir;
    kstrncpy(dst->content, src->content, MAX_CONTENT - 1);
    dst->content[MAX_CONTENT - 1] = '\0';
    fs_remove(src_path);
}

static void cmd_stat(int argc, const char** argv) {
    if (argc < 2) {
        console_outln("stat: missing path");
        return;
    }
    char path[MAX_PATH];
    path_resolve(argv[1], path);
    fs_entry_t* entry = fs_find(path);
    if (entry == 0) {
        console_outln("stat: not found");
        return;
    }
    console_printfln("path=%s type=%s size=%u",
                     entry->path,
                     entry->is_dir ? "dir" : "file",
                     (unsigned)kstrlen(entry->content));
}

static void cmd_wc(int argc, const char** argv) {
    if (argc < 2) {
        console_outln("wc: missing path");
        return;
    }
    char path[MAX_PATH];
    path_resolve(argv[1], path);
    fs_entry_t* file = fs_find(path);
    if (file == 0 || file->is_dir) {
        console_outln("wc: file missing");
        return;
    }
    size_t chars = 0;
    size_t words = 0;
    bool in_word = false;
    for (size_t i = 0; file->content[i] != '\0'; i++) {
        chars++;
        const char c = file->content[i];
        const bool sep = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
        if (sep) {
            in_word = false;
        } else if (!in_word) {
            words++;
            in_word = true;
        }
    }
    console_printfln("wc: chars=%u words=%u", (unsigned)chars, (unsigned)words);
}

static void cmd_head(int argc, const char** argv) {
    (void)argc;
    cmd_cat(argc, argv);
}

static void cmd_tail(int argc, const char** argv) {
    (void)argc;
    cmd_cat(argc, argv);
}

static void cmd_find(int argc, const char** argv) {
    if (argc < 2) {
        console_outln("find: missing token");
        return;
    }
    size_t count = 0;
    for (size_t i = 0; i < MAX_FS_ENTRIES; i++) {
        if (g_fs[i].used && g_fs[i].content[0] != '\0' && kstrstr(g_fs[i].content, argv[1]) != 0) {
            console_outln(g_fs[i].path);
            count++;
        }
    }
    console_printfln("find: %u hits", (unsigned)count);
}

static void cmd_tree(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    for (size_t i = 0; i < MAX_FS_ENTRIES; i++) {
        if (!g_fs[i].used) {
            continue;
        }
        console_out(g_fs[i].is_dir ? "[D] " : "[F] ");
        console_outln(g_fs[i].path);
    }
}

static void cmd_whoami(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    console_outln("root");
}

static void cmd_hostname(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    console_outln("aurora-node");
}

static void cmd_version(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    console_outln("Aurora kernel 0.1.0");
}

static void cmd_uname(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    console_outln("Aurora x86_64");
}

static void cmd_sleep(int argc, const char** argv) {
    if (argc < 2) {
        console_outln("sleep: sleep <ticks>");
        return;
    }
    const uint64_t wait = (uint64_t)katoi(argv[1]);
    const uint64_t target = interrupt_ticks() + wait;
    while (interrupt_ticks() < target) {
        cpu_pause();
    }
}

static void cmd_history(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    size_t idx = (g_history_head + MAX_HISTORY - g_history_count) % MAX_HISTORY;
    for (size_t i = 0; i < g_history_count; i++) {
        console_printfln("%u %s", (unsigned)i, g_history[idx]);
        idx = (idx + 1u) % MAX_HISTORY;
    }
}

static void cmd_alias(int argc, const char** argv) {
    if (argc == 1) {
        for (size_t i = 0; i < MAX_ALIAS; i++) {
            if (g_aliases[i].used) {
                console_printfln("%s -> %s", g_aliases[i].from, g_aliases[i].to);
            }
        }
        return;
    }
    if (argc < 3) {
        console_outln("alias: alias <from> <to>");
        return;
    }
    for (size_t i = 0; i < MAX_ALIAS; i++) {
        if (!g_aliases[i].used || kstrcmp(g_aliases[i].from, argv[1]) == 0) {
            g_aliases[i].used = true;
            kstrncpy(g_aliases[i].from, argv[1], sizeof(g_aliases[i].from) - 1);
            g_aliases[i].from[sizeof(g_aliases[i].from) - 1] = '\0';
            kstrncpy(g_aliases[i].to, argv[2], sizeof(g_aliases[i].to) - 1);
            g_aliases[i].to[sizeof(g_aliases[i].to) - 1] = '\0';
            return;
        }
    }
    console_outln("alias: table full");
}

static void cmd_fsinfo(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    size_t files = 0;
    size_t dirs = 0;
    for (size_t i = 0; i < MAX_FS_ENTRIES; i++) {
        if (!g_fs[i].used) {
            continue;
        }
        if (g_fs[i].is_dir) {
            dirs++;
        } else {
            files++;
        }
    }
    console_printfln("fs: files=%u dirs=%u cap=%u", (unsigned)files, (unsigned)dirs, (unsigned)MAX_FS_ENTRIES);
}

static void cmd_reboot(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    console_outln("reboot: not wired yet, halting");
    cpu_cli();
    for (;;) {
        cpu_hlt();
    }
}

static void cmd_shutdown(int argc, const char** argv) {
    cmd_reboot(argc, argv);
}

static void cmd_about(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    console_outln("Aurora: modular x86_64 kernel in active development");
}

static void cmd_ping(int argc, const char** argv) {
    const uint64_t token = (argc >= 2) ? (uint64_t)katoi(argv[1]) : 7;
    const uint64_t response = syscall_dispatch(0, token, 0, 0, 0);
    console_printfln("ping => %u", (unsigned)response);
}

static void cmd_alloc(int argc, const char** argv) {
    if (argc < 2) {
        console_outln("alloc: alloc <bytes>");
        return;
    }
    const uint64_t size = (uint64_t)katoi(argv[1]);
    const uint64_t ptr = syscall_dispatch(3, size, 0, 0, 0);
    console_printfln("alloc => 0x%x", ptr);
}

static void cmd_taskcount(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    const uint64_t count = syscall_dispatch(2, 0, 0, 0, 0);
    console_printfln("taskcount => %u", (unsigned)count);
}

static void cmd_root(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    console_printfln("cr3 root => 0x%x", paging_root());
}

static void cmd_map(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    paging_stats_t stats = paging_stats();
    console_printfln("paging: root=0x%x mapped=%u bytes",
                     stats.pml4_phys,
                     (unsigned)stats.mapped_bytes);
}

static void cmd_page(int argc, const char** argv) {
    if (argc < 3) {
        console_outln("page: page <virt_hex> <phys_hex>");
        return;
    }
    const uint64_t virt = (uint64_t)katoi_hex(argv[1]);
    const uint64_t phys = (uint64_t)katoi_hex(argv[2]);
    if (paging_map_2m(virt, phys, PAGE_WRITABLE)) {
        console_outln("page: ok");
    } else {
        console_outln("page: failed");
    }
}

static void cmd_loglevel(int argc, const char** argv) {
    if (argc < 2) {
        console_printfln("loglevel=%u", (unsigned)log_get_level());
        return;
    }
    const int level = katoi(argv[1]);
    if (level < 0 || level > 3) {
        console_outln("loglevel: expected 0..3");
        return;
    }
    log_set_level((log_level_t)level);
}

static void cmd_hex(int argc, const char** argv) {
    if (argc < 2) {
        console_outln("hex: hex <number>");
        return;
    }
    const uint64_t value = (uint64_t)katoi(argv[1]);
    console_printfln("0x%x", value);
}

static bool cmd_register(const char* name, const char* help, console_command_fn fn) {
    return console_register_command(name, help, fn);
}

void console_init(void) {
    g_command_count = 0;
    g_input_len = 0;
    g_history_head = 0;
    g_history_count = 0;
    g_boot_tick = interrupt_ticks();
    for (size_t i = 0; i < MAX_ALIAS; i++) {
        g_aliases[i].used = false;
    }

    fs_bootstrap();

    cmd_register("help", "show commands", cmd_help);
    cmd_register("clear", "clear screen", cmd_clear);
    cmd_register("cls", "alias of clear", cmd_clear);
    cmd_register("echo", "print text", cmd_echo);
    cmd_register("uptime", "show uptime", cmd_uptime);
    cmd_register("ticks", "show scheduler ticks", cmd_ticks);
    cmd_register("date", "show date source", cmd_date);
    cmd_register("mem", "memory stats", cmd_mem);
    cmd_register("free", "alias of mem", cmd_mem);
    cmd_register("ps", "process list", cmd_ps);
    cmd_register("top", "alias of ps", cmd_ps);
    cmd_register("jobs", "alias of ps", cmd_ps);
    cmd_register("pwd", "print working dir", cmd_pwd);
    cmd_register("ls", "list files", cmd_ls);
    cmd_register("dir", "alias of ls", cmd_ls);
    cmd_register("ll", "alias of ls", cmd_ls);
    cmd_register("cd", "change directory", cmd_cd);
    cmd_register("mkdir", "create directory", cmd_mkdir);
    cmd_register("rmdir", "remove directory", cmd_rmdir);
    cmd_register("touch", "create file", cmd_touch);
    cmd_register("mkfile", "alias of touch", cmd_touch);
    cmd_register("write", "overwrite file", cmd_write);
    cmd_register("append", "append to file", cmd_append);
    cmd_register("cat", "read file", cmd_cat);
    cmd_register("rm", "delete file", cmd_rm);
    cmd_register("del", "alias of rm", cmd_rm);
    cmd_register("cp", "copy file", cmd_cp);
    cmd_register("mv", "move/rename file", cmd_mv);
    cmd_register("stat", "file metadata", cmd_stat);
    cmd_register("wc", "word count", cmd_wc);
    cmd_register("head", "show file head", cmd_head);
    cmd_register("tail", "show file tail", cmd_tail);
    cmd_register("find", "find content token", cmd_find);
    cmd_register("tree", "list full tree", cmd_tree);
    cmd_register("whoami", "current user", cmd_whoami);
    cmd_register("hostname", "machine name", cmd_hostname);
    cmd_register("version", "kernel version", cmd_version);
    cmd_register("uname", "system identity", cmd_uname);
    cmd_register("sleep", "sleep by ticks", cmd_sleep);
    cmd_register("history", "command history", cmd_history);
    cmd_register("alias", "set/show aliases", cmd_alias);
    cmd_register("fsinfo", "filesystem stats", cmd_fsinfo);
    cmd_register("reboot", "halt CPU", cmd_reboot);
    cmd_register("shutdown", "halt CPU", cmd_shutdown);
    cmd_register("about", "about aurora", cmd_about);
    cmd_register("ping", "syscall ping", cmd_ping);
    cmd_register("alloc", "syscall allocator", cmd_alloc);
    cmd_register("taskcount", "syscall task count", cmd_taskcount);
    cmd_register("root", "show page root", cmd_root);
    cmd_register("map", "show paging map", cmd_map);
    cmd_register("page", "map one 2M page", cmd_page);
    cmd_register("loglevel", "set log level", cmd_loglevel);
    cmd_register("hex", "decimal to hex", cmd_hex);

    console_outln("Aurora Console ready. Type 'help'.");
    console_print_prompt();
}

bool console_register_command(const char* name, const char* help, console_command_fn fn) {
    if (g_command_count >= MAX_COMMANDS) {
        return false;
    }
    g_commands[g_command_count].name = name;
    g_commands[g_command_count].help = help;
    g_commands[g_command_count].fn = fn;
    g_command_count++;
    return true;
}

void console_execute_line(const char* line) {
    if (line == 0 || line[0] == '\0') {
        return;
    }

    history_push(line);

    char local[MAX_INPUT];
    kstrncpy(local, line, sizeof(local) - 1);
    local[sizeof(local) - 1] = '\0';

    const char* argv[MAX_ARGS];
    int argc = tokenize(local, argv, MAX_ARGS);
    if (argc == 0) {
        return;
    }

    argv[0] = alias_lookup(argv[0]);
    for (size_t i = 0; i < g_command_count; i++) {
        if (kstrcmp(g_commands[i].name, argv[0]) == 0) {
            g_commands[i].fn(argc, argv);
            return;
        }
    }
    console_printfln("%s: command not found", argv[0]);
}

void console_print_prompt(void) {
    console_printf("%s$ ", g_cwd);
}

void console_handle_char(char c) {
    if (c == '\r' || c == '\n') {
        console_out("\r\n");
        g_input[g_input_len] = '\0';
        console_execute_line(g_input);
        g_input_len = 0;
        console_print_prompt();
        return;
    }
    if (c == '\b' || c == 0x7F) {
        if (g_input_len > 0) {
            g_input_len--;
            console_out("\b \b");
        }
        return;
    }
    if (g_input_len + 1 < MAX_INPUT) {
        g_input[g_input_len++] = c;
        serial_write_char(c);
    }
}

void console_poll(void) {
}

void console_run_scripted_boot_demo(void) {
    const char* demo_lines[] = {
        "version",
        "mem",
        "ps",
        "ls /home",
        "cat /home/readme.txt",
    };
    for (size_t i = 0; i < sizeof(demo_lines) / sizeof(demo_lines[0]); i++) {
        console_printfln("%s$ %s", g_cwd, demo_lines[i]);
        console_execute_line(demo_lines[i]);
    }
    console_print_prompt();
}
