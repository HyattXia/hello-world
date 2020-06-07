#include <curses.h>
#include <dirent.h>
#include <grp.h>
#include <linux/limits.h>
#include <math.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <term.h>
#include <time.h>
#include <unistd.h>

#define OPTION_NONE 0
#define OPTION_A 1
#define OPTION_L 2

#define MAX_DISPLAY_FILE_NUM 256
#define MAX_FANDD_PA_NUM 10

#define BL_GREEN "\e[102m"
#define L_GREEN "\e[92m"
#define RSET_COLOR "\e[0m"

struct Adate {
    int month;
    int month_day;
    int item3;
};

static int filename_width = 0;                  /* 文件名输出宽度 */
static int link_width = 0;                      /* l连接数的最大位数*/
static int owner_name_width = 0;                /* 当前展示文件拥有者名字的最大宽度 */
static int group_name_width = 0;                /* 当前展示用户组名的最大宽度 */
static int size_width = 0;                      /* 文件大小的最大宽度 */
static bool contain_tyear_file = false;         /* 是否包含上次修改时间是今年的文件 */
static struct Adate adate_width = { 3, 1, 0 };  /* 展示上次修改时间三个时间段的宽度*/
static int terimal_with = 0;                    /* 当前终端的宽度*/
static int i_and_a_width = 0;                   /* 单纯 -i选项和-a项展示问的宽度*/
static int pos = 0;                             /* 当前打印的位置*/

/*  初始化全区变量*/
static void 
init_global_variable(void)
{
    filename_width = 0;
    link_width = 0;
    owner_name_width = 0;
    group_name_width = 0;
    size_width = 0;
    contain_tyear_file = false;
    adate_width.month = 3;
    adate_width.month_day = 1;
    adate_width.item3 = 0;
    setupterm(NULL, fileno(stdout), (int*)0);
    terimal_with = tigetnum("cols");
    pos = 0;
}

static void my_error(const char* err_str, int line)
{
    fprintf(stderr, "line:%d  ", line);
    perror(err_str);
    exit(1);
}

static int once_quick_sort(char* data[], int low, int high)
{
    char* tmp = data[low];
    while (low < high) {
        while (low < high && (strcmp(tmp, data[high]) <= 0)) {
            high--;
        }
        data[low] = data[high];
        while (low < high && (strcmp(tmp, data[low]) >= 0)) {
            low++;
        }
        data[high] = data[low];
    }
    data[low] = tmp;
    return low;
}

/*  对字符串进行快速排序*/
static void quick_sort(char* data[], int left, int right)
{
    if (left < right) {
        int pos = once_quick_sort(data, left, right);
        quick_sort(data, left, pos - 1);
        quick_sort(data, pos + 1, right);
    }
}

/* 默认无任何选项展示*/
static void display_default(struct stat* buf, const char* filename)
{
    if (S_ISDIR(buf->st_mode))
        printf("%s%s%s ", BL_GREEN, filename, RSET_COLOR);
    else
        printf("%s%s%s ", L_GREEN, filename, RSET_COLOR);
}

/*  -l 选项展示*/
static void display_l(struct stat* buf, const char* filename)
{
    if (S_ISLNK(buf->st_mode))
        printf("l");
    else if (S_ISREG(buf->st_mode))
        printf("-");
    else if (S_ISCHR(buf->st_mode))
        printf("c");
    else if (S_ISBLK(buf->st_mode))
        printf("b");
    else if (S_ISFIFO(buf->st_mode))
        printf("f");
    else if (S_ISSOCK(buf->st_mode))
        printf("s");
    else if (S_ISDIR(buf->st_mode))
        printf("d");

    if (buf->st_mode & S_IRUSR)
        printf("r");
    else
        printf("-");

    if (buf->st_mode & S_IWUSR)
        printf("w");
    else
        printf("-");

    if (buf->st_mode & S_IXUSR)
        printf("x");
    else
        printf("-");

    if (buf->st_mode & S_IRGRP)
        printf("r");
    else
        printf("-");

    if (buf->st_mode & S_IWGRP)
        printf("w");
    else
        printf("-");

    if (buf->st_mode & S_IXGRP)
        printf("x");
    else
        printf("-");

    if (buf->st_mode & S_IROTH)
        printf("r");
    else
        printf("-");

    if (buf->st_mode & S_IWOTH)
        printf("w");
    else
        printf("-");

    if (buf->st_mode & S_IXOTH)
        printf("x");
    else
        printf("-");

    struct passwd* psd;
    struct group* grp;
    printf(" %*lu", link_width, buf->st_nlink);
    psd = getpwuid(buf->st_uid);
    grp = getgrgid(buf->st_gid);
    printf(" %*s", owner_name_width, psd->pw_name);
    printf(" %*s", group_name_width, grp->gr_name);

    printf(" %*lu", size_width, buf->st_size);

    time_t now_calendar_time;
    now_calendar_time = time(NULL);
    struct tm *now_bd_time, *st_abd_time;
    now_bd_time = localtime(&now_calendar_time);
    int this_year = now_bd_time->tm_year;
    st_abd_time = localtime(&buf->st_mtime);
    char tmp[10];
    strftime(tmp, 10, "%b", st_abd_time);
    printf(" %*s", adate_width.month, tmp);
    strftime(tmp, 10, "%e", st_abd_time);
    printf(" %*s", adate_width.month_day, tmp);

    if (this_year == st_abd_time->tm_year) {
        strftime(tmp, 10, "%R", st_abd_time);
    } else {
        strftime(tmp, 10, "%G", st_abd_time);
    }

    printf(" %*s", adate_width.item3, tmp);
}

/* 若打印位置到达终端有边界，输出换行*/
static void print_newline(const char* filename)
{
    pos += strlen(filename) + 1;
    if (pos > terimal_with) {
        printf("\n");
        pos = 0;
        pos += strlen(filename) + 1;
    }
}

static void diplay_single_file(int option, const char* path_with_filename)
{
    struct stat buf;
    if (lstat(path_with_filename, &buf) == -1) {
        my_error("lstat", __LINE__);
    }

    switch (option) {
    case OPTION_NONE:
    case OPTION_A:
        print_newline(path_with_filename);
        display_default(&buf, path_with_filename);
        break;
    case OPTION_L:
    case OPTION_A + OPTION_L:
        display_l(&buf, path_with_filename);
        if (S_ISDIR(buf.st_mode))
            printf(" %s%-1s%s\n", BL_GREEN, path_with_filename, RSET_COLOR);
        else
            printf(" %s%-1s%s\n", L_GREEN, path_with_filename, RSET_COLOR);
        break;
    }
}

/*  展示单个文件*/
static void diplay_entity_in_dir(int option, const char* path_with_filename)
{
    char filename[PATH_MAX];
    int i = 0, j = 0;
    for (i = 0; i < strlen(path_with_filename); i++) {
        if (path_with_filename[i] == '/') {
            j = 0;
            continue;
        }
        filename[j++] = path_with_filename[i];
    }
    filename[j] = '\0';
    struct stat buf;
    if (lstat(path_with_filename, &buf) == -1) {
        my_error("lstat", __LINE__);
    }

    switch (option) {
    case OPTION_NONE:
        if (strcmp(filename, "..") != 0 && strcmp(filename, ".") != 0) {
            if (filename[0] != '.') {
                print_newline(filename);
                display_default(&buf, filename);
            }
        }
        break;
    case OPTION_A:
        print_newline(filename);
        display_default(&buf, filename);
        break;
    case OPTION_L:
        if (strcmp(filename, "..") != 0 && strcmp(filename, ".") != 0) {
            if (filename[0] != '.') {
                display_l(&buf, filename);
                if (S_ISDIR(buf.st_mode))
                    printf(" %s%-1s%s\n", BL_GREEN, filename, RSET_COLOR);
                else
                    printf(" %s%-1s%s\n", L_GREEN, filename, RSET_COLOR);
            }
        }
        break;
    case OPTION_A + OPTION_L:
        display_l(&buf, filename);
        if (S_ISDIR(buf.st_mode))
            printf(" %s%-1s%s\n", BL_GREEN, filename, RSET_COLOR);
        else
            printf(" %s%-1s%s\n", L_GREEN, filename, RSET_COLOR);
        break;
    }
}

/*  展示文件夹*/
static void display_dir(int option, const char* path)
{
    DIR* dir;
    struct dirent* ptr;
    char filename[MAX_DISPLAY_FILE_NUM][PATH_MAX + 1];
    // printf("%s\n", path);

    if ((dir = opendir(path)) == NULL) {
        my_error("opendir", __LINE__);
    }

    int rentry_in_dir = 0;
    while ((ptr = readdir(dir)) != NULL) {
        rentry_in_dir++;
    }
    closedir(dir);

    if (rentry_in_dir > 256) {
        my_error("too mant files in the directory", __LINE__);
    }

    dir = opendir(path);
    int i, len = strlen(path);
    char* path_with_filename[rentry_in_dir];
    struct stat buf;
    struct passwd* psd;
    struct group* grp;
    for (i = 0; i < rentry_in_dir; i++) {
        ptr = readdir(dir);
        strncpy(filename[i], path, len);
        filename[i][len] = '\0';
        strcat(filename[i], ptr->d_name);
        filename[i][len + strlen(ptr->d_name)] = '\0';
        path_with_filename[i] = filename[i];
        // printf("%s\n", path_with_filename[i]);
        if (strlen(ptr->d_name) > filename_width)
            filename_width = strlen(ptr->d_name);

        if (lstat(path_with_filename[i], &buf) == -1)
            my_error("lstat", __LINE__);

        if (buf.st_nlink > link_width)
            link_width = buf.st_nlink;

        psd = getpwuid(buf.st_uid);
        if (strlen(psd->pw_name) > owner_name_width)
            owner_name_width = strlen(psd->pw_name);

        grp = getgrgid(buf.st_gid);
        if (strlen(grp->gr_name) > group_name_width)
            group_name_width = strlen(grp->gr_name);

        char st_size[20];
        sprintf(st_size, "%lu", buf.st_size);
        if (strlen(st_size) > size_width)
            size_width = strlen(st_size);

        time_t now_calendar_time;
        now_calendar_time = time(NULL);
        struct tm *now_bd_time, *st_abd_time;
        now_bd_time = localtime(&now_calendar_time);
        int this_year = now_bd_time->tm_year;
        st_abd_time = localtime(&buf.st_mtime);
        if (st_abd_time->tm_mday >= 10)
            adate_width.month_day = 2;

        if (this_year == st_abd_time->tm_year && adate_width.item3 < 5)
            adate_width.item3 = 5;

        if (st_abd_time->tm_year + 1900 >= 10000)
            adate_width.item3 = 5;
    }

    quick_sort(path_with_filename, 0, rentry_in_dir - 1);

    for (i = 0; i < rentry_in_dir; i++) {
        diplay_entity_in_dir(option, path_with_filename[i]);
    }

    if ((option & OPTION_L) == 0) {
        printf("\n");
    }
    closedir(dir);
}

int main(int argc, char* argv[])
{
    int i, j, k;
    char raw_option[16];
    int option = OPTION_NONE;
    char path[PATH_MAX];
    char a_dir[MAX_FANDD_PA_NUM][PATH_MAX];
    char a_file[MAX_FANDD_PA_NUM][PATH_MAX];
    int num_of_option = 0;
    init_global_variable();

    k = 0;
    for (i = 1; i < argc; i++) { /* 提取选项*/
        if (argv[i][0] == '-') {
            for (j = 1; j < strlen(argv[i]); j++) {
                raw_option[k++] = argv[i][j];
            }
            num_of_option++;
        }
    }
    raw_option[k] = '\0';

    for (i = 0; i < k; i++) { /* 设置选项*/
        if (raw_option[i] == 'a') {
            option |= OPTION_A;
            continue;
        } else if (raw_option[i] == 'l') {
            option |= OPTION_L;
            continue;
        } else {
            printf("my_ls: invaild option -%c\n", raw_option[i]);
            exit(1);
        }
    }

    if ((num_of_option + 1) == argc) {
        strcpy(path, "./");
        path[2] = '\0';
        display_dir(option, path);
        return 0;
    }

    i = 1;
    struct stat buf;
    int file_num = 0, dir_num = 0;
    do {
        if (argv[i][0] == '-') {
            i++;
            continue;
        } else {
            //printf("%s\n", argv[i]);
            if (lstat(argv[i], &buf) == -1) {
                printf("ls: cannont access '%s': No such file or directory\n", argv[i]);
                i++;
                continue;
            }

            if (S_ISDIR(buf.st_mode)) { /* 提取文件夹*/
                strcpy(a_dir[dir_num], argv[i]);
                a_dir[dir_num++][strlen(argv[i])] = '\0';
            } else { /* 提取文件*/
                strcpy(a_file[file_num], argv[i]);
                a_file[file_num][strlen(argv[i])] = '\0';
                if (strlen(a_file[file_num]) > i_and_a_width)
                    i_and_a_width = strlen(a_file[file_num]);
                file_num++;
            }
            i++;
        }
    } while (i < argc);

    /* 将文件和文件夹变成指向字符串的指针后，用快速排序排序*/
    char *tmp_a_file[file_num], *tmp_a_dir[dir_num];
    for (i = 0; i < file_num; i++)
        tmp_a_file[i] = a_file[i];

    for (i = 0; i < dir_num; i++)
        tmp_a_dir[i] = a_dir[i];

    quick_sort(tmp_a_file, 0, file_num - 1);
    quick_sort(tmp_a_dir, 0, dir_num - 1);

    for (i = 0; i < file_num; i++) { /* 展示单个文件*/
        diplay_single_file(option, tmp_a_file[i]);
    }

    if (dir_num > 0) {
        printf("\n");
    }
    for (i = 0; i < dir_num; i++) { /* 逐个展示文件夹中的文件*/
        init_global_variable();
        printf("%s:\n", tmp_a_dir[i]);
        if (tmp_a_dir[i][strlen(tmp_a_dir[i]) - 1] != '/') {
            tmp_a_dir[i][strlen(tmp_a_dir[i])] = '/';
            tmp_a_dir[i][strlen(tmp_a_dir[i]) + 1] = '\0';
        } else {
            tmp_a_dir[i][strlen(tmp_a_dir[i])] = '\0';
        }
        display_dir(option, tmp_a_dir[i]);
        if (dir_num > 1 && i < dir_num - 1) {
            printf("\n");
        }
    }
    return 0;
}