/*
  Copyright 2007 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/*
  syntax:  bin2c <input_file1> <input_file2> ...
  output: C arrays of input-files to stdout

  Based on bin2c.c written by Sandro Sigala. Original file header is below:
*/

// bin2c.c
//
// convert a binary file into a C source vector
//
// THE "BEER-WARE LICENSE" (Revision 3.1415):
// sandro AT sigala DOT it wrote this file. As long as you retain this notice you can do
// whatever you want with this stuff.  If we meet some day, and you think this stuff is
// worth it, you can buy me a beer in return.  Sandro Sigala
//
// syntax:  bin2c [-c] [-z] <input_file> <output_file>
//
//          -c    add the "const" keyword to definition
//          -z    terminate the array with a zero (useful for embedded C strings)
//
// examples:
//     bin2c -c myimage.png myimage_png.cpp
//     bin2c -z sometext.txt sometext_txt.cpp

 #include <ctype.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>

 #ifndef PATH_MAX
 #define PATH_MAX 1024
 #endif

 void process(const char *ifname, char **array_name) {
    FILE *ifile;
    ifile = fopen(ifname, "rb");
    if (ifile == NULL) {
        fprintf(stderr, "cannot open %s for reading\n", ifname);
        exit(1);
    }

    // prepare array name
    char *p, *buf;
    const char *cp;
    if ((cp = strrchr(ifname, '/')) != NULL)
        ++cp;
    else {
        if ((cp = strrchr(ifname, '\\')) != NULL)
            ++cp;
        else
            cp = ifname;
    }
    *array_name = buf = new char[strlen(cp) + 1];
    strcpy(buf, cp);
    for (p = buf; *p != '\0'; ++p)
        if (!isalnum(*p))
            *p = '_';

    printf("static const char %s[] = {\n  ", buf);
    int c, col = 1;
    bool newline = true;
    while ((c = fgetc(ifile)) != EOF) {
        if (col >= 78 - 6) {
            printf("\n  ");
            //putchar('\n');
            col = 1;
        }

        printf("0x%.2x, ", c);
        col += 6;

    }
    printf("\n};\n\n");

    fclose(ifile);
 }

 int strcompare(const void *s1, const void *s2) {
   return strcmp(*(char **)s1, *(char **)s2);
 }

 void print_header(int argc, char **argv, char **array_names) {
   char **argv_copy = new char*[argc];
   for (int i = 1; i < argc; i++) { // 0th element is left blank
     argv_copy[i] = new char[PATH_MAX];
     snprintf(argv_copy[i], PATH_MAX, "\"%s\", sizeof(%s), %s",
              argv[i], array_names[i], array_names[i]);
   }
   qsort(argv_copy + 1, argc - 1, sizeof(char *), strcompare);

   printf("static const Resource kResourceList[] = {\n");
   for (int i = 1; i < argc; i++) {
     printf("  {%s},\n", argv_copy[i]);
   }
   printf("};\n");

   for (int i = 1; i < argc; i++) { // 0th element is left blank
     delete[] argv_copy[i];
     argv_copy[i] = NULL;
   }
   delete[] argv_copy;
   argv_copy = NULL;
 }

 void usage() {
    fprintf(stderr, "usage: bin2c <input_file1> <input_file2> ...\n");
    exit(1);
 }

 int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
    }

    char **array_names = new char*[argc];    
    for (int i = 1; i < argc; i++) {
      process(argv[i], array_names + i);
    }

    print_header(argc, argv, array_names);    

    for (int i = 1; i < argc; i++) { // 0th element is left blank
      delete[] array_names[i];
      array_names[i] = NULL;
    }
    delete[] array_names;
    array_names = NULL;
    return 0;
 }
