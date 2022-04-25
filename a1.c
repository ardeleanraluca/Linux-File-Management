#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

typedef struct
{
    char name[10];
    int type;
    int offset;
    int size;
} Section;

typedef struct
{
    char magic[3];
    int size;
    int version;
    int nrSections;
    Section *sections;
} Header;

void list(const char *path, int isFile, int size, int hasPerm)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    dir = opendir(path);
    if (dir == NULL)
    {
        perror("ERROR\ninvalid directory path\n");
        return;
    }
    printf("SUCCESS\n");
    char fullPath[512] = "";
    struct stat statbuf;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if (lstat(fullPath, &statbuf) == 0)
            {
                if (isFile == 1)
                {
                    if (!S_ISDIR(statbuf.st_mode) && statbuf.st_size > size)
                    {
                        printf("%s\n", fullPath);
                    }
                }
                else if (hasPerm == 1)
                {
                    if (((statbuf.st_mode & 0700) == 0200 || (statbuf.st_mode & 0700) == 0300 ||
                         (statbuf.st_mode & 0700) == 0600 || (statbuf.st_mode & 0700) == 0700))
                    {
                        printf("%s\n", fullPath);
                    }
                }
                else
                {
                    printf("%s\n", fullPath);
                }
            }
        }
    }
    closedir(dir);
}

void listRec(const char *path, int okFile, int size, int hasPerm)
{
    static int succ = 0;
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512] = "";
    struct stat statbuf;
    dir = opendir(path);
    if (dir == NULL)
    {
        perror("ERROR\ninvalid directory path\n");
        return;
    }

    if (succ == 0)
    {
        printf("SUCCESS\n");
        succ = 1;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if (lstat(fullPath, &statbuf) == 0)
            {
                if (okFile == 1)
                {
                    if (!S_ISDIR(statbuf.st_mode) && statbuf.st_size > size)
                    {
                        printf("%s\n", fullPath);
                    }
                }
                else if (hasPerm == 1)
                {
                    if (((statbuf.st_mode & 0700) == 0200 || (statbuf.st_mode & 0700) == 0300 ||
                         (statbuf.st_mode & 0700) == 0600 || (statbuf.st_mode & 0700) == 0700))
                    {
                        printf("%s\n", fullPath);
                    }
                }
                else
                {
                    printf("%s\n", fullPath);
                }
                if (S_ISDIR(statbuf.st_mode))
                {
                    listRec(fullPath, okFile, size, hasPerm);
                }
            }
        }
    }
    closedir(dir);
}

char *strTok(char *string)
{

    char *p;
    char *str;
    p = strtok(string, "=");
    while (p != NULL)
    {
        str = p;
        p = strtok(NULL, "=");
    }
    return str;
}

int parse(char *path, int parseOrExtr, Header *header)
{
    // parsare = 0
    // extract = 1

    // returneaza 0 daca fisierul e corup si 1 daca e valid
    char details[50] = "";
    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        perror("Could not open input file");
        return 0;
    }

    header->sections = (Section *)malloc(sizeof(Section));

    char magic[3] = "";
    read(fd, &magic, 2);
    magic[2] = '\0';
    snprintf(header->magic, 3, "%s", magic);

    if (strcmp(magic, "5A") != 0)
    {
        if (parseOrExtr == 0)
            printf("ERROR\nwrong magic\n");
        close(fd);
        return 0;
    }

    int headerSize = 0;
    read(fd, &headerSize, 2);
    header->size = headerSize;

    int version = 0;
    read(fd, &version, 2);
    header->version = version;

    if (version < 80 || version > 137)
    {
        if (parseOrExtr == 0)
            printf("ERROR\nwrong version\n");
        close(fd);
        return 0;
    }

    int nrSections = 0;
    read(fd, &nrSections, 1);
    header->nrSections = nrSections;

    if (nrSections < 3 || nrSections > 20)
    {
        if (parseOrExtr == 0)
            printf("ERROR\nwrong sect_nr\n");
        close(fd);
        return 0;
    }

    if (parseOrExtr == 0)
        snprintf(details, 50, "%s%s%d\n%s%d\n", "SUCCESS\n", "version=", version, "nr_sections=", nrSections);

    header->sections = realloc(header->sections, header->nrSections * sizeof(Section));
    for (int i = 0; i < nrSections; i++)
    {
        char sectionName[10] = "";
        read(fd, sectionName, 8);
        snprintf(header->sections[i].name, 10, "%s", sectionName);

        int sectionType = 0;
        read(fd, &sectionType, 4);
        header->sections[i].type = sectionType;

        if ((sectionType != 41) && (sectionType != 71))
        {
            if (parseOrExtr == 0)
                printf("ERROR\nwrong sect_types\n");
            close(fd);
            return 0;
        }

        int sectionOffset = 0;
        read(fd, &sectionOffset, 4);
        header->sections[i].offset = sectionOffset;

        int sectionSize = 0;
        read(fd, &sectionSize, 4);
        header->sections[i].size = sectionSize;
    }
    if (parseOrExtr == 0)
    {
        printf("%s", details);
        for (int i = 0; i < nrSections; i++)
            printf("%s%d: %s %d %d\n", "section", i + 1, header->sections[i].name, header->sections[i].type,
                   header->sections[i].size);
        close(fd);
        return 1;
    }
    close(fd);

    return 1;
}

void extract(char *path, Header header, int section, int lineNr)
{

    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        perror("Could not open input file");
        return;
    }
    lseek(fd, header.sections[section - 1].offset, SEEK_SET);

    if (section > header.nrSections)
    {
        printf("ERROR\ninvalid section\n");
        close(fd);
        return;
    }

    char c;
    int ok = 0;
    int j = 0;
    int i = 0;
    int octeti = 0;
    char *linie = calloc(header.sections[section - 1].size, sizeof(char));

    while (i < lineNr)
    {
        if (ok == 0)
            memset(linie, 0, header.sections[section - 1].size);
        j = 0;
        while (read(fd, &c, 1) > 0 && c != 0x0A)
        {
            if (octeti > header.sections[section - 1].size)
            {
                printf("a %d\n", i);
                ok = 1;
                i++;
                break;
            }
            octeti++;
            linie[j++] = c;
        }
        if (ok == 1)
            break;
        octeti++;
        i++;
    }

    printf("%d", i);

    if (i == lineNr)
    {

        for (int k = 0; k < j / 2; k++)
        {
            char temp = linie[k];
            linie[k] = linie[j - k - 1];
            linie[j - k - 1] = temp;
        }
        printf("SUCCESS\n%s\n", linie);
    }
    else
    {
        printf("ERROR\ninvalid line\n");
    }
    free(linie);
    close(fd);
}

void listAll(const char *path, int lines)
{

    static int succ = 0;
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512] = "";
    struct stat statbuf;
    dir = opendir(path);
    if (dir == NULL)
    {
        perror("Could not open directory");
        return;
    }

    if (succ == 0)
    {
        printf("SUCCESS\n");
        succ = 1;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        memset(fullPath, 0, 512);
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if (lstat(fullPath, &statbuf) == 0)
            {
                if (S_ISREG(statbuf.st_mode))
                {
                    Header header;
                    int valid = parse(fullPath, 1, &header);
                    if (valid != 0)
                        if (header.nrSections > 0)
                        {
                            int fd = open(fullPath, O_RDONLY);
                            for (int i = 0; i < header.nrSections; i++)
                            {
                                lseek(fd, header.sections[i].offset, SEEK_SET);
                                int nrLines = 0;
                                int ok = 0;
                                char *sect = (char *)calloc(header.sections[i].size, sizeof(char));
                                read(fd, sect, header.sections[i].size);

                                for (int j = 0; j < header.sections[i].size; j++)
                                    if (sect[j] == '\n')
                                        nrLines++;
                                if (sect[header.sections[i].size - 1] == '\n')
                                    ok = 1;

                                free(sect);

                                if (ok == 0)
                                {
                                    if (nrLines + 1 > lines)
                                    {
                                        printf("%s\n", fullPath);
                                        break;
                                    }
                                }
                                else
                                {
                                    if (nrLines > lines)
                                    {
                                        printf("%s\n", fullPath);
                                        break;
                                    }
                                }
                            }
                            close(fd);
                        }
                    free(header.sections);
                }
                if (S_ISDIR(statbuf.st_mode))
                {
                    listAll(fullPath, lines);
                }
            }
        }
    }

    closedir(dir);
}

int main(int argc, char **argv)
{
    char arguments[512] = "";
    if (argc >= 2)
    {

        /// VARIANT
        if (strcmp(argv[1], "variant") == 0)
        {
            printf("57535\n");
        }

        /// LIST
        if (strcmp(argv[1], "list") == 0)
        {

            int argSize = 0;
            int argPath = 0;
            int hasPerm = 0;

            for (int i = 2; i < argc; i++)
            {
                strcat(arguments, " ");
                strcat(arguments, argv[i]);
                if (strstr(argv[i], "path"))
                    argPath = i;
                else if (strstr(argv[i], "has_perm_write"))
                    hasPerm = i;
                else if (strcmp(argv[i], "list") != 0 && strcmp(argv[i], "recursive") != 0)
                    argSize = i;
            }

            if (strstr(arguments, "recursive"))
            {
                if (argc == 5)
                {
                    if (hasPerm == 0)
                    {
                        char *val = strTok(argv[argSize]);
                        int size = 0;
                        sscanf(val, "%d", &size);
                        char *path = strTok(argv[argPath]);
                        listRec(path, 1, size, 0);
                    }
                    else
                    {
                        char *path = strTok(argv[argPath]);
                        listRec(path, 0, 0, 1);
                    }
                }
                else if (argc == 4)
                {
                    char *path = strTok(argv[argPath]);
                    listRec(path, 0, 0, 0);
                }
            }
            else if (argc == 4)
            {
                if (hasPerm == 0)
                {
                    char *val = strTok(argv[argSize]);
                    int size = 0;
                    sscanf(val, "%d", &size);
                    char *path = strTok(argv[argPath]);
                    list(path, 1, size, 0);
                }
                else
                {
                    char *path = strTok(argv[argPath]);
                    list(path, 0, 0, 1);
                }
            }
            else if (argc == 3)
            {
                char *path = strTok(argv[argPath]);
                list(path, 0, 0, 0);
            }
        }

        /// PARSE
        if (strcmp(argv[1], "parse") == 0)
        {
            char *path = strTok(argv[2]);
            Header header;
            parse(path, 0, &header);
            free(header.sections);
        }

        /// EXCTRACT
        if (strcmp(argv[1], "extract") == 0)
        {

            int argPath = 0;
            int argSect = 0;
            int argLine = 0;

            for (int i = 2; i < argc; i++)
            {
                strcat(arguments, " ");
                strcat(arguments, argv[i]);
                if (strstr(argv[i], "path"))
                    argPath = i;
                else if (strstr(argv[i], "section"))
                    argSect = i;
                else
                    argLine = i;
            }

            Header header;
            char *path = strTok(argv[argPath]);
            int section = 0;
            char *sec = strTok(argv[argSect]);
            sscanf(sec, "%d", &section);
            int line = 0;
            char *l = strTok(argv[argLine]);
            sscanf(l, "%d", &line);

            parse("M7fLJHF.3la", 1, &header);
            extract("M7fLJHF.3la", header, 2, 15);
            free(header.sections);
        }

        /// FIND ALL
        if (strcmp(argv[1], "findall") == 0)
        {
            char *path = strTok(argv[2]);
            listAll(path, 14);
        }
    }

    return 0;
}
