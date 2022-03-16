/*******************************************************************************
xcursorgen-conf lists PNG files in ascending order inside given directories and
automatically (manual control is optional) assigns hotspot coordinates and dura-
tions to matching image sizes, then saves it as a configuration file to help
generating xcursorgen configuration files. Xcursorgen-conf is particularly use-
ful when creating animated cursors requiring high numbers of images.

xcursorgen config files are built as follow:
size hotspotx hotspoty filepath milliseconds
Example:
64 5 5 mycursor.png 30
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <png.h>
#include <zlib.h>

struct filelist {
    char filename[16385];
    struct filelist *next;
    };

void init_listing(DIR *, char *); //Lists a directory
void init_paths(); //Sets cursor and config paths
void init_man(); //Sets manual override settings
int get_png_size(FILE *); //Figures the width and height of a PNG file. Also makes sure that the file is valid.
struct filelist * newfilelist(char *); //Creates new file list
struct filelist * insertfilelist(struct filelist *, char *); //Insert item in existing list
int countfilelist(struct filelist *); //Counts list elements
void printfilelist(struct filelist *); //List all items on a last
char * listsinglefilelist(struct filelist *, int); //Lists an item based on index
void orderfilelist(struct filelist *); //Orders a file list
void deletefilelist(struct filelist *); //Deletes list
int compareStr(char *, char *); //Compares two strings
const char * lowercase(char *); //Turns a string into all lower case


//Creates global variables
int manx=0, many=0, manm=0, inddir = 0, init_man_count = 0, file_count = 0;
char cursor_path[16385], config_path[16385];
int png_param[100][4]; //SIZE, X, Y, MS
FILE *config;


int main(int argc, char *argv[]) {
    //Captures arguments and saves them in a filelist IN REVERSE ORDER. So, ARG3, ARG2, ARG1
    int arg = 0;
    struct filelist *arguments_list = newfilelist(argv[0]);
    if(argc > 1) {
        for(arg=1; arg<argc; arg++) {
            arguments_list = insertfilelist(arguments_list, argv[arg]);
        }
    } else {
        arg++;
    }

    //Initializes png_param
    int a = 0, b = 0;
    for(b=0; b<4; b++) {
        for(a=0; a < 100; a++) {
            png_param[a][b] = -1;
        }
    }

    //Sets config_path and cursor_path
    init_paths();
    config = fopen(config_path, "w");
    if(!config) {
        printf("\n@ Error creating config file. Quitting.\n\n");
        fclose(config);
        return 1;
    }

    //Initiates working directory variable
    DIR *workdir;
    //Lists directories passed on as arguments, or lists local working directory.
    if(countfilelist(arguments_list) > 1) {
        arg = arg-2;
        while(arg >= 0) {
            workdir = opendir(listsinglefilelist(arguments_list, arg));
            if(workdir) {
                printf("\n###\n### Working on %s\n###\n\n", listsinglefilelist(arguments_list, arg));
                if((inddir == 1 && arg < countfilelist(arguments_list)-1 && arg >= 0) || init_man_count == 0) {
                    init_man();
                }
                init_listing(workdir, listsinglefilelist(arguments_list, arg));
                closedir(workdir);
            } else {
                printf("\n@ Failed to open directory %s\n\n", listsinglefilelist(arguments_list, arg));
            }
            arg--;
        }
    } else {
        workdir = opendir(".");
        if(workdir) {
            printf("###\n### Working on ./\n###\n\n");
            init_man();
            init_listing(workdir, "./");
            closedir(workdir);
        } else {
            printf("\n@ Failed to open ./\n");
        }
    }
    fclose(config);
    deletefilelist(arguments_list);

    //Terminates the program. Offers to run the command automatically for the user.
    if(file_count > 0) {
        printf("\n\n\n     %i files added to configuration. Use the following command to compile:\n\n", file_count);
        printf("          xcursorgen %s %s\n\n", config_path, cursor_path);
        printf("     Do you want to run the command automatically? (Y/N) ");
        char c = '!', ch = '\0';
        int cn = 0;
        while(c != '\0' && c != '\n') {
            scanf("%c", &c);
            cn++;
            if(cn == 1) {
                ch = c;
            }
        }
        if(ch == 'Y' || ch == 'y') {
            printf("\n     Running command...\n\n");
            char execmd[65385];
            strcat(execmd, "xcursorgen ");
            strcat(execmd, config_path);
            strcat(execmd, " ");
            strcat(execmd, cursor_path);
            if(system(execmd) == 0) {
                printf("\n     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
                printf("          Finished! Goodbye!\n");
                printf("     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n\n");
            } else {
                printf("\n\n\n     An error may have occurred with xcursorgen. Possible explanations:\n\n     -The paths contain a space.\n     -Couldn't save the cursor because a file already exists.\n\n");
            }
        } else {
            printf("\n     Goodbye!\n\n");
        }
    } else {
        printf("\n\n\n     No valid files were found. Goodbye!\n\n");
    }
    return 0;
}

//Lists a directory and saves the information to the config file
void init_listing(DIR *workdir, char *path) {

    //Opens directory for listing
    struct dirent *dir;

    //Makes sure the directory path ends with a /
    char workpath[strlen(path)+1];
    if(path[strlen(path)-1] != '/') {
        strcpy(workpath, strcat(path, "/\0"));
    } else {
        strcpy(workpath, path);
    }

    //Creates a filelist and adds all the files in the list
    int item_number = 0;
    struct filelist *dirlist;
    while((dir = readdir(workdir)) != NULL) {
        if(dir->d_type == DT_REG) {
            if(item_number == 0) {
                dirlist = newfilelist(dir->d_name);
            } else {
                dirlist = insertfilelist(dirlist, dir->d_name);
            }
            item_number++;
        }
    }

    //Ends the function if the folder contained no files.
    if(item_number == 0) {
        return;
    }

    //Orders the list in alphabetical order
    orderfilelist(dirlist);

    //Opens each file individually, checks if it's a PNG, and asks to enter details if needed.
    int i = 0, j = 0, k = 0, hy = -1, hx = -1, hm = -1;
    int size = 0;
    FILE *currentfile;
    char currentfilepath[65385];

    for(i=0; i < countfilelist(dirlist); i++) {
        size = 0;
        //Initialize current currentfilepath
        for(j=0; j<65385; j++) {
            currentfilepath[j] = '\0';
        }
        //Building filepath
        strcat(currentfilepath, workpath);
        strcat(currentfilepath, listsinglefilelist(dirlist,i));
        currentfile = fopen(currentfilepath, "rb");
        if(currentfile != NULL) {
            size = get_png_size(currentfile);
            fclose(currentfile);
            if(size > 0) {

                //Checks if size is recorded, and records it if it is not. Sets j to the right array.
                j = 0;
                while(j<100 && size != png_param[j][0]) {
                    j++;
                }
                if(j > 99) {
                    //New size
                    j = 0;
                    while(png_param[j][0] != -1) {
                        j++;
                    }
                    png_param[j][0] = size;
                }

                //Tells the user what file is being worked on
                printf("%ipx FILE: %s\n", size, currentfilepath);

                //Checks for manual settings and for unset settings.
                //Needs to loop input characters
                char c = '!';
                char inputnb[21];
                int inputn;
                //Checking for Hotspot X
                //Initializes variables
                inputn = 0;
                for(k=0; k<21; k++) {
                    inputnb[k] = '\0';
                }
                if(png_param[j][1] < 0 || manx == 1) {
                    hx = -1;
                    printf("     HOTSPOT X: ");
                    c = '!';
                    while(c != '\0' && c != '\n') {
                        scanf("%c", &c);
                        if(inputn < 20) {
                            inputnb[inputn] = c;
                        }
                        inputn++;
                    }
                    inputnb[inputn] = '\0';
                    hx = atoi(inputnb);
                    png_param[j][1] = hx;
                }
                //Checking for Hotspot Y
                //Initializes variables
                inputn = 0;
                for(k=0; k<21; k++) {
                    inputnb[k] = '\0';
                }
                if(png_param[j][2] < 0 || many == 1) {
                    hy = -1;
                    printf("     HOTSPOT Y: ");
                    c = '!';
                    while(c != '\0' && c != '\n') {
                        scanf("%c", &c);
                        if(inputn < 20) {
                            inputnb[inputn] = c;
                        }
                        inputn++;
                    }
                    inputnb[inputn] = '\0';
                    hy = atoi(inputnb);
                    png_param[j][2] = hy;
                }
                //Checking for duration
                //Initializes variables
                inputn = 0;
                for(k=0; k<21; k++) {
                    inputnb[k] = '\0';
                }
                if(png_param[j][3] < 0 || manm == 1) {
                    hm = -1;
                    printf("     DURATION (MS): ");
                    c = '!';
                    while(c != '\0' && c != '\n') {
                        scanf("%c", &c);
                        if(inputn < 20) {
                            inputnb[inputn] = c;
                        }
                        inputn++;
                    }
                    inputnb[inputn] = '\0';
                    hm = atoi(inputnb);
                    png_param[j][3] = hm;
                }

                //PRINTING THE RESULT TO THE CONFIG FILE!
                fprintf(config, "%i %i %i %s %i\n", png_param[j][0], png_param[j][1], png_param[j][2], currentfilepath, png_param[j][3]);
                file_count++;

            }
        }
    }

    return;
}

//Asks and sets cursor_path and config_path
void init_paths() {
    char c = '\0';
    int i = 0;

    //Initializes cursor_path and config_path
    for(i=0; i<= 16385; i++) {
        cursor_path[i] = '\0';
    }
    for(i=0; i<= 16385; i++) {
        config_path[i] = '\0';
    }

    //Asks for cursor_path
    printf("\nEnter the name or path of the cursor you want to create: ");
    int n = 0;
    c = '!';
    while( c != '\0' && c != '\n') {
        scanf("%c", &c);
        if(c != '\n' && c != '\0' && n < 16384) {
            cursor_path[n] = c;
            n++;
        } else {
            n++;
            if(n<=16385) {
                cursor_path[n] = '\0';
            }
        }
    }
    if(strlen(cursor_path) == 0) {
        strcpy(cursor_path, "default_cursor");
        printf("\nCursor path set to %s\n\n", cursor_path);
    } else {
        printf("\n");
    }

    //Asks for config_path
    printf("Enter the name or path of the configuration file you want to create: ");
    n = 0;
    c = '!';
    while( c != '\0' && c != '\n') {
        scanf("%c", &c);
        if(c != '\n' && c != '\0' && n < 16384) {
            config_path[n] = c;
            n++;
        } else {
            n++;
            if(n<=16385) {
                config_path[n] = '\0';
            }
        }
    }
    if(strlen(config_path) == 0) {
        strcpy(config_path, "default_config");
        printf("\nConfig path set to %s\n\n", config_path);
    } else {
        printf("\n");
    }
    return;
}

//Asks and sets manual override options
void init_man() {
    //Asks for options to manually override settings
    printf("Hotspots and durations will only be asked once and applied to every image of matching size.\n\n");
    if(manx == 1) {
        printf("(Z) Set Hotspot X to automatic\n");
    } else if(manx == 0) {
        printf("(X) Set Hotspot X to manual\n");
    }
    if(many == 1) {
        printf("(T) Set Hotspot Y to automatic\n");
    } else if(many == 0) {
        printf("(Y) Set Hotspot Y to manual\n");
    }
    if(manm == 1) {
        printf("(N) Set duration to automatic\n");
    } else if(manm == 0) {
        printf("(M) Set duration to manual\n");
    }
    if(inddir == 1) {
        printf("(C) Cancel asking for each directory.\n");
    } else if(inddir ==0) {
        printf("(D) Ask again for each directory\n");
    }
    printf("\nPlease enter which settings you would like to adjust manually: ");
    char c = '!';
    while(c != '\0' && c != '\n') {
        scanf("%c", &c);
        if(c == 'X' || c == 'x') {
            manx = 1;
        } else if (c == 'Z' || c == 'z') {
            manx = 0;
        } else if (c == 'Y' || c == 'y') {
            many = 1;
        } else if (c == 'T' || c == 't') {
            many = 0;
        } else if (c == 'M' || c == 'm') {
            manm = 1;
        } else if (c == 'N' || c == 'n') {
            manm = 0;
        } else if (c == 'D' || c == 'd') {
            inddir = 1;
        } else if (c == 'C' || c == 'c') {
            inddir = 0;
        }
    }
    init_man_count++;
    return;
}

//Figures the width and height of a PNG file. Also makes sure that the file is valid.
int get_png_size(FILE *pngfile) {
    unsigned char sig[8];
    fread(sig, 1, 8, pngfile);
    if(!png_check_sig(sig, 8)){
        return -1;
    }
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr) {
        return -1;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return -1;
    }
    png_uint_32 pwidth = 0, pheight = 0;
    png_uint_32 *ptr_pwidth = &pwidth, *ptr_pheight = &pheight;
    png_init_io(png_ptr, pngfile);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, ptr_pwidth, ptr_pheight, NULL, NULL, NULL, NULL, NULL);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    if(pwidth < 1 || pheight < 1) {
        return -1;
    }
    if(pwidth != pheight) {
        return -1;
    }

    return (int) pwidth;
}

//Creates new file list
struct filelist * newfilelist(char *item) {
    struct filelist *filelist_ptr;
    filelist_ptr = malloc(sizeof(struct filelist));
    strcpy(filelist_ptr->filename, item);
    filelist_ptr->next = NULL;
    return filelist_ptr;
}

//Insert item in existing list
struct filelist * insertfilelist(struct filelist *list_ptr, char *item) {
    struct filelist *new = newfilelist(item);
    strcpy(new->filename, item);
    new->next = list_ptr;
    return new;
}

//Counts list elements
int countfilelist(struct filelist *list_ptr) {
    int n = 0;
    struct filelist *current = list_ptr;
    struct filelist *next = current->next;
    while(current != NULL) {
        current = next;
        n++;
        if (current != NULL) {
            next = current->next;
        }
    }
    return n;
}

//Show all items on a list
void printfilelist(struct filelist *list_ptr) {
    struct filelist *current = list_ptr;
    struct filelist *next = list_ptr->next;
    while(current != NULL) {
        printf("%s\n", current->filename);
        current = next;
        if(current != NULL) {
            next = current->next;
        }
    }
    return;
}

//Lists an item based on index
char * listsinglefilelist(struct filelist *list_ptr, int index) {
    int i = 0;
    struct filelist *current = list_ptr;
    struct filelist *next = current->next;
    for(i=0; i<index; i++) {
        current = next;
        if(current != NULL) {
            next = current->next;
        }
    }
    return current->filename;
}

//Orders a file list
void orderfilelist(struct filelist *list_ptr) {
    if(countfilelist(list_ptr) < 2) {
        return;
    }

    struct filelist *current = list_ptr;
    struct filelist *next = current->next;
    char swap[16385];

    int stop = 0, count = 0, size = countfilelist(list_ptr);
    while(stop == 0){
        stop = 1;
        for(count=0; count < size-1; count++) {
            if(compareStr(current->filename, next->filename) == 2) {
                strcpy(swap,current->filename);
                strcpy(current->filename,next->filename);
                strcpy(next->filename,swap);
                stop = 0;
            }
            current = next;
            next = current->next;
        }
        current = list_ptr;
        next = current->next;
    }
    return;
}

//Deletes list
void deletefilelist(struct filelist *list_ptr) {
    struct filelist *current = list_ptr;
    struct filelist *next = current->next;
    while(current != NULL) {
        current->next = NULL;
        free(current);
        current = next;
        if(current != NULL) {
            next = current->next;
        }
    }
    return;
}

//Alphabetically compares strings A and B. 0 means both strongs are the same. 1 means string A comes first. 2 means strong B comes first.
int compareStr(char *a, char *b) {
    int i = 0;
    int j = 0;
    char a1[1001], b1[1001];
    strcpy(a1, lowercase(a));
    strcpy(b1, lowercase(b));
    while (a1[i] == b1[i] && (a1[i] != '\0' && b1[i] != '\0')) {
        i++;
    }
    if (a1[i] == b1[i]) {
        while (a[j] == b[j] && (a[j] != '\0' && b[j] != '\0')) {
            j++;
        }
        if (a[j] == b[j]) {
            return 0;
        } else if (a[j] < b[j]) {
            return 1;
        } else if (a[j] > b[j]) {
            return 2;
        }
    } else if (a1[i] < b1[i]) {
        return 1;
    } else if (a1[i] > b1[i]) {
        return 2;
    }
    return -1;
}

//Turns a string into all lower case
const char * lowercase(char *word) {
    char letters[2][26] = {
        {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'},
        {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'}
    };
    char *lowerword;
    lowerword = malloc(sizeof(char)*1001);
    int l = 0, w = 0, chg = 0;
    while(word[w] != '\0' && word[w]) {
        for(l = 0; l < 26; l++) {
            if(word[w] == letters[0][l]) {
                chg = 1;
                break;
            }
        }
        if(chg == 1) {
            lowerword[w] = letters[1][l];
            chg = 0;
        } else {
            lowerword[w] = word[w];
        }
        w++;
    }
    //word[w] = '\0';
    return lowerword;
}
