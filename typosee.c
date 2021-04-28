/***************************/
/* typosee.c               */
/*                         */
/* Enhanced by Ed Gibbs    */
/*                         ***********************************************************************************************************/
/* The Levenshtein distance is a measure of the similarity of two strings. It is defined as the minimum number of                    */
/* insertions, deletions, and substitutions necessary to transform the first string into the second. For example, the                */
/*                                                                                                                                   */
/* Levenshtein distance between “CHALK” and “CHEESE” is 4, as follows:                                                               */
/*                                                                                                                                   */
/*     Substitute E for A                                                                                                            */
/*     Substitute E for L                                                                                                            */
/*     Substitute S for K                                                                                                            */
/*     Insert E                                                                                                                      */
/*                                                                                                                                   */
/* The Levenshtein distance can be calculated using a dynamic programming algorithm due to Wagner and Fischer. The algorithm         */
/* fills a table with the distance between all of the prefixes of the two strings, the final entry being the distance between        */
/* the two entire strings. Below is an implementation in C. I have recorded in the matrix cells information about the edits and      */
/* the previous cell from which each cell was derived to enable tracing back the sequence of edits – the edit script. The function   */
/* levenshtein_distance() takes two strings, and the address of a pointer to which to assign an array containing the edit script.    */
/* It returns the Levenshtein distance.                                                                                              */
/*                                                                                                                                   */
/* The unmodifed algorythm was orignally posted at http://www.martinbroadhurst.com/levenshtein-distance-in-c.html                    */
/*                                                                                                                                   */
/* v1 - Take a list of FQNDs from the WHOIS Subdomain database and match up each FQDN element using LDA                              */
/*************************************************************************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

typedef enum {
    INSERTION,
    DELETION,
    SUBSTITUTION,
    NONE
} edit_type;
 
struct edit {
    unsigned int score;
    edit_type type;
    char arg1;
    char arg2;
    unsigned int pos;
    struct edit *prev;
};

typedef struct edit edit;

void print(const edit *e)
{
    if (e->type == INSERTION) {
        printf("\tInsert %c", e->arg2);
    }
    else if (e->type == DELETION) {
        printf("\tDelete %c", e->arg1);
    }
    else {
        printf("\tSubstitute %c for %c", e->arg2, e->arg1);
    }
    printf(" at %u\n", e->pos);
}
 

static int min3(int a, int b, int c)
{
    if (a < b && a < c) {
        return a;
    }
    if (b < a && b < c) {
        return b;
    }
    return c;
}
 
static unsigned int levenshtein_matrix_calculate(edit **mat, const char *str1, size_t len1,
        const char *str2, size_t len2)
{
    unsigned int i, j;
    for (j = 1; j <= len2; j++) {
        for (i = 1; i <= len1; i++) {
            unsigned int substitution_cost;
            unsigned int del = 0, ins = 0, subst = 0;
            unsigned int best;
            if (str1[i - 1] == str2[j - 1]) {
                substitution_cost = 0;
            }
            else {
                substitution_cost = 1;
            }
            del = mat[i - 1][j].score + 1; /* deletion */
            ins = mat[i][j - 1].score + 1; /* insertion */
            subst = mat[i - 1][j - 1].score + substitution_cost; /* substitution */
            best = min3(del, ins, subst);
            mat[i][j].score = best;                  
            mat[i][j].arg1 = str1[i - 1];
            mat[i][j].arg2 = str2[j - 1];
            mat[i][j].pos = i - 1;
            if (best == del) {
                mat[i][j].type = DELETION;
                mat[i][j].prev = &mat[i - 1][j];
            }
            else if (best == ins) {
                mat[i][j].type = INSERTION;
                mat[i][j].prev = &mat[i][j - 1];
            }
            else {
                if (substitution_cost > 0) {
                    mat[i][j].type = SUBSTITUTION;
                }
                else {
                    mat[i][j].type = NONE;
                }
                mat[i][j].prev = &mat[i - 1][j - 1];
            }
        }
    }
    return mat[len1][len2].score;
}
 
static edit **levenshtein_matrix_create(const char *str1, size_t len1, const char *str2,
        size_t len2)
{
    unsigned int i, j;
    edit **mat = malloc((len1 + 1) * sizeof(edit *));
    if (mat == NULL) {
        return NULL;
    }
    for (i = 0; i <= len1; i++) {
        mat[i] = malloc((len2 + 1) * sizeof(edit));
        if (mat[i] == NULL) {
            for (j = 0; j < i; j++) {
                free(mat[j]);
            }
            free(mat);
            return NULL;
        }
    }
    for (i = 0; i <= len1; i++) {
        mat[i][0].score = i;
        mat[i][0].prev = NULL;
        mat[i][0].arg1 = 0;
        mat[i][0].arg2 = 0;
    }
 
    for (j = 0; j <= len2; j++) {
        mat[0][j].score = j;
        mat[0][j].prev = NULL;
        mat[0][j].arg1 = 0;
        mat[0][j].arg2 = 0;
    }
    return mat; 
}
 
unsigned int levenshtein_distance(const char *str1, const char *str2, edit **script)
{
    const size_t len1 = strlen(str1), len2 = strlen(str2);
    unsigned int i, distance;
    edit **mat, *head;
 
    /* If either string is empty, the distance is the other string's length */
    if (len1 == 0) {
        return len2;
    }
    if (len2 == 0) {
        return len1;
    }
    /* Initialise the matrix */
    mat = levenshtein_matrix_create(str1, len1, str2, len2);
    if (!mat) {
        *script = NULL;
        return 0;
    }
    /* Main algorithm */
    distance = levenshtein_matrix_calculate(mat, str1, len1, str2, len2);
    /* Read back the edit script */
    *script = malloc(distance * sizeof(edit));
    if (*script) {
        i = distance - 1;
        for (head = &mat[len1][len2];
                head->prev != NULL;
                head = head->prev) {
            if (head->type != NONE) {
                memcpy(*script + i, head, sizeof(edit));
                i--;
            }
        }
    }
    else {
        distance = 0;
    }
    /* Clean up */
    for (i = 0; i <= len1; i++) {
        free(mat[i]);
    }
    free(mat);
 
    return distance;
}

int count_periods(char *str)
{
	int i, p = 0;
	
	for(i=0;i<strlen(str);i++)
		if(str[i] == '.')
			p++;

	return p;
}

char *strrev(char *str)
{
      char *p1, *p2;

      if (! str || ! *str)
            return str;
      for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
      {
            *p1 ^= *p2;
            *p2 ^= *p1;
            *p1 ^= *p2;
      }
      return str;
}

void strip(char *str)
{
	int i;

	if(str[strlen(str)-1] == '\n' || str[strlen(str)-1] == '\r')
		str[strlen(str)-1] = 0x0;
		
	if(str[strlen(str)-1] == 0x0d || str[strlen(str)-1] == 0x0a)
		str[strlen(str)-1] = 0x0;	
		
	if(str[strlen(str)-1] == ',')
		str[strlen(str)-1] = 0x0;
		
	for ( ; *str; ++str) *str = tolower(*str);
}

void strip_subline(char *str)
{
	int i;
	char buf[2048];
	
	strcpy(buf, str);
	
	strip(buf);			/* 4,4,4,abc.com */
	
	strrev(buf);			/* moc.cba,4,4,4 */
	
	for(i=strlen(buf)-1;i>=0;i--)
		if(buf[i] == ',')
			buf[i] = 0x0;
			
	strrev(buf);
	
	strcpy(str, buf);
}

int main(int argc, char **argv)
{
    FILE *fp, *kfp;
    edit *script;
    unsigned int distance, threshold;
    unsigned int i, x, lineNum = 0, token_cnt, num_p, debug=0;
    char fileName[1024], keyWord[2048], lineBuf[2048], copyOfLine[2048], verbose=0, fqdn_word[2048], keyLineBuf[2048];
    char *token;
    const char period[2] = ".\0";
    
    if(argc < 4)
    	{
	printf("\ntyposee by Ed@whoisxmlapi.com.\n\n\t");
	printf("args: subdomain_filename keyword_filename Threshhold# [v:q]  where 'q'=quiet, 'v'=verbose\n\n");
	return 0;
	}
	
    strcpy(fileName, argv[1]);
    strcpy(keyWord, argv[2]);
    
    threshold = atoi(argv[3]);
    
    if(threshold < 1 || threshold > 100)
    	{
    	printf("[ERR] Invalid threshold number. Must be between 0 and 100.\n");
    	return 0;
    	}
    
    if(argv[4][0] == 'v')
    	verbose = 1;
    if(argv[4][0] == 'd')
    	debug = 1;
    	
    if( (fp = fopen(fileName, "rt")) == NULL)
    	{
    	printf("[ERR]: Unable to open %s\n", fileName);
    	return 0;
    	}
    	
    if( (kfp = fopen(keyWord, "rt")) == NULL)
    	{
    	printf("[ERR]: Unable to open %s\n", keyWord);
    	return 0;
    	}
    	
    printf("distance,keyword,fqdn-element,full-fqdn\n");
    	
    while( fgets(keyLineBuf, 2048, kfp)!= NULL )
    {
    	strip(keyLineBuf);
    	
    	strcpy(keyWord, keyLineBuf);
    	
    	if(debug)
    		printf("[DEBUG] ReadLine [%s]\n", keyLineBuf);
    		
    	token_cnt = 0;
    
       while( fgets(lineBuf, 2048, fp) != NULL)
    	{
 	   if(!lineNum++)
 	   	{
 	   	if(debug)
 	   		printf("[DEBUG]: lineNum = %d\n", lineNum);		
 	   	continue;
 	   	}
 
 	   strip_subline(lineBuf);
 	
 	   token_cnt = num_p = 0;
 	
 	   num_p = count_periods(lineBuf);
 	
 	   strcpy(copyOfLine, lineBuf);
 	   
 	   if(debug)
 	   	printf("%s, %d for [%s]\n", keyWord, lineNum, lineBuf);
 
 	   if( (token = strtok(lineBuf, period)) != NULL)
 		{
		memset(&script, sizeof(script), 0x0);
 
		distance = levenshtein_distance(keyWord, token, &script);
	
		if(distance <= threshold)
			{
			printf("%d,%s,%s,%s\n", distance, keyWord, token, copyOfLine);
			
			if(debug)
				printf("K: [%s], H: [%s] in [%s]\n\tDistance is %d:\n", keyWord, token, copyOfLine, distance);

			if(verbose)
		   		for (i = 0; i < distance; i++) 
		     			print(&script[i]);
		     	}
		     		
		token_cnt++;
		
		if(token_cnt == num_p)			/* don't process domain */
			continue;
 	
		while(token != NULL)
			{	
			if(token_cnt++ == num_p)
				break;
		
			if( (token = strtok(NULL, period)) == NULL)
				break;
			
			memset(&script, sizeof(script), 0x0);
 
			distance = levenshtein_distance(keyWord, token, &script);
			
			if(distance <= threshold)
				{
				printf("%d,%s,%s,%s\n", distance, keyWord, token, copyOfLine);
				
				if(debug)
					printf("K: [%s], H: [%s] in [%s]\n\tDistance is %d:\n", keyWord, token, copyOfLine, distance);

				if(verbose)
		   			for (i = 0; i < distance; i++) 
		     				print(&script[i]);
		     		}
		     	}
		}
    	}
    	rewind(fp);
    }

    free(script);
    
    fclose(fp);
    fclose(kfp);
 
    printf("Total lines processed: %d\n", --lineNum);
    
    return 0;
}
