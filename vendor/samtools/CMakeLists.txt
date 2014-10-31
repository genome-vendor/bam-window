cmake_minimum_required(VERSION 2.8)

add_definitions(-D_USE_KNETFILE)
set(SOURCES
    bgzf.c
    kstring.c
    bam_aux.c
    bam.c
    bam_import.c
    sam.c
    bam_index.c
    bam_pileup.c
    bam_lpileup.c
    bam_md.c
    razf.c
    faidx.c
    bedidx.c
    knetfile.c
    bam_sort.c
    sam_header.c
    bam_reheader.c
    kprobaln.c
    bam_cat.c
)

add_library(bam ${SOURCES})
