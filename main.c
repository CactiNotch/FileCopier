//#include <stdio.h>
//#include <stdlib.h>
//
////void fileSourceError();
////void fileDestError();
//
//int main(int argc, char *argv[])
//{
//    // If the number of parameters (arguments) passed when calling the file is not 3, provide a help response
//    // When the program is executed the params "fileName" (default param), "source_file" and "destination_file" should be provided
//	if (argc != 3) {
//        printf("Usage: %s <source_file> <destination_file>\n", argv[0]);
//        return 1;
//    }
//    
//    // Open the source file specified by the first command-line argument in read-binary mode
//    FILE *source_file = fopen(argv[1], "rb");
//    if (source_file == NULL)
//    {
//        perror("Error opening source file");
//        return 1;
//    }    
//    
//    // Open the destination file specified by the second command-line argument in write-binary mode
//    FILE *dest_file = fopen(argv[2], "wb");
//    if (dest_file == NULL) 
//    {
//        perror("Error opening destination file");
//        fclose(source_file);
//        return 1;
//    }
//    
//    char buffer[1024];
//    size_t bytes_read;
//    
//    while((bytes_read = fread(buffer, 1, sizeof(buffer), source_file)) > 0)
//    {
//        fwrite(buffer, 1, bytes_read, dest_file);
//    }
//    
//    fclose(source_file);
//    fclose(dest_file);
//    
//	return 0;
//}

//void fileError()
//{
//    perror("Error opening source file");
//}
//
//void fileDestError()
//{
//    perror("Error opening destination file
//}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
//#include <dirent.h>
#include <windows.h>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(dir) _mkdir(dir)
#else
    #include <sys/types.h>
    #include <sys/stat.h>
    #define MKDIR(dir) mkdir(dir, 0755)
#endif

#define BUFFER_SIZE 4096
#define MAX_PATH 260
#define MAX_FILES 100

void print_usage(const char* program_name) {
    printf("Usage:\n");
    printf("Single file: %s -f \"<source_file>\" \"<destination_path>\"\n", program_name);
    printf("Multiple files: %s -m \"<source_file1>\" \"<source_file2>\" ... \"<destination_directory>\"\n", program_name);
    printf("Entire directory: %s -d \"<source_directory>\" \"<destination_directory>\"\n", program_name);
    printf("Note: Use quotes around paths containing spaces\n");
}

char* get_filename_from_path(const char* path) {
    const char* last_slash = strrchr(path, '\\');
    if (last_slash == NULL) {
        last_slash = strrchr(path, '/');
    }
    return last_slash ? (char*)(last_slash + 1) : (char*)path;
}

// Function to check if directory exists, otherwise create it
int ensure_directory_exists(const char* path) {
    if (MKDIR(path) == 0 || errno == EEXIST) {
        return 0;
    }
    return -1;
}

int copy_file(const char* source_path, const char* dest_path) {
    FILE *source = NULL, *dest = NULL;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    int success = 0;
    char full_dest_path[MAX_PATH];
    struct stat path_stat;
        
    // Check if destination is a directory
//    stat(dest_path, &path_stat);
    if (stat(dest_path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
        // If destination is a directory, append the source filename
        snprintf(full_dest_path, MAX_PATH, "%s\\%s", 
                dest_path, 
                get_filename_from_path(source_path));
    } else {
        strncpy(full_dest_path, dest_path, MAX_PATH);
    }
    
    // Open source file for reading
    source = fopen(source_path, "rb");
    if (!source) {
        fprintf(stderr, "Error opening source file: %s\n", strerror(errno));
        return 1;
    }

    // Open destination file for writing
    dest = fopen(full_dest_path, "wb");
    if (!dest) {
        fprintf(stderr, "Error opening destination file: %s\n", strerror(errno));
        fclose(source);
        return 1;
    }

    // Copy file contents
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, source)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dest) != bytes_read) {
            fprintf(stderr, "Error writing to destination file: %s\n", strerror(errno));
            success = 1;
            break;
        }
    }

    // Check for read error
    if (ferror(source)) {
        fprintf(stderr, "Error reading from source file: %s\n", strerror(errno));
        success = 1;
    }

    // Close files
    fclose(source);
    fclose(dest);

    if (success == 0) {
//        printf("Successfully copied: %s\n", get_filename_from_path(source_path));
        printf("Successfully copied:\nFrom: %s\nTo: %s\n", source_path, full_dest_path);
    }

    return success;
}

int copy_multiple_files(int file_count, char* source_files[], const char* dest_dir) {
    struct stat path_stat;
    int success = 0;
    int files_copied = 0;
    int errors = 0;

    // Verify destination is a directory
    if (stat(dest_dir, &path_stat) != 0 || !S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, "Destination must be a directory for multiple file copy\n");
        return 1;
    }

    // Copy each file
    for (int i = 0; i < file_count; i++) {
        // Verify source file exists
        if (GetFileAttributes(source_files[i]) == INVALID_FILE_ATTRIBUTES) {
            fprintf(stderr, "Source file does not exist: %s\n", source_files[i]);
            errors++;
            continue;
        }

        // Copy the file
        if (copy_file(source_files[i], dest_dir) == 0) {
            files_copied++;
        } else {
            errors++;
        }
    }

    printf("Multiple file copy complete. Files copied: %d, Errors: %d\n", files_copied, errors);
    return errors > 0 ? 1 : 0;
}

int copy_directory(const char* source_dir, const char* dest_dir) {
    WIN32_FIND_DATA find_data;
    HANDLE find_handle;
    char search_path[MAX_PATH];
    char source_file_path[MAX_PATH];
    char dest_file_path[MAX_PATH];
    int files_copied = 0;
    int errors = 0;

    // Ensure destination directory exists
    if (ensure_directory_exists(dest_dir) != 0) {
        fprintf(stderr, "Error creating/finding destination directory: %s\n", dest_dir);
        return 1;
    }

    // Create search path for FindFirstFile
    snprintf(search_path, MAX_PATH, "%s\\*", source_dir);

    // Start finding files
    find_handle = FindFirstFile(search_path, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error accessing directory: %s\n", source_dir);
        return 1;
    }

    do {
        // Skip . and .. directories
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }

        // Construct full source and destination paths
        snprintf(source_file_path, MAX_PATH, "%s\\%s", source_dir, find_data.cFileName);
        snprintf(dest_file_path, MAX_PATH, "%s\\%s", dest_dir, find_data.cFileName);

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Recursively copy subdirectories
            if (copy_directory(source_file_path, dest_file_path) == 0) {
                printf("Copied directory: %s\n", find_data.cFileName);
            } else {
                errors++;
            }
        } else {
            // Copy individual file
            if (copy_file(source_file_path, dest_file_path) == 0) {
                files_copied++;
            } else {
                errors++;
            }
        }
    } while (FindNextFile(find_handle, &find_data));

    FindClose(find_handle);

    printf("Directory copy complete. Files copied: %d, Errors: %d\n", files_copied, errors);
    return errors > 0 ? 1 : 0;
}

int main(int argc, char *argv[]) {
    // Check arguments
    if (argc < 4) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Handle different modes
    if (strcmp(argv[1], "-f") == 0 && argc == 4) {
        // Single file copy mode
        if (GetFileAttributes(argv[2]) == INVALID_FILE_ATTRIBUTES) {
            fprintf(stderr, "Source file '%s' does not exist\n", argv[2]);
            return 1;
        }
        return copy_file(argv[2], argv[3]);
    } 
    else if (strcmp(argv[1], "-m") == 0 && argc >= 5) {
        // Multiple files copy mode
        int file_count = argc - 3;  // Subtract program name, flag, and destination
        char* destination = argv[argc - 1];  // Last argument is destination
        
        // Check if we exceed maximum file limit
        if (file_count > MAX_FILES) {
            fprintf(stderr, "Too many files specified. Maximum is %d\n", MAX_FILES);
            return 1;
        }
        
        return copy_multiple_files(file_count, &argv[2], destination);
    }
    else if (strcmp(argv[1], "-d") == 0 && argc == 4) {
        // Directory copy mode
        if (GetFileAttributes(argv[2]) == INVALID_FILE_ATTRIBUTES) {
            fprintf(stderr, "Source directory '%s' does not exist\n", argv[2]);
            return 1;
        }
        return copy_directory(argv[2], argv[3]);
    } 
    else {
        fprintf(stderr, "Invalid arguments\n");
        print_usage(argv[0]);
        return 1;
    }
}