#include <math.h>
#include <curses.h>
#include <dirent.h>
#include <grp.h>
#include <linux/limits.h>
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

#define BL_GREEN "\e[102m"
#define RSET_COLOR "\e[0m"

struct Adate {
    int month;
    int month_day;
    int item3;
};

static int filename_width = 0;
static int link_width = 0;
static int owner_name_width = 0;
static int group_name_width = 0;
static int size_width = 0;
static bool contain_tyear_file = false;
static struct Adate adate_width = { 3, 1, 0 };
static int defp_dsp_num = 0;
static int dispnum = 0;
static int instances_per_line = 0;
static int terimal_with = 0;
static int i_and_a_width = 0;
static int *order = NULL;

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
    defp_dsp_num = 0;
    dispnum = 0;
    instances_per_line = 0;
    setupterm(NULL, fileno(stdout), (int*)0);
    terimal_with = tigetnum("cols");
    free(order);
    order = NULL;
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

static void quick_sort(char* data[], int left, int right)
{
    if (left < right) {
        int pos = once_quick_sort(data, left, right);
        quick_sort(data, left, pos - 1);
        quick_sort(data, pos + 1, right);
    }
}

static void display_default(struct stat* buf, char* filename)
{
    if (S_ISDIR(buf->st_mode))
        printf("%s%*s%s ", BL_GREEN, i_and_a_width, filename, RSET_COLOR);
    else
        printf("%*s ", i_and_a_width, filename);
}

static void display_l(struct stat* buf, char* filename)
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

static void display(int option, const char* path_with_filename)
{
    char filename[NAME_MAX + 1];
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
        if (filename[0] != '.') {
            display_default(&buf, filename);
        }
        break;
    case OPTION_A:
        display_default(&buf, filename);
        break;
    case OPTION_L:
        if (filename[0] != '.') {
            display_l(&buf, filename);
            if (S_ISDIR(buf.st_mode))
                printf(" %s%-10s%s\n", BL_GREEN, filename, RSET_COLOR);
            else
                printf(" %-10s\n", filename);
        }
        break;
    case OPTION_A + OPTION_L:
        display_l(&buf, filename);
        printf(" %-10s\n", filename);
        break;
    }
}

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
        display(option, path_with_filename[i]);
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
    char afileadir[10][PATH_MAX];
    int num_of_option = 0;
    init_global_variable();

    k = 0;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (j = 1; j < strlen(argv[i]); j++) {
                raw_option[k++] = argv[i][j];
            }
            num_of_option++;
        }
    }
    raw_option[k] = '\0';

    // printf("%s\n", option);
    for (i = 0; i < k; i++) {
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
    // printf("%d\n", option);

    i = 1;
    j = 0;
    struct stat buf;
    int file_num = 0;
    do {
        if (argv[i][0] == '-') {
            i++;
            continue;
        } else {
            printf("%s\n", argv[i]);
            strcpy(afileadir[j], argv[i]);
            afileadir[j][strlen(argv[i])] = '\0';
            i++;
            j++;

            if ((lstat(afileadir[j], &buf) != -1) && !S_ISDIR(buf.st_mode)) {
                file_num++;
                if (strlen(afileadir[j]) > i_and_a_width)
                    i_and_a_width = strlen(afileadir[j]);
            }
        }
    } while (i < argc);

    char* tmp_afileadir[j];
    for (i = 0; i < j; i++) {
        tmp_afileadir[i] = afileadir[i];
    }
    quick_sort(tmp_afileadir, 0, j - 1);

    instances_per_line = terimal_with / (i_and_a_width + 1);
    int rows = (int)ceil(file_num / instances_per_line);
    if (rows == 1){
        order = NULL;
    }
    else{
        order = (int*)malloc(sizeof(int) * rows * instances_per_line);
        menset(order, 0, sizeof(order));
        int count = 0;
        int i = 0, j = 0;
        for (j = 0; j < instances_per_line; j++) {
            for (i = 0; i < rows; i++) {
                order[i * instances_per_line + j] = count++;
                if (count == file_num)
                    break;
            }
        }
    }

    for (i = 0; i < j; i++) {
        if (lstat(tmp_afileadir[i], &buf) == -1) {
            printf("ls: cannont access '%s': No such file or directory", tmp_afileadir[i]);
            continue;
        }
        if (!S_ISDIR(buf.st_mode)) {
            display(option, tmp_afileadir[i]);
        }
    }

    for (i = 0; i < j; i++) {
        if (lstat(tmp_afileadir[i], &buf) == -1) {
            printf("ls: cannont access '%s': No such file or directory", tmp_afileadir[i]);
            continue;
        }
        if (S_ISDIR(buf.st_mode)) {
            if (tmp_afileadir[i][strlen(tmp_afileadir[i]) - 1] != '/') {
                tmp_afileadir[i][strlen(tmp_afileadir[i])] = '/';
                tmp_afileadir[i][strlen(tmp_afileadir[i]) + 1] = '\0';
            } else {
                tmp_afileadir[i][strlen(tmp_afileadir[i])] = '\0';
            }
            display_dir(option, tmp_afileadir[i]);
        }
    }
    return 0;
}