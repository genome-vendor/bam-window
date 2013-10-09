/*
 * bamwindow.c 
 * Copyright (c) 2012 WUGSC All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <crypt.h>
#include "sam.h"
#include "faidx.h"
#include "khash.h"

#define MAX_TOKEN_LEN 100
#define MAX_LINE_LEN 2000
#define MAX_RGLIBS 200
#define TOP_LINES_FOR_READ_LENGTH 1000000

// Hash type for read groups
KHASH_MAP_INIT_STR(str, int);

// Struct to store info to be passed around
typedef struct 
{
    int min_mapq;   //minimum mapping qualitiy to use
    int beg, end;    //start and stop of region
    int num_read_lengths;
    int read_lengths[1024];
    int other_read_lengths[1024];
    int num_other_read_lengths;

    int only_paired;
    int only_properly_paired;
    int only_use_start_site;

    samfile_t *in;  //bam file

    unsigned long current_window_reads[1024];
    long int other_read_lengths_count[1024];

    float downratio;

    char libgroups[4096];
    char read_lengths_string[1024];

    khash_t(str) *rghash;
    khash_t(str) *lengthhash;

    // for version 0.4
    int dolibgroups;
    int doreadlength;

} pileup_data_t;

// Since this list is small, i'm not even bothering to use a hash
int existsInArray(int val, int a[], int alen)
{
    int i;

    for (i=0; i<alen; i++)
    {
        if (a[i] == val)
        {
            return(1);
        }
    }

    return(0);
}

int getIndex(int val, int a[], int alen)
{
    int i;

    for (i=0; i<alen; i++)
    {
        if (a[i] == val)
        {
            return(i);
        }
    }

    return(-1);
}

// For sorting
int intcmp(const void *aa, const void *bb)
{
    const int *a = aa;
    const int *b = bb;

    return (*a < *b) ? -1 : (*a > *b);
}

int uniq(int a[], int array_size)
{
    int i;
    int j = 0;
    // Remove the duplicates
    for (i = 1; i < array_size; i++)
    {

        if (a[i] != a[j])
        {
            j++;
            a[j] = a[i]; // Move it to the front
        }

    }
    // The new array size
    array_size = (j + 1);
    // Return the new size
    return(j + 1);
}

// How many lines
int countChNUM(char *string, char c)
{
    int x;
    int z = 0;
    int length = strlen(string);

    for (x = 0; x < length; x++)
    {
        if (string[x] == c)
        {
            z++;
        }
    }

    return(z);

}

// Separate string based on given char 
int separateString(char *string, char c, char **container)
{
    int z = 0;
    int x, y = 0;
    int tokenNum = 0;
    int length = strlen(string);
    char buff1[MAX_LINE_LEN];
    char array[MAX_LINE_LEN];

    for (x = 0; x < length; x++)
    {
        if (string[x + 1] == '\0')
        {
            buff1[z] =  string[x];
            buff1[z + 1] =  '\0';

            if (strlen(buff1))
            {             
                strcpy(array, buff1);
                strcpy(container[y], buff1);
                //fprintf(stderr, "%s\n", array);
                tokenNum++;
            }
        }
        else if (string[x] != c)
        {
            buff1[z] =  string[x];
            z++;

            if (string[x + 1] == c)
            {
                buff1[z] = '\0';
                z = 0;

                if (strlen(buff1))
                {
                    strcpy(array, buff1);
                    strcpy(container[y], buff1);
                    y++;
                    //fprintf(stderr, "%s\n", array);
                    tokenNum++;
                }

            }
        }
    }

    return(tokenNum);

}

// Parse RG lines
int parseRGlines(char *pBamHeader, char **LBs, char **RGs)
{

    // Grab RGid and Lib infor
    int lineNum;
    int rglineNum;
    int tokenNum;

    int iter0;
    int iter1;
    int iter2;

    const char *rgHead  = "@RG";
    const char *rgID    = "ID:";
    const char *markLIB = "LB:";

    char headTemp[4];
    char headTemp0[4];
    char **bamHeadLines;
    char **RgLineContent;
    char conTemp[MAX_TOKEN_LEN];
    char conTemp1[MAX_TOKEN_LEN];

    lineNum = countChNUM(pBamHeader, '\n');
    //fprintf(stderr," lineNum: %d \n", lineNum);
    // Memmory allocation
    bamHeadLines = (char **)malloc((lineNum+1)*sizeof(char *));

    for (iter0=0; iter0<=lineNum; iter0++)
    {
        bamHeadLines[iter0] = (char *)malloc((MAX_LINE_LEN)*sizeof(char));
    }

    separateString(pBamHeader, '\n', bamHeadLines);
    iter1 = 0;
    rglineNum = 0;

    for (iter0=0; iter0<lineNum; iter0++)
    {
        //fprintf(stderr," %s \n", bamHeadLines[iter0]);
        strncpy(headTemp, bamHeadLines[iter0], 3);
        headTemp[3] = '\0';

        if (!strcmp(headTemp, rgHead))
        {
            rglineNum++;
            tokenNum = countChNUM(bamHeadLines[iter0], '\t');

            if (tokenNum > iter1)
            {
                iter1 = tokenNum;
            }
        }
    }

    tokenNum = iter1;
    RgLineContent = (char **)malloc((tokenNum+1)*sizeof(char *));

    for (iter0=0; iter0<=tokenNum; iter0++)
    {
        RgLineContent[iter0] = (char *)malloc((MAX_TOKEN_LEN)*sizeof(char));
    }
    iter2 = 0;

    for (iter0=0; iter0<lineNum; iter0++)
    {
        strncpy(headTemp, bamHeadLines[iter0], 3);
        headTemp[3] = '\0';

        if (!strcmp(headTemp, rgHead))
        {
            separateString(bamHeadLines[iter0], '\t', RgLineContent);

            for (iter1=0; iter1<tokenNum; iter1++)
            {
               strncpy(headTemp0, RgLineContent[iter1], 3);
               headTemp0[3] = '\0';

               if (!strcmp(headTemp0, rgID))
               {
                   strncpy(conTemp1, RgLineContent[iter1]+3, strlen(RgLineContent[iter1]));
               }

               if (!strcmp(headTemp0, markLIB))
               {
                   strncpy(conTemp, RgLineContent[iter1]+3, strlen(RgLineContent[iter1]));
               }
            }
            strcpy(LBs[iter2], conTemp);
            strcpy(RGs[iter2], conTemp1);
            iter2++;
        }
    }

    // Return memory
    for (iter0=0; iter0<=tokenNum; iter0++)
    {
        free(RgLineContent[iter0]);
    }
    free(RgLineContent);

    for (iter0=0; iter0<=lineNum; iter0++)
    {
        free(bamHeadLines[iter0]);
    }
    free(bamHeadLines);

    return rglineNum;

}

// Grab read lengths
int grabReadLens(char *fn, int *lenscon, int num)
{    
    int r;
    int iter = 0;

    samfile_t *in;

    in  = samopen(fn, "rb", 0);
    bam1_t *bb = bam_init1();

    while ((r = samread(in, bb)) >= 0)
    {
        lenscon[iter] = bb->core.l_qseq;
        iter++;

        if (iter >= num)
        {
            break;
        }
    } 

    samclose(in);

    return 0;
}

// Fast sorting
typedef int T;
void quicksort(T *data, int N)
{
    int i, j;
    T v, t;

    if (N <= 1) return;
    // Partition elements

    v = data[0];
    i = 0;
    j = N;

    for (;;)
    {
        while (data[++i] < v && i < N){ }
        while (data[--j] > v) { }

        if (i >= j) break;

        t = data[i];

        data[i] = data[j];
        data[j] = t;
    }

    t = data[i-1];

    data[i-1] = data[0];
    data[0] = t;

    quicksort(data, i-1);
    quicksort(data+i, N-i);

}

// callback for bam_fetch()
static int fetch_func(const bam1_t *b, void *data)
{
    char tmp[1024];
    char rgroup[1024];

    khiter_t key;
    pileup_data_t *d = (pileup_data_t*) data;

    // drop this read
    if ((double)(rand()/((double)RAND_MAX + 1)) > d->downratio)
    { 
        return 0;
    }

    if (d->only_use_start_site && (b->core.pos<d->beg || b->core.pos>=d->end))
    { 
        return 0;
    }

    if (b->core.qual >= d->min_mapq)
    {
        if ((d->only_paired || d->only_properly_paired) && !(b->core.flag & BAM_FPAIRED))
        { 
            return 0;
        }

        if (d->only_properly_paired && !(b->core.flag & BAM_FPROPER_PAIR))
        { 
            return 0;
        }

        if (d->dolibgroups == 0)
        {
            if (d->doreadlength == 0)
            {
                d->current_window_reads[0]++;

            }
            else
            {
                // based on read length
                sprintf(tmp, "%i", b->core.l_qseq);
                key = kh_get(str, d->lengthhash, tmp);

                if (!(key == kh_end(d->lengthhash)))
                {
                    d->current_window_reads[kh_val(d->lengthhash, key)]++;
                }
            }
        }
        else
        {
            // have to deal with readgroups
            strcpy(rgroup, bam_aux2Z(bam_aux_get(b, "RG")));

            if (d->doreadlength)
            {
                sprintf(tmp,"%i",b->core.l_qseq);
                strcat(rgroup,".");
                strcat(rgroup,tmp);

            }

            key = kh_get(str, d->rghash, rgroup);

            if (key == kh_end(d->rghash))
            {
                // is this stored in our other_rl array, if so just increment
                if (existsInArray(b->core.l_qseq, d->other_read_lengths, d->num_other_read_lengths))
                { 
                    d->other_read_lengths_count[getIndex(b->core.l_qseq, d->other_read_lengths, d->num_other_read_lengths)]++;
                }
                else
                {
                   // if it's an input read length, then the lib name is wrong
                   if (existsInArray(b->core.l_qseq, d->read_lengths, d->num_read_lengths))
                   {
                       fprintf(stderr,"wack key: %s not found\n", rgroup);
                   }
                   else
                   { 
                        // must be a new read length
                        d->other_read_lengths[d->num_other_read_lengths] = b->core.l_qseq;
                        d->other_read_lengths_count[d->num_other_read_lengths++] = 1;
                   }
                }
            }
            else
            {
                d->current_window_reads[kh_val(d->rghash, key)]++;
            }

        } // end if
    } // end if 

    return 0;

}

int main(int argc, char *argv[])
{
    int c;
    unsigned int window = 1000;

    pileup_data_t *d = (pileup_data_t*)calloc(1, sizeof(pileup_data_t));
   
    d->min_mapq = 0;
    d->only_paired = 0;
    d->only_properly_paired = 0;
    d->only_use_start_site = 0;
    d->downratio = 1.0;
    d->rghash = kh_init(str);
    d->lengthhash = kh_init(str);
    d->num_read_lengths = 0;
    d->num_other_read_lengths = 0;

    // Added for 0.4 version
    d->dolibgroups  = 0;
    d->doreadlength = 0;

    while ((c = getopt(argc, argv, "q:w:lrpPd:s")) >= 0)
    {
        switch (c) {
            case 'q': d->min_mapq = atoi(optarg); break;
            case 'w': window = atoi(optarg); break;
            case 'l': d->dolibgroups  = 1; break;
            case 'r': d->doreadlength = 1; break;
            case 'd': d->downratio = atof(optarg); break;
            case 'p': d->only_paired = 1; break;
            case 'P': d->only_properly_paired = 1; break;
            case 's': d->only_use_start_site = 1; break;
            default: fprintf(stderr, "Unrecognized option '-%c'.\n", c); return 1;
        }
    }

    // usage
    if ((argc-optind) == 0)
    {
        fprintf(stderr, "\n");
        fprintf(stderr, "Version 0.4\n");
        fprintf(stderr, "Usage: bam-window <bam_file>\n");
        fprintf(stderr, "        -q INT    filtering reads with mapping quality less than INT [%d]\n", d->min_mapq);
        fprintf(stderr, "        -w INT    window size to count reads within [%d]\n",window);
        fprintf(stderr, "        -p        only include paired reads\n");
        fprintf(stderr, "        -P        only include properly paired reads\n");
        fprintf(stderr, "        -s        only count a read as in the window if its leftmost\n");
        fprintf(stderr, "                  mapping position is within the window\n");
        fprintf(stderr, "        -l        output a column for each library in each window\n");
        fprintf(stderr, "        -r        output a column for each read length in each window\n");
        fprintf(stderr, "        -d FLOAT  probability of reporting a read [%f]\n",d->downratio);

        fprintf(stderr, "\nbam-window reports the number of reads mapping within adjacent windows on the genome.\n");
        fprintf(stderr, "It reports the number of reads overlapping the window or optionally, just reports the\n");
        fprintf(stderr, "number whose leftmost mapping coordinate is within the window. Use -d to report only a\n");
        fprintf(stderr, "random sampling of the reads within the window. This is useful for approximating results\n");
        fprintf(stderr, "at lower coverage depths.");
        fprintf(stderr, "\n\n");
        fprintf(stderr, "The -l option creates separate counts for each library\n");
        fprintf(stderr, "The -r option creates separate counts for each read length\n");
        fprintf(stderr, "The -l option can be paired with the -r option to get separate counts for each read length\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "\n");
        return 1;
    }

    d->beg = 0; 
    d->end = window;

    // Get reads length list
    int iter0;
    int iter2 = 0;
    int iter3 = 0;

    int *lenList;
    int RGlinenumber;

    char **LBs;
    char **RGs;
    char *pBamHeader;

    lenList =  (int *)malloc((TOP_LINES_FOR_READ_LENGTH+1)*sizeof(int));

    grabReadLens(argv[optind], lenList, TOP_LINES_FOR_READ_LENGTH);
    quicksort(lenList, TOP_LINES_FOR_READ_LENGTH);

    for (iter2=(TOP_LINES_FOR_READ_LENGTH-1); iter2>=0; iter2--)
    {
        if (lenList[iter2] != iter3)
        {
            d->read_lengths[d->num_read_lengths++] = lenList[iter2];
            //fprintf(stderr, "%d\n", lenList[iter2]);
            iter3 = lenList[iter2];
        }
    }

    // Open bam
    d->in = samopen(argv[optind], "rb", 0);

    if (d->in == 0)
    {
        fprintf(stderr, "Fail to open BAM file %s\n", argv[optind]);
        return 1;
    }

    bam_index_t *idx;

    // load BAM index
    idx = bam_index_load(argv[optind]);
    if (idx == 0)
    {
        fprintf(stderr, "BAM indexing file is not available.\n");
        return 1;
    }

    // Get RGLIBS infor
    bam_header_t  *test_bam_header = d->in->header;
    LBs = (char **)malloc((MAX_RGLIBS+1)*sizeof(char *));

    for (iter0=0; iter0<=MAX_RGLIBS; iter0++)
    {
        LBs[iter0] = (char *)malloc((MAX_TOKEN_LEN)*sizeof(char));
    }

    RGs = (char **)malloc((MAX_RGLIBS+1)*sizeof(char *));

    for (iter0=0; iter0<=MAX_RGLIBS; iter0++)
    {
        RGs[iter0] = (char *)malloc((MAX_TOKEN_LEN)*sizeof(char));
    }

    pBamHeader = test_bam_header->text;
    RGlinenumber = parseRGlines(pBamHeader, LBs, RGs);

    // seed the random number generator with the current microseconds and time
    // http://www.guyrutenberg.com/2007/09/03/seeding-srand/
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    srand(current_time.tv_usec * current_time.tv_sec);
    // print header start
    printf("Chr\tStart");
    // initialize the readgroup stuff
    khash_t(str) *libhash  = kh_init(str);

    char tmpreadlen[1024];
    char rgroup[1024];

    khiter_t key;

    int col;
    int ret;
    int lib_columns = 0;

    // if read lengths are given, convert them to an array of ints
    // if we're splitting by lib.readgroup (have the -r param)
    if (d->dolibgroups)
    {
        int iter0;

        char tmp[1024];
        char lib[1024];
        char tmp2[1024];
        char lib_plus_rl[1024];

        // make sure the rg/lib file exists
        // read the file
        for (iter0=0; iter0<RGlinenumber; iter0++)
        {

            strcpy(lib, LBs[iter0]);
            strcpy(rgroup, RGs[iter0]);

            // if read lengths are given, create a separate column for each 
            // read length within each lib
            if (d->doreadlength)
            {
                int k;

                for (k=0; k < d->num_read_lengths; k++)
                {
                    // append read length to lib name
                    sprintf(tmp,"%i",d->read_lengths[k]);

                    strcpy(lib_plus_rl,lib);                    
                    strcat(lib_plus_rl,".");
                    strcat(lib_plus_rl,tmp);

                    //fprintf(stderr, "%s\n", lib_plus_rl);
                    // see if we've encountered this library before
                    key = kh_get(str, libhash, lib_plus_rl);

                    if (key == kh_end(libhash))
                    {
                        // not found, hash it
                        key = kh_put(str, libhash, strdup(lib_plus_rl), &ret);
                        kh_value(libhash, key) = lib_columns;
                        col = lib_columns;
                        lib_columns++;
                        // print header label
                        printf("\t%s",lib_plus_rl);

                    }
                    else
                    {
                        // is found, retrieve the column number
                        col = kh_val(libhash, key);
                    }
                    // now, hash this readgroup with the value of the column 
                    // of ouput appending the read length to the name
                    strcpy(tmp2, rgroup);
                    strcat(tmp2, ".");
                    strcat(tmp2, tmp);

                    //fprintf(stderr, "%s\n", tmp2);
                    key = kh_put(str, d->rghash, strdup(tmp2), &ret);
                    kh_value(d->rghash, key) = col;

                }
            }
            else
            {
                // just do it once
                // first, see if we've encountered this library before
                key = kh_get(str, libhash, lib);

                if (key == kh_end(libhash))
                {
                    // not found, hash it
                    key = kh_put(str, libhash, strdup(lib), &ret);
                    kh_value(libhash, key) = lib_columns;
                    col = lib_columns;
                    lib_columns++;
                    // print header label
                    printf("\t%s", lib);
                    //fprintf(stderr, "\t%s", lib);
                }
                else
                {
                    // is found, retrieve the column number
                    col = kh_val(libhash, key);
                }
                // now, hash this readgroup with the value of the 
                // column of ouput
                key = kh_put(str, d->rghash, strdup(rgroup), &ret);
                kh_value(d->rghash, key) = col;
            }
        }

        printf("\n");

    }
    else
    {
        if (d->doreadlength)
        {
            // read length as column title
            int k = 0;

            lib_columns = 0;

            for (k=0; k < d->num_read_lengths; k++)
            {
                // append read length to lib name
                sprintf(tmpreadlen, "%i", d->read_lengths[k]);
                printf("\t%s", tmpreadlen);

                col = lib_columns;
                key = kh_put(str, d->lengthhash, strdup(tmpreadlen), &ret);
                kh_value(d->lengthhash, key) = col;

                lib_columns++;
            }

            printf("\n");

        }
        else
        {
            // print header start
            printf("\tCounts\n");
        }

    }

    // uncomment this for quick testing of the header and libhashing 
    // without reading all the way through the bam
    int i = 0;
    for (i = 0; i<d->in->header->n_targets; ++i)
    {
        d->beg = 0;
        d->end = window;

        while (d->beg < d->in->header->target_len[i])
        {

            bam_fetch(d->in->x.bam, idx, i, d->beg, d->end, d, fetch_func);
            printf("%s\t%d", d->in->header->target_name[i], d->beg+1);

            // no readgroup splitting, just one count per window
            if (d->dolibgroups == 0)
            {
                if (d->doreadlength)
                {
                    int k;
                    for (k=0; k<lib_columns; k++)
                    {
                        printf("\t%ld", d->current_window_reads[k]);
                    }
                }
                else
                {
                    printf("\t%ld", d->current_window_reads[0]);
                    // multiple lib
                }
            }
            else
            {
                // for each library (col #)
                int k;

                for (k=0; k<lib_columns; k++)
                {
                    printf("\t%ld", d->current_window_reads[k]);
                }   
            }  

            printf("\n");

            d->beg = d->end;
            d->end = d->beg + window;

            int j = 0;

            for (j=0; j<=lib_columns; j++)
            {
                d->current_window_reads[j] = 0;
            }
        }

    }   

    if (d->num_other_read_lengths > 0)
    {
        fprintf(stderr,"--------------\n");
        fprintf(stderr,"These readlengths were present in bam but not specified for output:\n\n");
        fprintf(stderr,"Length\tNumReads\n");

        for(i=0; i<d->num_other_read_lengths; i++)
        {
            fprintf(stderr,"%i\t%li\n", d->other_read_lengths[i], d->other_read_lengths_count[i]);
        }
    }

    fprintf(stderr,"--------------\n\n");
    samclose(d->in);

    // Free memory
    for (iter0=0; iter0<=MAX_RGLIBS; iter0++)
    {
        free(RGs[iter0]);
    }
    free(RGs);

    for (iter0=0; iter0<=MAX_RGLIBS; iter0++)
    {
        free(LBs[iter0]);
    }
    free(LBs);
    free(lenList);

    return 0;

}

