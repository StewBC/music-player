/*--------------------------------------------------------------------------*\
 * vgmclean - Strip a VGM file of everything but register, data pairs and
 * the delay controls blocks for the Yamaha YM2151.
 * 
 * Other than 0x54, 0x61, 0x62 and 0x63, all other types are omitted.
 * 
 * Delay for type 0x61 is stripped down to a single byte using:
 * nnnn / (44100 / hertz) where nnnn is the 0x61 type and hertz is specified
 * on the command laine (defaults to 50).  0x62 and 0x63 both output register
 * 0x16.  If the 0x61 delay is 0, 0x16 is output. 0x17 is always appended at 
 * the end.
 * 
 * Output - register, data
 * Special registers:
 * 0x15 = 0x61, i.e. 0x15, nn = 50hz default
 * 0x16 = 0x62 & 0x63 (no data)
 * 0x17 = 0x66 (no data)
 * 
 * Stefan Wessels, 2019
 * This is free and unencumbered software released into the public domain.
\*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define REG_DELAY   0x02
#define REG_YIELD   0x03
#define REG_END     0x04
#define REG_PAD     0x05

/*--------------------------------------------------------------------------*/
typedef struct _globals
{
    char *inFile;
    char *outFile;
    char *optFile;
    int ascii;
    int cStyle;
    int fit;
    int hertz;
    int split;
    int verbose;
    int x16prep;
    int yield;
    unsigned int xAddress;
    char *attribs;
    unsigned int written;
    unsigned int partNumber;
    unsigned int lastBlock;
    unsigned int fitSize;
    FILE *fpi;
    FILE *fpo;
} Globals;

/*--------------------------------------------------------------------------*/
Globals globs;

/*--------------------------------------------------------------------------*/
FILE *openFile(int input, char *fileAttribs)
{
    FILE *fp;
    char *openName;

    if(!input)
    {
        if(!globs.ascii && globs.split)
        {
            int len;

            if(globs.outFile && globs.optFile)
            {
                fclose(globs.fpo);
                free(globs.outFile);
            }
            
            if(!globs.optFile)
            {
                globs.optFile = globs.outFile;
            }
            
            len = strlen(globs.optFile) + 16;
            globs.outFile = (char *)malloc(len);
            if(!globs.outFile)
            {
                printf("Malloc failed for out file name!\n");
                exit(1);
            }
            sprintf(globs.outFile, "%s%02X", globs.optFile, ++globs.partNumber);
        }
        openName = globs.outFile;
    }
    else
    {
        openName = globs.inFile;
    }
    

    fp = fopen(openName, fileAttribs);
    if(!fp)
    {
        printf("Error opening %s for %s\n", openName, fileAttribs);
        exit(1);
    }

    if(!input && globs.x16prep)
    {
        fprintf(fp, "%c", (unsigned char)(globs.xAddress & 0xff));
        fprintf(fp, "%c", (unsigned char)((globs.xAddress & 0xff00) >> 8));
    }

    return fp;
}

/*--------------------------------------------------------------------------*/
void output(int atomic, unsigned char byte)
{
    if(globs.ascii)
    {
        if(globs.cStyle)
        {
            if(globs.written && !(globs.written % 16))
                fprintf(globs.fpo, "\n");
            fprintf(globs.fpo, "0x%02X, ", byte);
        }
        else
        {
            if(!(globs.written % 16))
            {
                if(globs.written)
                    fprintf(globs.fpo, "\n");
                fprintf(globs.fpo, ".byte $%02X", byte);
            }
            else
                fprintf(globs.fpo, ", $%02X", byte);
        }
        
    }
    else 
    {
        if(globs.split)
        {
            if(globs.written != 0 && 0 == (globs.written % globs.split))
            {
                globs.fpo = openFile(0, globs.attribs);
                if(globs.verbose > 1)
                    printf("Output now to file %s\n", globs.outFile);
            }
        }
        fprintf(globs.fpo, "%c", byte);

    }
    globs.written++;

    if(!atomic && globs.fit && (globs.written - globs.lastBlock) >= (globs.fitSize - 2))
    {
        unsigned int bytesNeeded = (globs.fitSize - (globs.written - globs.lastBlock));

        if(globs.verbose > 2)
        {
            int offset = globs.x16prep ? -2 : 0;
            printf("Adding %u REG_PAD at %ld to fill a %u block of bytes\n", bytesNeeded, ftell(globs.fpo) + offset, globs.fitSize);
        }

        while(bytesNeeded--)
        {
            output(1, REG_PAD);
        }

        globs.lastBlock = globs.written;
    }
}

/*--------------------------------------------------------------------------*/
int parse(unsigned char *iBuffer, unsigned int iSize)
{
    unsigned int index;
    int Gd3;
    int lastYield;

    if(globs.verbose > 2)
    {
        printf("parsing buffer of %d bytes\n", iSize);
        printf("YM2151 clock %d\n", *((unsigned int *)&(iBuffer[0x30])));
        printf("Data at offset 0x%02X\n", iBuffer[0x34]);
    }

    index = 0x34 + iBuffer[0x34];
    Gd3 = 0x14 + *(int*)&(iBuffer[0x14]);
    lastYield = index;

    while(index < iSize)
    {
        unsigned char command = iBuffer[index++];

        if(globs.yield && globs.written - globs.yield >= lastYield)
        {
            lastYield = globs.written;
            if(globs.verbose > 2)
                printf("Yielding at %d\n", index);
            output(0, REG_YIELD);
        }

        if(command <= 0x50)
        {
            if(globs.verbose > 2)
                printf("CMD: %02x skipping 1\n", command);
            index++;
        }
        else if(0x60 >= command)
        {
            if(0x54 == command)
            {
                if(iBuffer[index] == REG_DELAY || iBuffer[index] == REG_YIELD || iBuffer[index] == REG_END)
                {
                    printf("Warning: %02X Skipping 2.  Repurposed reg appears in this vgm\n", iBuffer[index]);
                    index += 2;
                }
                else 
                {
                    output(1, iBuffer[index++]);
                    output(0, iBuffer[index++]);
                }
            }
            else 
            {
                if(globs.verbose > 2)
                    printf("CMD: %02x skipping 2\n", command);
                index += 2;
            }
        }
        else if(0x61 == command)
        {
            unsigned short delay = *(unsigned short*)&(iBuffer[index]);
            delay /= (44100 / globs.hertz);
            lastYield = globs.written;

            if(delay > 1)
            {
                while(delay > 0xff)
                {
                    output(1, REG_DELAY);
                    output(0, 0xff);
                    delay -= 0xff;
                }
                output(1, REG_DELAY);
                output(0, delay);
            }
            else
            {
                output(0, REG_YIELD);
            }
            index += 2;
        }
        else if(0x62 == command || 0x63 == command)
        {
            lastYield = globs.written;
            output(0, REG_YIELD);
        }
        else if(0x66 == command)
        {
            output(0, REG_END);
            if(index != Gd3)
                printf("WARNING: %02X isn't followed by the Gd3 tag at 0x%04X from end of file\n", command, iSize - index);
            break;
        }
        else
        {
            if(0x64 == command)
            {
                if(globs.verbose > 2)
                    printf("Command 0x64 encountered.  Not great.  Skipping 3\n");
                index += 3;
            }
            else if(0x67 == command)
            {
                unsigned int skip = *(unsigned int*)&(iBuffer[index+2]) + 2 + 4;
                if(globs.verbose > 2)
                    printf("CMD %02X skipping %d\n", command, skip);
                index += skip;
            }
            else if(0x68 == command)
            {
                printf("Aborting: 0x%02X encountered after writing %d bytes\n", command, globs.written);
                globs.written = -1;
                break;
            }
            else if(command < 0x90)
            {
                if(globs.verbose > 2)
                    printf("CMD %02X skipping 0 ** cmd 7n or 8n otherwise an issue\n", command);
            }
            else if(0x90 == command || 0x91 == command)
            {
                if(globs.verbose > 2)
                    printf("CMD %02X skipping 4\n", command);
                index += 4;
            }
            else if(0x92 == command)
            {
                if(globs.verbose > 2)
                    printf("CMD %02X skipping 5\n", command);
                index += 5;
            }
            else if(0x93 == command)
            {
                if(globs.verbose > 2)
                    printf("CMD %02X skipping 10 *** This may be an issue\n", command);
                index += 10;
            }
            else if(0x94 == command)
            {
                if(globs.verbose > 2)
                    printf("CMD %02X skipping 1\n", command);
                index += 1;
            }
            else if(0x95 == command)
            {
                if(globs.verbose > 2)
                    printf("CMD %02X skipping 4\n", command);
                index += 4;
            }
            else if (0xc0 > command)
            {
                if(globs.verbose > 2)
                    printf("CMD %02X skipping 2\n", command);
                index += 2;
            }
            else if(0xe0 > command)
            {
                if(globs.verbose > 2)
                    printf("CMD %02X skipping 3\n", command);
                index += 3;
            }
            else
            {
                if(globs.verbose > 2)
                    printf("CMD %02X skipping 4\n", command);
                index += 4;
            }
        }
    }

    return globs.written;
}

/*--------------------------------------------------------------------------*/
int process(void)
{
    unsigned int iSize, rSize;
    unsigned char *iBuffer, *oBuffer;

    if(0L != fseek(globs.fpi, 0L, SEEK_END))
        return -1;

    if(globs.verbose)
        printf("File end seek okay\n");

    if((iSize = ftell(globs.fpi)) == -1)
        return -1;

    if(globs.verbose)
        printf("File size %d\n", iSize);
    
    if(!(iBuffer = (unsigned char *)malloc(iSize)))
        return -1;

    if(globs.verbose)
        printf("Read buffer allocated okay\n");
    
    rewind(globs.fpi);

    rSize = fread (iBuffer, 1, iSize, globs.fpi);
    if ( rSize != iSize)
    {
        printf("File bytes read %d\n", rSize);
        return -1;
    }

    if(globs.verbose)
        printf("File read into buffer okay\n");
    
    rSize = parse(iBuffer, iSize);

    if(rSize >= 0)
    {
        output(0, 0x66);
    }
    
    return rSize + 1;
}


/*--------------------------------------------------------------------------*/
void Usage(int argi, char **argv)
{
    if(argi)
        printf("Argument not understood or missing opt: \"%s\"\n", argv[argi]);

    printf("\nVGMCLEAN V1.00 by Stefan Wessels, Nov 2019.\n\n");
    printf("Usage: vgmclean [-<opts> [<opt>]] <infile> <outfile>\n");
    printf("Where opts are:\n");
    printf("\ta - outfile is a hex, comma seperated ascii file (see \"c\")\n");
    printf("\tc - \"C\" style (0x prepended) - default is asm inc style ($ prepended)\n");
    printf("\tf - opt is a block size (-f- defaults to 8192) to fit a stream in.\n\t\tBlocks padded with $%02X\n", REG_PAD);
    printf("\th - Hertz, usually opt is 50 (default) or 60\n");
    printf("\ts - Split output into binary files of opt bytes (-s- uses -f size)\n");
    printf("\tv - Verbose output (More v's more verbose up to -vvv)\n");
    printf("\tx - opt is address that will be prepended to binary files\n\t\t(CBM load style. -x- defaults to $A000)\n");
    printf("\ty - opt is max run length before a yield is forced\n");
    printf("\n");

    exit(1);
}

/*--------------------------------------------------------------------------*/
void cmdLine(int argc, char **argv)
{
    int argi = 1;

    while(argi < argc)
    {
        if(argv[argi][0] == '-')
        {
            switch(argv[argi][1])
            {
                case 'a':
                    globs.ascii = 1;
                    globs.attribs = "w+";
                    globs.x16prep = 0;
                    globs.split = 0;
                    break;

                case 'c':
                    globs.cStyle = 1;
                    break;

                case 'f':
                    globs.fit = 1;
                    if(argv[argi][2] != '-')
                    {
                        if(argi + 1 < argc)
                        {
                            argi++;
                            globs.fitSize = atol(argv[argi]);
                        }
                        else 
                            Usage(argi, argv);
                    }
                    break;

                case 'h':
                    if(argi + 1 < argc)
                    {
                        argi++;
                        globs.hertz = atol(argv[argi]);
                    }
                    else 
                        Usage(argi, argv);
                    break;

                case 's':
                    globs.ascii = 0;
                    if(argv[argi][2] != '-')
                    {
                        if(argi + 1 < argc)
                        {
                            argi++;
                            globs.split = atol(argv[argi]);
                        }
                        else 
                            Usage(argi, argv);
                    }
                    else
                    {
                        globs.split = globs.fitSize;
                    }
                    break;
                
                case 'v':
                    globs.verbose = 1;
                    while('v' == argv[argi][globs.verbose + 1])
                        globs.verbose++;
                    break;

                case 'x':
                    globs.ascii = 0;
                    globs.x16prep = 1;
                    if(argv[argi][2] != '-')
                    {
                        if(argi + 1 < argc)
                        {
                            argi++;
                            globs.xAddress = atol(argv[argi]) & 0xffff;
                        }
                        else 
                            Usage(argi, argv);
                    }
                    break;

                case 'y':
                    if(argi + 1 < argc)
                    {
                        argi++;
                        globs.yield = atol(argv[argi]);
                    }
                    else 
                        Usage(argi, argv);
                    break;
            }
        }
        else 
        {
            if(!globs.inFile)
                globs.inFile = argv[argi];
            else if(!globs.outFile)
                globs.outFile = argv[argi];
            else 
                Usage(argi, argv);
        }
        argi++;
    }

    if(!globs.inFile || !globs.outFile)
        Usage(0, 0);
}

/*--------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    int bytesProcessed;

    globs.inFile = globs.outFile = globs.optFile = 0;
    globs.hertz = 50;
    globs.ascii = globs.verbose = globs.x16prep = globs.lastBlock = globs.fit = globs.split = globs.yield = 0;
    globs.fitSize = 0x2000;
    globs.xAddress = 0xA000;
    globs.attribs = "w+b";
    
    cmdLine(argc, argv);
    if(globs.verbose)
    {
        printf("Processing file \"%s\"\nOutput to \"%s\"\nHertz: %d\nAscii %s\nYield %d\n", globs.inFile, globs.outFile, globs.hertz, globs.ascii ? "Yes" : "No", globs.yield);
        if(globs.ascii)
        {
            printf("Hex Tpye: %s\n", globs.cStyle ? "C (0x)" : "ASM ($)");
        }
        else 
        {
            if(globs.fit)
                printf("Fit Size: %d\n", globs.fitSize);
            if(globs.split)
                printf("Split Size: %d\n", globs.split);
            if(globs.x16prep)
                printf("x16 (CBM) Address: %u (0x%02X)\n", globs.xAddress, globs.xAddress);
        }
    }

    globs.fpi = openFile(1, "rb");
    if(globs.verbose)
        printf("infile opened okay\n");

    globs.fpo = openFile(0, globs.attribs);
    if(globs.verbose)
        printf("outfile opened okay\n");

    bytesProcessed = process();
    if(globs.verbose)
        printf("Bytes processed %d\n", bytesProcessed);

    fclose(globs.fpi);
    fclose(globs.fpo);

    if(bytesProcessed < 0)
    {
        printf("\n** Error **\nFailed to process\t\"%s\"\ninto\n\t\"%s\"\n\n", argv[1], argv[2]);
        exit(1);
    }

    return 0;
}
