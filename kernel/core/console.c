#include <aurora/arch/x86_64/cpu_ops.h>
#include <aurora/core/console.h>
#include <aurora/drivers/keyboard.h>
#include <aurora/drivers/serial.h>
#include <aurora/lib/string.h>
#include <aurora/mm/kheap.h>
#include <aurora/mm/pmm.h>
#include <aurora/mm/vmm.h>
#include <aurora/sched/scheduler.h>

enum {
    CONSOLE_LINE_MAX = 256U,
    CONSOLE_PATH_MAX = 128U,
    CONSOLE_HISTORY_MAX = 64U,
    VFS_MAX_NODES = 128U,
    VFS_NAME_MAX = 32U,
    VFS_FILE_CAPACITY = 1024U
};

typedef struct vfs_node {
    bool used;
    bool is_dir;
    int32_t parent;
    char name[VFS_NAME_MAX];
    char data[VFS_FILE_CAPACITY];
    size_t size;
} vfs_node_t;

static vfs_node_t g_nodes[VFS_MAX_NODES];
static int32_t g_current_dir = 0;
static bool g_console_ready = false;

static char g_history[CONSOLE_HISTORY_MAX][CONSOLE_LINE_MAX];
static size_t g_history_len = 0U;
static size_t g_history_next = 0U;

static bool char_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static bool char_is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool str_equal(const char *left, const char *right) {
    if (left == (const char *)0 || right == (const char *)0) {
        return false;
    }

    size_t i = 0U;
    while (left[i] != '\0' && right[i] != '\0') {
        if (left[i] != right[i]) {
            return false;
        }
        ++i;
    }
    return left[i] == '\0' && right[i] == '\0';
}

static bool str_contains(const char *text, const char *needle) {
    if (text == (const char *)0 || needle == (const char *)0) {
        return false;
    }
    if (needle[0] == '\0') {
        return true;
    }

    const size_t text_len = aurora_strlen(text);
    const size_t needle_len = aurora_strlen(needle);
    if (needle_len > text_len) {
        return false;
    }

    for (size_t i = 0U; i + needle_len <= text_len; ++i) {
        bool match = true;
        for (size_t j = 0U; j < needle_len; ++j) {
            if (text[i + j] != needle[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;
}

static bool parse_u64(const char *text, uint64_t *value_out) {
    if (text == (const char *)0 || value_out == (uint64_t *)0 || text[0] == '\0') {
        return false;
    }

    uint64_t value = 0ULL;
    for (size_t i = 0U; text[i] != '\0'; ++i) {
        if (!char_is_digit(text[i])) {
            return false;
        }
        value = (value * 10ULL) + (uint64_t)(text[i] - '0');
    }
    *value_out = value;
    return true;
}

static void write_text(const char *text) {
    serial_write(text);
}

static void write_line(const char *text) {
    serial_write(text);
    serial_write_char('\n');
}

static int32_t fs_alloc_node(void) {
    for (int32_t i = 0; i < (int32_t)VFS_MAX_NODES; ++i) {
        if (!g_nodes[i].used) {
            return i;
        }
    }
    return -1;
}

static size_t fs_node_count(void) {
    size_t used = 0U;
    for (int32_t i = 0; i < (int32_t)VFS_MAX_NODES; ++i) {
        if (g_nodes[i].used) {
            ++used;
        }
    }
    return used;
}

static int32_t fs_find_child(int32_t parent, const char *name) {
    for (int32_t i = 0; i < (int32_t)VFS_MAX_NODES; ++i) {
        if (g_nodes[i].used && g_nodes[i].parent == parent && str_equal(g_nodes[i].name, name)) {
            return i;
        }
    }
    return -1;
}

static int32_t fs_create_node(int32_t parent, const char *name, bool is_dir) {
    const int32_t slot = fs_alloc_node();
    if (slot < 0) {
        return -1;
    }

    g_nodes[slot].used = true;
    g_nodes[slot].is_dir = is_dir;
    g_nodes[slot].parent = parent;
    g_nodes[slot].size = 0U;
    g_nodes[slot].data[0] = '\0';
    (void)aurora_strlcpy(g_nodes[slot].name, name, sizeof(g_nodes[slot].name));
    return slot;
}

static int32_t fs_resolve(const char *path) {
    if (path == (const char *)0 || path[0] == '\0') {
        return g_current_dir;
    }

    char buffer[CONSOLE_PATH_MAX];
    (void)aurora_strlcpy(buffer, path, sizeof(buffer));

    int32_t current = (buffer[0] == '/') ? 0 : g_current_dir;
    char *cursor = (buffer[0] == '/') ? &buffer[1] : &buffer[0];

    while (*cursor != '\0') {
        while (*cursor == '/') {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }

        char *segment = cursor;
        while (*cursor != '\0' && *cursor != '/') {
            ++cursor;
        }
        if (*cursor == '/') {
            *cursor = '\0';
            ++cursor;
        }

        if (str_equal(segment, ".")) {
            continue;
        }
        if (str_equal(segment, "..")) {
            if (current != 0 && g_nodes[current].parent >= 0) {
                current = g_nodes[current].parent;
            }
            continue;
        }

        const int32_t child = fs_find_child(current, segment);
        if (child < 0) {
            return -1;
        }
        current = child;
    }

    return current;
}

static bool fs_resolve_parent(const char *path, int32_t *parent_out, char *name_out, size_t name_size) {
    if (path == (const char *)0 || path[0] == '\0' || parent_out == (int32_t *)0 || name_out == (char *)0 || name_size == 0U) {
        return false;
    }

    char buffer[CONSOLE_PATH_MAX];
    (void)aurora_strlcpy(buffer, path, sizeof(buffer));

    size_t length = aurora_strlen(buffer);
    while (length > 1U && buffer[length - 1U] == '/') {
        buffer[length - 1U] = '\0';
        --length;
    }

    char *last_slash = (char *)0;
    for (size_t i = 0U; buffer[i] != '\0'; ++i) {
        if (buffer[i] == '/') {
            last_slash = &buffer[i];
        }
    }

    int32_t parent = g_current_dir;
    const char *name = buffer;

    if (last_slash != (char *)0) {
        *last_slash = '\0';
        name = last_slash + 1;
        if (last_slash == buffer) {
            parent = 0;
        } else {
            parent = fs_resolve(buffer);
        }
    }

    if (parent < 0 || !g_nodes[parent].used || !g_nodes[parent].is_dir) {
        return false;
    }
    if (name[0] == '\0' || str_equal(name, ".") || str_equal(name, "..")) {
        return false;
    }

    (void)aurora_strlcpy(name_out, name, name_size);
    *parent_out = parent;
    return true;
}

static bool fs_dir_empty(int32_t node) {
    for (int32_t i = 0; i < (int32_t)VFS_MAX_NODES; ++i) {
        if (g_nodes[i].used && g_nodes[i].parent == node) {
            return false;
        }
    }
    return true;
}

static bool fs_is_ancestor(int32_t ancestor, int32_t node) {
    int32_t cursor = node;
    while (cursor > 0) {
        if (cursor == ancestor) {
            return true;
        }
        cursor = g_nodes[cursor].parent;
    }
    return false;
}

static void fs_build_path(int32_t node, char *out, size_t out_size) {
    if (out_size == 0U) {
        return;
    }

    if (node <= 0) {
        (void)aurora_strlcpy(out, "/", out_size);
        return;
    }

    int32_t stack[32];
    size_t depth = 0U;
    int32_t cursor = node;
    while (cursor > 0 && depth < 32U) {
        stack[depth++] = cursor;
        cursor = g_nodes[cursor].parent;
    }

    out[0] = '\0';
    for (size_t i = 0U; i < depth; ++i) {
        const int32_t n = stack[depth - 1U - i];
        if (aurora_strlen(out) + aurora_strlen(g_nodes[n].name) + 2U >= out_size) {
            break;
        }
        aurora_strlcpy(out + aurora_strlen(out), "/", out_size - aurora_strlen(out));
        aurora_strlcpy(out + aurora_strlen(out), g_nodes[n].name, out_size - aurora_strlen(out));
    }
}

static char *next_token(char **cursor) {
    if (cursor == (char **)0 || *cursor == (char *)0) {
        return (char *)0;
    }

    char *p = *cursor;
    while (*p != '\0' && char_is_space(*p)) {
        ++p;
    }
    if (*p == '\0') {
        *cursor = p;
        return (char *)0;
    }

    char *start = p;
    while (*p != '\0' && !char_is_space(*p)) {
        ++p;
    }
    if (*p != '\0') {
        *p = '\0';
        ++p;
    }
    *cursor = p;
    return start;
}

static char *skip_spaces(char *text) {
    if (text == (char *)0) {
        return (char *)0;
    }
    while (*text != '\0' && char_is_space(*text)) {
        ++text;
    }
    return text;
}

static void history_add(const char *line) {
    if (line == (const char *)0 || line[0] == '\0') {
        return;
    }
    (void)aurora_strlcpy(g_history[g_history_next], line, sizeof(g_history[g_history_next]));
    g_history_next = (g_history_next + 1U) % CONSOLE_HISTORY_MAX;
    if (g_history_len < CONSOLE_HISTORY_MAX) {
        ++g_history_len;
    }
}

static const char *history_get(size_t index) {
    if (index >= g_history_len) {
        return (const char *)0;
    }
    const size_t start = (g_history_next + CONSOLE_HISTORY_MAX - g_history_len) % CONSOLE_HISTORY_MAX;
    const size_t slot = (start + index) % CONSOLE_HISTORY_MAX;
    return g_history[slot];
}

static void print_prompt(void) {
    char path[CONSOLE_PATH_MAX];
    fs_build_path(g_current_dir, path, sizeof(path));
    write_text("aurora:");
    write_text(path);
    write_text("$ ");
}

static void cmd_help(void) {
    write_line("Commands (45):");
    write_line("help clear cls echo uptime ticks date mem free ps top jobs");
    write_line("pwd ls dir ll cd mkdir rmdir touch mkfile write append cat rm del");
    write_line("cp mv stat wc head tail find tree whoami hostname version uname");
    write_line("sleep history alias fsinfo reboot shutdown about");
}

static void cmd_ps(void) {
    write_line("ID  STATE      WAKE    NAME");
    const int32_t capacity = scheduler_task_capacity();
    for (int32_t i = 0; i < capacity; ++i) {
        if (!scheduler_task_active(i)) {
            continue;
        }
        serial_write_dec_u64((uint64_t)i);
        write_text("   ");
        const char *state = scheduler_task_state(i);
        write_text(state);
        const size_t state_len = aurora_strlen(state);
        for (size_t s = state_len; s < 9U; ++s) {
            serial_write_char(' ');
        }
        serial_write_dec_u64(scheduler_task_wakeup_tick(i));
        write_text("   ");
        write_line(scheduler_task_name(i));
    }
}

static void cmd_mem(void) {
    write_text("pmm_free_pages=");
    serial_write_dec_u64(pmm_free_pages());
    write_text(" pmm_total_pages=");
    serial_write_dec_u64(pmm_total_pages());
    write_text(" heap=");
    serial_write_dec_u64(kheap_bytes_used());
    serial_write_char('/');
    serial_write_dec_u64(kheap_bytes_total());
    write_text(" vmm_reserved=");
    serial_write_dec_u64(vmm_bytes_reserved());
    serial_write_char('\n');
}

static void cmd_uptime(void) {
    const uint64_t ticks = scheduler_ticks();
    write_text("ticks=");
    serial_write_dec_u64(ticks);
    write_text(" seconds=");
    serial_write_dec_u64(ticks / 100U);
    serial_write_char('\n');
}

static void cmd_date(void) {
    const uint64_t seconds = scheduler_ticks() / 100U;
    const uint64_t h = seconds / 3600U;
    const uint64_t m = (seconds % 3600U) / 60U;
    const uint64_t s = seconds % 60U;
    write_text("boot+");
    serial_write_dec_u64(h);
    serial_write_char(':');
    serial_write_dec_u64(m);
    serial_write_char(':');
    serial_write_dec_u64(s);
    serial_write_char('\n');
}

static void cmd_pwd(void) {
    char path[CONSOLE_PATH_MAX];
    fs_build_path(g_current_dir, path, sizeof(path));
    write_line(path);
}

static void print_ls_entry(int32_t node, bool long_mode) {
    if (!long_mode) {
        write_text(g_nodes[node].name);
        if (g_nodes[node].is_dir) {
            serial_write_char('/');
        }
        serial_write_char('\n');
        return;
    }

    write_text(g_nodes[node].is_dir ? "d " : "f ");
    serial_write_dec_u64((uint64_t)g_nodes[node].size);
    write_text(" ");
    write_text(g_nodes[node].name);
    if (g_nodes[node].is_dir) {
        serial_write_char('/');
    }
    serial_write_char('\n');
}

static void cmd_ls(const char *path, bool long_mode) {
    int32_t dir = g_current_dir;
    if (path != (const char *)0 && path[0] != '\0') {
        dir = fs_resolve(path);
        if (dir < 0) {
            write_line("ls: path not found");
            return;
        }
    }

    if (!g_nodes[dir].is_dir) {
        print_ls_entry(dir, long_mode);
        return;
    }

    for (int32_t i = 0; i < (int32_t)VFS_MAX_NODES; ++i) {
        if (g_nodes[i].used && g_nodes[i].parent == dir) {
            print_ls_entry(i, long_mode);
        }
    }
}

static void cmd_cd(const char *path) {
    if (path == (const char *)0 || path[0] == '\0') {
        g_current_dir = 0;
        return;
    }

    const int32_t dir = fs_resolve(path);
    if (dir < 0 || !g_nodes[dir].is_dir) {
        write_line("cd: directory not found");
        return;
    }
    g_current_dir = dir;
}

static void cmd_mkdir(const char *path) {
    int32_t parent = -1;
    char name[VFS_NAME_MAX];
    if (!fs_resolve_parent(path, &parent, name, sizeof(name))) {
        write_line("mkdir: invalid path");
        return;
    }
    if (fs_find_child(parent, name) >= 0) {
        write_line("mkdir: already exists");
        return;
    }
    if (fs_create_node(parent, name, true) < 0) {
        write_line("mkdir: no free nodes");
    }
}

static int32_t ensure_file_node(const char *path, bool create_if_missing) {
    const int32_t existing = fs_resolve(path);
    if (existing >= 0) {
        if (g_nodes[existing].is_dir) {
            return -2;
        }
        return existing;
    }

    if (!create_if_missing) {
        return -1;
    }

    int32_t parent = -1;
    char name[VFS_NAME_MAX];
    if (!fs_resolve_parent(path, &parent, name, sizeof(name))) {
        return -1;
    }
    return fs_create_node(parent, name, false);
}

static void cmd_touch(const char *path) {
    const int32_t node = ensure_file_node(path, true);
    if (node == -2) {
        write_line("touch: path is a directory");
    } else if (node < 0) {
        write_line("touch: failed");
    }
}

static void write_file_data(int32_t node, const char *text, bool append) {
    if (node < 0 || text == (const char *)0) {
        return;
    }

    size_t write_offset = append ? g_nodes[node].size : 0U;
    if (!append) {
        g_nodes[node].size = 0U;
        g_nodes[node].data[0] = '\0';
    }

    const size_t len = aurora_strlen(text);
    const size_t space_left = (VFS_FILE_CAPACITY - 1U) - write_offset;
    const size_t copy = (len < space_left) ? len : space_left;

    aurora_memcpy(&g_nodes[node].data[write_offset], text, copy);
    g_nodes[node].size = write_offset + copy;
    g_nodes[node].data[g_nodes[node].size] = '\0';
}

static void cmd_write_common(const char *path, const char *text, bool append) {
    if (path == (const char *)0 || path[0] == '\0') {
        write_line("write: missing file path");
        return;
    }

    const int32_t node = ensure_file_node(path, true);
    if (node == -2) {
        write_line("write: target is a directory");
        return;
    }
    if (node < 0) {
        write_line("write: failed");
        return;
    }

    write_file_data(node, (text != (const char *)0) ? text : "", append);
}

static void cmd_cat(const char *path) {
    const int32_t node = fs_resolve(path);
    if (node < 0 || g_nodes[node].is_dir) {
        write_line("cat: file not found");
        return;
    }
    write_line(g_nodes[node].data);
}

static void cmd_rm(const char *path) {
    const int32_t node = fs_resolve(path);
    if (node <= 0) {
        write_line("rm: invalid path");
        return;
    }
    if (g_nodes[node].is_dir) {
        write_line("rm: is a directory (use rmdir)");
        return;
    }
    g_nodes[node].used = false;
}

static void cmd_rmdir(const char *path) {
    const int32_t node = fs_resolve(path);
    if (node <= 0 || !g_nodes[node].is_dir) {
        write_line("rmdir: directory not found");
        return;
    }
    if (!fs_dir_empty(node)) {
        write_line("rmdir: directory not empty");
        return;
    }
    g_nodes[node].used = false;
}

static void cmd_cp(const char *src, const char *dst) {
    if (src == (const char *)0 || src[0] == '\0' || dst == (const char *)0 || dst[0] == '\0') {
        write_line("cp: usage cp <src> <dst>");
        return;
    }

    const int32_t source = fs_resolve(src);
    if (source < 0 || g_nodes[source].is_dir) {
        write_line("cp: source file not found");
        return;
    }
    const int32_t target = ensure_file_node(dst, true);
    if (target < 0 || g_nodes[target].is_dir) {
        write_line("cp: invalid destination");
        return;
    }
    write_file_data(target, g_nodes[source].data, false);
}

static void cmd_mv(const char *src, const char *dst) {
    if (src == (const char *)0 || src[0] == '\0' || dst == (const char *)0 || dst[0] == '\0') {
        write_line("mv: usage mv <src> <dst>");
        return;
    }

    const int32_t source = fs_resolve(src);
    if (source <= 0) {
        write_line("mv: source not found");
        return;
    }

    int32_t parent = -1;
    char name[VFS_NAME_MAX];
    if (!fs_resolve_parent(dst, &parent, name, sizeof(name))) {
        write_line("mv: invalid destination");
        return;
    }
    if (fs_find_child(parent, name) >= 0) {
        write_line("mv: destination already exists");
        return;
    }
    if (g_nodes[source].is_dir && fs_is_ancestor(source, parent)) {
        write_line("mv: cannot move directory into itself");
        return;
    }

    g_nodes[source].parent = parent;
    (void)aurora_strlcpy(g_nodes[source].name, name, sizeof(g_nodes[source].name));
}

static void cmd_stat(const char *path) {
    const int32_t node = fs_resolve(path);
    if (node < 0) {
        write_line("stat: path not found");
        return;
    }

    char full_path[CONSOLE_PATH_MAX];
    fs_build_path(node, full_path, sizeof(full_path));
    write_text("type=");
    write_text(g_nodes[node].is_dir ? "dir" : "file");
    write_text(" size=");
    serial_write_dec_u64((uint64_t)g_nodes[node].size);
    write_text(" path=");
    write_line(full_path);
}

static void cmd_wc(const char *path) {
    const int32_t node = fs_resolve(path);
    if (node < 0 || g_nodes[node].is_dir) {
        write_line("wc: file not found");
        return;
    }

    const char *data = g_nodes[node].data;
    const size_t bytes = g_nodes[node].size;
    size_t lines = 0U;
    size_t words = 0U;
    bool in_word = false;

    for (size_t i = 0U; i < bytes; ++i) {
        const char c = data[i];
        if (c == '\n') {
            ++lines;
        }
        if (char_is_space(c)) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            ++words;
        }
    }

    serial_write_dec_u64((uint64_t)lines);
    write_text(" ");
    serial_write_dec_u64((uint64_t)words);
    write_text(" ");
    serial_write_dec_u64((uint64_t)bytes);
    write_text(" ");
    write_line(path);
}

static void print_head_tail(const char *data, size_t size, uint64_t lines, bool from_head) {
    if (lines == 0U) {
        return;
    }

    if (from_head) {
        uint64_t left = lines;
        for (size_t i = 0U; i < size; ++i) {
            serial_write_char(data[i]);
            if (data[i] == '\n') {
                if (--left == 0U) {
                    break;
                }
            }
        }
        if (size == 0U || data[size - 1U] != '\n') {
            serial_write_char('\n');
        }
        return;
    }

    size_t line_breaks = 0U;
    for (size_t i = 0U; i < size; ++i) {
        if (data[i] == '\n') {
            ++line_breaks;
        }
    }

    size_t skip = 0U;
    if ((uint64_t)line_breaks > lines) {
        skip = line_breaks - (size_t)lines;
    }

    size_t seen = 0U;
    size_t start = 0U;
    for (size_t i = 0U; i < size; ++i) {
        if (data[i] == '\n') {
            if (seen == skip) {
                start = i + 1U;
                break;
            }
            ++seen;
        }
    }

    for (size_t i = start; i < size; ++i) {
        serial_write_char(data[i]);
    }
    if (size == 0U || data[size - 1U] != '\n') {
        serial_write_char('\n');
    }
}

static void cmd_head_tail(const char *path, const char *count_text, bool head) {
    uint64_t lines = 10ULL;
    if (count_text != (const char *)0 && count_text[0] != '\0') {
        if (!parse_u64(count_text, &lines)) {
            write_line(head ? "head: invalid line count" : "tail: invalid line count");
            return;
        }
    }

    const int32_t node = fs_resolve(path);
    if (node < 0 || g_nodes[node].is_dir) {
        write_line(head ? "head: file not found" : "tail: file not found");
        return;
    }

    print_head_tail(g_nodes[node].data, g_nodes[node].size, lines, head);
}

static void cmd_find(const char *pattern) {
    for (int32_t i = 1; i < (int32_t)VFS_MAX_NODES; ++i) {
        if (!g_nodes[i].used) {
            continue;
        }
        if (pattern != (const char *)0 && pattern[0] != '\0' && !str_contains(g_nodes[i].name, pattern)) {
            continue;
        }
        char path[CONSOLE_PATH_MAX];
        fs_build_path(i, path, sizeof(path));
        write_text(path);
        if (g_nodes[i].is_dir) {
            write_text("/");
        }
        serial_write_char('\n');
    }
}

static void tree_print_node(int32_t node, int32_t depth) {
    for (int32_t i = 0; i < depth; ++i) {
        write_text("  ");
    }
    write_text(g_nodes[node].name);
    if (g_nodes[node].is_dir) {
        write_text("/");
    }
    serial_write_char('\n');

    if (!g_nodes[node].is_dir) {
        return;
    }
    for (int32_t i = 0; i < (int32_t)VFS_MAX_NODES; ++i) {
        if (g_nodes[i].used && g_nodes[i].parent == node) {
            tree_print_node(i, depth + 1);
        }
    }
}

static void cmd_tree(const char *path) {
    int32_t node = g_current_dir;
    if (path != (const char *)0 && path[0] != '\0') {
        node = fs_resolve(path);
    }
    if (node < 0) {
        write_line("tree: path not found");
        return;
    }
    tree_print_node(node, 0);
}

static void cmd_history(void) {
    for (size_t i = 0U; i < g_history_len; ++i) {
        serial_write_dec_u64((uint64_t)i);
        write_text(": ");
        write_line(history_get(i));
    }
}

static void cmd_alias(void) {
    write_line("Aliases:");
    write_line("cls=clear dir=ls ll='ls long' top=ps jobs=ps free=mem mkfile=touch del=rm");
}

static void cmd_fsinfo(void) {
    write_text("nodes=");
    serial_write_dec_u64((uint64_t)fs_node_count());
    write_text("/");
    serial_write_dec_u64((uint64_t)VFS_MAX_NODES);
    write_text(" cwd=");
    char path[CONSOLE_PATH_MAX];
    fs_build_path(g_current_dir, path, sizeof(path));
    write_line(path);
}

static void cmd_whoami(void) {
    write_line("root");
}

static void cmd_hostname(void) {
    write_line("aurora");
}

static void cmd_version(void) {
    write_line("Aurora Kernel 0.2 (console build)");
}

static void cmd_uname(void) {
    write_line("Aurora x86_64");
}

static void cmd_about(void) {
    write_line("Aurora: educational monolithic kernel with serial shell and in-memory FS.");
}

static void cmd_sleep(const char *ticks_text) {
    uint64_t ticks = 1ULL;
    if (ticks_text != (const char *)0 && ticks_text[0] != '\0') {
        if (!parse_u64(ticks_text, &ticks)) {
            write_line("sleep: invalid ticks");
            return;
        }
    }
    scheduler_sleep(ticks);
}

static void cmd_shutdown(void) {
    write_line("system halted");
    cpu_halt_forever();
}

static void cmd_reboot(void) {
    write_line("reboot not implemented, halting");
    cpu_halt_forever();
}

static void execute_line(char *line) {
    char *cursor = line;
    char *cmd = next_token(&cursor);
    if (cmd == (char *)0) {
        return;
    }

    if (str_equal(cmd, "help")) { cmd_help(); return; }
    if (str_equal(cmd, "clear") || str_equal(cmd, "cls")) { write_text("\033[2J\033[H"); return; }
    if (str_equal(cmd, "echo")) { write_line(skip_spaces(cursor)); return; }
    if (str_equal(cmd, "uptime")) { cmd_uptime(); return; }
    if (str_equal(cmd, "ticks")) { cmd_uptime(); return; }
    if (str_equal(cmd, "date")) { cmd_date(); return; }
    if (str_equal(cmd, "mem") || str_equal(cmd, "free")) { cmd_mem(); return; }
    if (str_equal(cmd, "ps") || str_equal(cmd, "top") || str_equal(cmd, "jobs")) { cmd_ps(); return; }
    if (str_equal(cmd, "pwd")) { cmd_pwd(); return; }
    if (str_equal(cmd, "ls") || str_equal(cmd, "dir")) { cmd_ls(next_token(&cursor), false); return; }
    if (str_equal(cmd, "ll")) { cmd_ls(next_token(&cursor), true); return; }
    if (str_equal(cmd, "cd")) { cmd_cd(next_token(&cursor)); return; }
    if (str_equal(cmd, "mkdir")) { cmd_mkdir(next_token(&cursor)); return; }
    if (str_equal(cmd, "rmdir")) { cmd_rmdir(next_token(&cursor)); return; }
    if (str_equal(cmd, "touch") || str_equal(cmd, "mkfile")) { cmd_touch(next_token(&cursor)); return; }
    if (str_equal(cmd, "cat")) { cmd_cat(next_token(&cursor)); return; }
    if (str_equal(cmd, "rm") || str_equal(cmd, "del")) { cmd_rm(next_token(&cursor)); return; }
    if (str_equal(cmd, "stat")) { cmd_stat(next_token(&cursor)); return; }
    if (str_equal(cmd, "wc")) { cmd_wc(next_token(&cursor)); return; }
    if (str_equal(cmd, "find")) { cmd_find(next_token(&cursor)); return; }
    if (str_equal(cmd, "tree")) { cmd_tree(next_token(&cursor)); return; }
    if (str_equal(cmd, "whoami")) { cmd_whoami(); return; }
    if (str_equal(cmd, "hostname")) { cmd_hostname(); return; }
    if (str_equal(cmd, "version")) { cmd_version(); return; }
    if (str_equal(cmd, "uname")) { cmd_uname(); return; }
    if (str_equal(cmd, "about")) { cmd_about(); return; }
    if (str_equal(cmd, "history")) { cmd_history(); return; }
    if (str_equal(cmd, "alias")) { cmd_alias(); return; }
    if (str_equal(cmd, "fsinfo")) { cmd_fsinfo(); return; }
    if (str_equal(cmd, "sleep")) { cmd_sleep(next_token(&cursor)); return; }
    if (str_equal(cmd, "reboot")) { cmd_reboot(); return; }
    if (str_equal(cmd, "shutdown")) { cmd_shutdown(); return; }

    if (str_equal(cmd, "write") || str_equal(cmd, "append")) {
        char *path = next_token(&cursor);
        char *text = skip_spaces(cursor);
        cmd_write_common(path, text, str_equal(cmd, "append"));
        return;
    }
    if (str_equal(cmd, "cp")) {
        char *src = next_token(&cursor);
        char *dst = next_token(&cursor);
        cmd_cp(src, dst);
        return;
    }
    if (str_equal(cmd, "mv")) {
        char *src = next_token(&cursor);
        char *dst = next_token(&cursor);
        cmd_mv(src, dst);
        return;
    }
    if (str_equal(cmd, "head")) {
        char *path = next_token(&cursor);
        char *count = next_token(&cursor);
        cmd_head_tail(path, count, true);
        return;
    }
    if (str_equal(cmd, "tail")) {
        char *path = next_token(&cursor);
        char *count = next_token(&cursor);
        cmd_head_tail(path, count, false);
        return;
    }

    write_line("unknown command (use: help)");
}

void console_init(void) {
    aurora_memset(g_nodes, 0, sizeof(g_nodes));
    aurora_memset(g_history, 0, sizeof(g_history));
    g_history_len = 0U;
    g_history_next = 0U;

    g_nodes[0].used = true;
    g_nodes[0].is_dir = true;
    g_nodes[0].parent = -1;
    (void)aurora_strlcpy(g_nodes[0].name, "/", sizeof(g_nodes[0].name));

    (void)fs_create_node(0, "home", true);
    (void)fs_create_node(0, "tmp", true);
    (void)fs_create_node(0, "var", true);
    (void)fs_create_node(0, "bin", true);

    const int32_t readme = fs_create_node(0, "README.txt", false);
    if (readme >= 0) {
        write_file_data(
            readme,
            "Aurora Console\nType 'help' for commands.\n",
            false
        );
    }

    g_current_dir = 0;
    g_console_ready = true;
}

void console_run(void) {
    if (!g_console_ready) {
        console_init();
    }

    write_line("");
    write_line("Aurora Ready");
    write_line("Type 'help' to see available commands.");
    print_prompt();

    char line[CONSOLE_LINE_MAX];
    size_t line_len = 0U;
    aurora_memset(line, 0, sizeof(line));

    for (;;) {
        const int32_t value = keyboard_getchar();
        if (value < 0) {
            scheduler_sleep(1U);
            continue;
        }

        const char c = (char)value;
        if (c == '\r' || c == '\n') {
            serial_write_char('\n');
            line[line_len] = '\0';
            history_add(line);
            execute_line(line);
            line_len = 0U;
            line[0] = '\0';
            print_prompt();
            continue;
        }

        if (c == '\b' || c == 127) {
            if (line_len > 0U) {
                --line_len;
                line[line_len] = '\0';
                write_text("\b \b");
            }
            continue;
        }

        if ((uint8_t)c >= 32U && (uint8_t)c <= 126U && line_len + 1U < sizeof(line)) {
            line[line_len++] = c;
            line[line_len] = '\0';
            serial_write_char(c);
        }
    }
}
