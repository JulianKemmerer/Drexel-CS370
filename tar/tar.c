#include <stdio.h>
#include <stdlib.h>


#define MAX_FILES 50
#define MAX_FILE_SIZE 1000

struct posix_header 
{                              /* byte offset */
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */
                                /* 500 */
};

struct tar_file {
    struct posix_header header;
    char  contents[MAX_FILE_SIZE];
    int content_end;
} typedef tar_file;

tar_file files[MAX_FILES];
int file_count;

void read_files(FILE * fp) {
    /**
     * Get all the meta data for the files and the content
     */
    int offset = 0;
    int header_offset = 500;
    int c; // Current character being read
    int count_nulls = 0;
    file_count = 0;
    int file_started = 0;
            
    do { 
        c = getc(fp);
        // New header
        struct posix_header header;
        if(offset < header_offset) {
            // Load in the headers based on their offset 
               
            if(offset < 100) {
                file_started = 1;
                header.name[offset] = c;
            }
            else if(offset < 108)
                header.mode[offset - 100] = c;
            else if(offset < 116)
                header.uid[offset - 108] = c;
            else if(offset < 124)
                header.gid[offset - 116] =  c;
            else if(offset < 136)
                header.size[offset - 124] = c;
            else if(offset < 148)
                header.mtime[offset - 136] = c;
            else if(offset < 156)
                header.chksum[offset - 148] = c;
            else if(offset < 157)
                header.typeflag = c;
            else if(offset < 257)
                header.linkname[offset - 157] = c;
            else if(offset < 263)
               header.magic[offset - 257] = c;
            else if(offset < 265)
                header.version[offset - 263] = c;
            else if(offset < 297)
                header.uname[offset - 265] = c;
            else if(offset < 329)
                header.gname[offset - 297] = c;
            else if(offset < 337)
                header.devmajor[offset - 329] = c;
            else if(offset < 345)
                header.devminor[offset - 337] = c;
            else
                header.prefix[offset - 345] = c;
        }
        else if (offset == header_offset){
            // We've completed one entire file header
            tar_file f;
            f.header = header;
            f.content_end = 0;
            files[file_count] = f;
            file_count++;
        } else { 
            // Since we've completed a file, the rest must be the file's content until we reach enough the null delimiter
            if(c){
                // Read the character into the file's contents
                tar_file f;
                f = files[file_count - 1];
                if(f.content_end < MAX_FILE_SIZE) {
                    // As long as we have room
                    f.contents[f.content_end] = c;
                    f.content_end++;
                    files[file_count - 1] = f;
                }
            }
        }
        if(!c) {
            // If we have a null character, we need to increment the number of nulls.
            // This will get reset each time we hit a character that is not a null
            count_nulls++;
        } else {
            if(count_nulls >= 493) { // This is basically the "magic number" of nulls we need to declare the end of a file's contents
                // We are now resetting the loop
                offset = 0;
                file_started = 0;
                // Unget because we used "c" when it wasn't null, meaning it was probably the first letter of the next file
                ungetc(c, fp);
            } 
            // If we hit a non null
            count_nulls = 0;
            }
        if(file_started == 1)
            offset++;
    } while (c != EOF);
}

void print_file_data() {
    int i;
    for(i = 0; i < file_count; i++) {
            tar_file f;
            f = files[i];
            struct posix_header header = f.header;
            printf("Name: %s\n", header.name);
            printf("Mode: %s\n", header.mode);
            printf("UID: %s\n", header.uid);
            printf("GID : %s\n", header.gid);
            printf("Size: %s\n", header.size);
            printf("MTime: %s\n", header.mtime);
            printf("Checksum: %s\n", header.chksum);
            //printf("Typeflag: %c\n", header.typeflag);
            //printf("Link Name: %s\n", header.linkname);
            //printf("Magic: %s\n", header.magic);
            //printf("Version: %s\n", header.version);
            printf("UName: %s\n", header.uname);
            printf("GName: %s\n", header.gname);
            //printf("DevMajor: %s\n", header.devmajor);
            //printf("DevMinor: %s\n", header.devminor);
            //printf("Prefix: %s\n", header.prefix);
            printf("\n");
    }
}

void extract_files() {

    printf("Extracting files:\n");
    int i;
    for(i = 0; i < file_count; i++) {
            tar_file f;
            f = files[i];
            struct posix_header header;
            header = f.header;
            FILE * fp;
            fp = fopen(header.name, "w");
            //Rather than printing the contents as a string
            //(lacks a null terminator)
            //Print a single char into the file at a time
            int j;
            for(j = 0; j < (f.content_end-1); j++)
            {
				fprintf(fp,"%c",f.contents[j]);
			}
            fclose(fp);
            printf("\t%s\n", header.name);
    }
};

int main(int argc, char *argv[]) {
    if(argc > 1){
      FILE * fp;
      fp = fopen(argv[1], "r");
      if(fp == NULL) {
          printf("File does not exist!\n");
          exit(EXIT_FAILURE);
      }
      read_files(fp);
      print_file_data();
      extract_files();
      fclose(fp);
      exit(EXIT_SUCCESS);
    } else {
        printf("Usage: ./tar filename\n");
        exit(EXIT_FAILURE);
    }
}
