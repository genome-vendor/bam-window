all:
	gcc -g -Wall -O2 -I/gsc/pkg/bio/samtools/samtools-0.1.17/ bamwindow.c -o bam-window -lm -lz -L/gsc/pkg/bio/samtools/samtools-0.1.17/ -lbam
