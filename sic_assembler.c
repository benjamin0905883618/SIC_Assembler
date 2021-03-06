#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXOP 27

char name[16];
FILE *f;
FILE *fobj;
FILE *intermediate_file;
FILE *listing_code;

char line[80];
char label[7];
char op[7];
char operand[10];
char new_operand[7];
int indexed = 0;

char prog_name[7];
int  start_addr = 0;
int  prog_len = 0;
char obj_line[70];
char obj_code[7];
int  last_locctr = 0;
int  locctr = 0;
int  textpos = 0;
int instruction_len = 0;
int start_address;

const char a_start[] = "START";
const char a_end[] = "END";
const char a_byte[] = "BYTE";
const char a_word[] = "WORD";
const char a_resb[] = "RESB";
const char a_resw[] = "RESW";

const char optab[26][2][6] = {{"ADD", "18"}, {"AND", "40"}, {"COMP", "28"}, {"DIV", "24"}, {"J", "3C"}, {"JEQ", "30"}, {"JGT", "34"}, {"JLT", "38"}, {"JSUB", "48"}, {"LDA", "00"}, {"LDCH", "50"}, {"LDL", "08"}, {"LDX", "04"}, {"MUL", "20"}, {"OR", "44"}, {"RD", "D8"}, {"RSUB", "4C"}, {"STA", "0C"}, {"STCH", "54"}, {"STL", "14"}, {"STSW", "E8"}, {"STX", "10"}, {"SUB", "1C"}, {"TD", "E0"}, {"TIX", "2C"}, {"WD", "DC"}};
const char hex_c[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

//symbol table的一個節點
typedef struct sym_node {
    char l[7];
    int v;
    struct sym_node * next,* prev;

} symNODE;

symNODE *tail = NULL;
symNODE *symtab = NULL;
int flag = 1;

//插入一個節點到symbol table
symNODE* insert(char *s, int r) {
    if(flag == 1){
        symtab = (struct sym_node *)malloc(sizeof(struct sym_node));
        tail = (struct sym_node *)malloc(sizeof(struct sym_node));
        symtab->prev = NULL;
        symtab->next = tail;
        tail->prev = symtab;
        tail->next = NULL;
        sprintf(tail->l,"XXXXXXX");
        flag = 0;
    }
    symNODE *temp = NULL;
    temp = (struct sym_node *)malloc(sizeof(struct sym_node));
    strcpy(temp->l,s);
    temp->v = r;
    temp->next = symtab->next;
    temp->prev = symtab;
    temp->next->prev = temp;
    temp->prev->next = temp;
    return temp;
}

//搜尋symbol table的結點
symNODE* search(symNODE *t, char *s) {
    if( t ) {
        if (strcmp(s, t->l) == 0) {
            return t;
        } else {
            return search(t->next, s);
        }
    } else {
        return NULL;
    }
}

//搜尋symbol table的節點
symNODE* new_search( char *s) {
    int i;
    strcpy( new_operand, s );
    for (i=strlen(new_operand); i<6; i++) new_operand[i] = ' ';
    new_operand[6] = '\0';
    return search( symtab, new_operand );
}

//找opcode
char* lookup (char *s) {
    int i = 0;
    int nf = 1;
    while ((i < MAXOP) && (nf)) {
        if (strcmp(s, optab[i][0]) == 0) nf=0;
        else i++;
    }
    if (i >= MAXOP) return NULL;
    else return (char*)optab[i][1];
}

//回傳operand長度
int operand_len () {
    int i, l;
    l = strlen(operand);
    // for (i=0; i<l; i++) printf("operand[%d] = [%c].\n", i, operand[i]);
    if (operand[0] == 'C') l -= 3;
    else if (operand[0] == 'X') l = (l-3) / 2;
    // printf("find_length([%s] = %d.\n", operand, l);
    return l;
}

//讀一行指令
int readline(){
    int i, j, l, x;
    indexed = 0;
    fgets(line, 80, f);
    l = strlen(line);
    if ((l>0) && (line[0]!='.')) {
       for (i = 0; i < 6; i++) {
           label[i] = line[i];
       }
       label[i] = '\0';
       while(line[i]==' ') i++;
       j = 0;
       while ((line[i]!=' ') && (line[i]!='\0') && (line[i]!='\n') && (i < l)) {
           op[j] = line[i];
           i++; j++;
       }
       op[j] = '\0';
       while(line[i]==' ') i++;
       j = 0;
       while ((line[i]!=' ') && (line[i]!='\0') && (line[i]!='\n') && (i < l)) {
           operand[j] = line[i];
           i++; j++;
       }
       operand[j] = '\0';
       indexed = 0; x = strlen(operand);
       if((x>2) && (operand[x-2]==',') && (operand[x-1]=='X')) {
           operand[x-2] = '\0';
           indexed = 1;
       }
       //printf("[%6X] Read a line: label=[%s], op=[%s], operand=[%s].\n",
           //locctr, label, op, operand);
       return 1;
    } else {
       return 0;
    }
}

void write_line(){
    line[0] = '\0';
    char str[6];
    sprintf(str,"%X",locctr);
    strcat(line,str);
    strcat(line," ");
    strcat(line,label);
    strcat(line," ");
    strcat(line,op);
    strcat(line," ");
    strcat(line,operand);
    //printf("%s\n",operand);
    strcat(line,"\n");
    line[strlen(line)] = '\0';
    //printf("%s",line);
    fprintf(intermediate_file,"%s",line);
}

void pass1 () {
//
// Write your own pass1()
//
    readline();
    intermediate_file = fopen("loc.txt","w");
    if(strcmp(op,a_start) == 0){
        sscanf(operand,"%x",&start_addr);
        sscanf(operand,"%x",&locctr);
        write_line(intermediate_file);
    }
    else
        locctr = 0;
    while(1){
        if(readline() == 1){
            if(strcmp(label,"      ") != 0){
                if( new_search(label)){
                    printf("error : duplicate symbol!\n");
                    break;
                }
                else
                    insert(label,locctr);
            }

            if(lookup(op) != NULL){
                locctr += 3;

            }
            else if(strcmp(op,a_end) == 0)
                break;
            else if(strcmp(op,a_word) == 0)
                locctr += 3;
            else if(strcmp(op,a_resw) == 0)
                locctr += 3 * atoi(operand);
            else if(strcmp(op,a_resb) == 0)
                locctr += atoi(operand);
            else if(strcmp(op,a_byte) == 0)
                locctr += operand_len(operand);
            else{
                printf("error : invalid operation code !");
                break;
            }
            write_line(intermediate_file);

        }

    }
    write_line(intermediate_file);
    fclose(intermediate_file);
    prog_len = locctr - start_addr;

}

//輸出synbol table
void print_symtab (symNODE * t) {
    if (t && strcmp(t->l,"XXXXXXX") != 0) {
        print_symtab( t->next );
        printf("[%s] = [%5X]\n", t->l, t->v);
    }
}

//初始化object code line
void init_obj_line () {
    obj_line[0] = '\0';
}

//寫header record
void wr_header () {
//
// Write your own wr_header()
//
    init_obj_line();
    obj_line[0] = '\0';
    strcat(obj_line,"H");
    strcat(obj_line,label);
    char str[6];
    sprintf(str,"%.6X",start_addr);
    strcat(obj_line,str);
    sprintf(str,"%.6X",prog_len);
    strcat(obj_line,str);
    fprintf(fobj,"%s\n",obj_line);
}

//初始object code
void init_obj_code () {
    obj_code[0] = '\0';
}


void conv_byte ( int l, char *p, char *q ) {
    int i, j, k, max, c, m, n;
    if (p[0] == 'X') {
        max = 2 * l;
        for (i=2, j=0, k=0; k < max; i++, j++, k++) q[j] = p[i];
        q[j] = '\0';
    }
    else if (p[0] == 'C') {
        max = l;
        for (i=2, j=0, k=0; k < max; i++, k++) {
            c = (int)p[i];
            m = c / 16;
            q[j++] = hex_c[m];
            n = c % 16;
            q[j++] = hex_c[n];
        }
        q[j] = '\0';
    }
    else {
        printf("Error: wrong operand of BYTE!\n");
    }
}

//初始text record
int flag_start = 1;
void init_text () {
    init_obj_line();
    if(flag_start == 1){
        sprintf( obj_line, "T%.6X  ", locctr );
        flag_start = 0;
    }
    else
        sprintf( obj_line, "T%.6X  ", last_locctr );
    for (int i=1; i<7; i++)
        if (obj_line[i] == ' ') obj_line[i] = '0';
    textpos = 9;
    sscanf("00","%x",&instruction_len);
}

//寫入text record
void wr_text () {
//
// Write your own wr_text()
    char str1[2];
    char str2[2];
    int temp,i;
    temp = instruction_len % 16;
    instruction_len /= 16;
    itoa(temp,str1,16);
    itoa(instruction_len,str2,16);
    str1[0] = toupper(str1[0]);
    str2[0] = toupper(str2[0]);
    obj_line[7] = str2[0];
    obj_line[8] = str1[0];
    fprintf(fobj,"%s\n",obj_line);
}

//
void add_text ( int n, char *p ) {
    int const max = 69;
    int k = n * 2;
    int i;
    if ((textpos+k) > max) {
        wr_text();
        init_text();
    }
    for (i=0; i<k; i++) obj_line[textpos++] = p[i];
    if(p[0] != '\0'){
        instruction_len += n;
    }
}
//寫入end record
void wr_end () {
//
// Write your own wr_end()
//
    fprintf(fobj,"E%.6X",start_addr);
}

int flag_obj = 1;
void write_line_obj(){

    line[0] = '\0';
    char str[6];
    sprintf(str,"%X",locctr);
    //printf("%s",str);
    strcat(line,str);
    strcat(line," ");
    strcat(line,label);
    strcat(line," ");
    strcat(line,op);
    strcat(line," ");
    strcat(line,operand);
    strcat(line," ");
    strcat(line,obj_code);
    strcat(line,"\n");
    int len = strlen(line);
    line[len+1] = '\0';
    fputs(line,listing_code);
}


void pass2 () {
//
// Write your own pass2()
//
    locctr = 0;
    readline();
    listing_code = fopen("output.txt","w");
    if(strcmp(op,a_start) == 0){
        sscanf(operand,"%x",&locctr);
        sscanf(operand,"%x",&last_locctr);
        write_line_obj();
        wr_header();
    }
    init_text();
    while(1){
        if(readline() == 1){
            init_obj_code();
            if(lookup(op) != NULL){
                locctr += 3;
                strcat(obj_code,lookup(op));
                if(new_search(operand) != NULL){
                    char stroperand[4];
                    int temp;
                    temp = new_search(operand)->v;
                    if(indexed){
                        temp += 32768;
                    }
                    sprintf(stroperand,"%4X",temp);
                    strcat(obj_code,stroperand);
                    obj_code[strlen(obj_code)] = '\0';
                    write_line_obj();

                    add_text(3,obj_code);
                }
                else if(strcmp(operand,"") == 0){
                    strcat(obj_code,"0000");
                    obj_code[strlen(obj_code)] = '\0';
                    write_line_obj();
                    add_text(3,obj_code);
                }
                else if(new_search(operand) == NULL)
                {
                    printf("error : undefined symbol!");
                    break;
                }
            }
            else if(strcmp(op,a_end) == 0)
                break;
            else if(strcmp(op,a_resw) == 0){
                locctr += 3 * atoi(operand);
                init_obj_code();
                obj_code[strlen(obj_code)] = '\0';
                write_line_obj();
                add_text(3,obj_code);
            }
            else if(strcmp(op,a_resb) == 0){
                locctr += atoi(operand);
                init_obj_code();
                obj_code[strlen(obj_code)] = '\0';
                write_line_obj();
                add_text(3,obj_code);
            }
            else if(strcmp(op,a_byte) == 0){
                locctr += operand_len(operand);
                conv_byte(operand_len(operand),operand,obj_code);
                obj_code[strlen(obj_code)] = '\0';
                write_line_obj();
                add_text(operand_len(operand),obj_code);
            }
            else if(strcmp(op,a_word) == 0){
                locctr += 3;
                int temp = 0;
                char str[6];
                sscanf(operand,"%d",&temp);
                sprintf(obj_code,"%.6X",temp);
                obj_code[strlen(obj_code)] = '\0';
                write_line_obj();
                add_text(3,obj_code);
            }
            if(strlen(obj_code) >= 6)
                printf("obj_code: %X [%s]\n",last_locctr,obj_code);
            last_locctr = locctr;
        }
    }
    wr_text();
    wr_end();
    write_line_obj();
    fclose(listing_code);
}


int main(int argc, char*argv[]){
    argc = 2;
    int t = argc;
    argv[1] = "C:\\Users\\徐邦傑\\Desktop\\HW01\\test.sic";
    char fname[100];
    int i = 0;
    if (t == 2 ) {
        f = fopen(argv[1], "r");
        if (f) {
            printf("... Assembling %s!\n", argv[1]);
            pass1();
            printf("...... End of Pass 1; Program length = %6X.\n", prog_len);
            wr_header();
            printf("...... Contents in SymbTab:\n");
            print_symtab( symtab->next );
            fclose( f );
            strcpy( fname, argv[1] );
            //printf("%s\n",fname);
            for (i=0; (fname[i]!='.') && (fname[i]!='\0'); i++);
            fname[i++] = '.';
            fname[i++] = 'o';
            fname[i++] = 'b';
            fname[i++] = 'j';
            fname[i] = '\0';
            //intermediate_file = "C:\\Users\\徐邦傑\\Desktop\\HW01\\loc.txt";
            f = fopen(argv[1], "r");
            fobj = fopen(fname, "w");
            printf("...... Start of Pass 2.\n");
            pass2();
	    printf("Assembling succeeded.  %s is generated.\n", fname);
	    fclose( f );
	    fclose( fobj );
        } else {
            printf("Assemble syntax: [assemble soure_file_name]\n");
        } // f
    } else {
        printf("Assemble syntax: [assemble soure_file_name]\n");
    } // t
}
