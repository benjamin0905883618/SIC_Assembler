# SIC Assembler實作
因系統程式課需求，使用C語言實作Two-Pass Assembler

## Symbol Table結構
```
typedef struct sym_node {
    char l[7];
    int v;
    struct sym_node * next,* prev;

} symNODE;

symNODE *tail = NULL;
symNODE *symtab = NULL;
```
label(l) 為7個字元的字元陣列，Locctr(v)為整數，但基本上印出時會轉成十六進制。
在鏈結串列串接上，為了確保串接正確，除了增加prev的節點之外，也在下面增加了tail的節點，使各個Symbol Label可以依想像的順序照順序列出。
```
int flag = 1;
//插入一個節點到symbol table
symNODE* insert(char *s, int r) {
    //初始化鏈結串列開頭及結尾
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
    //插入新結點
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
```
外部的全域變數 flag 初始值為 1 ，用於確保插入時不會重複初始鏈結串列的頭尾，造成已插入的資料變成沒被任何人指向的垃圾空間。在插入上，將 tail 的 label 設為 "XXXXXXX" 是為確保後續在找尋結尾時的方便，可以換成其他設定或者不設。將 symtab 的下一個設為頭，就像 head 跟 tail 之間的關係，這裡的結構也類似這個關係。
## Search Symbol
```
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
```
輸入 label 後，尋找這個 label ，確保沒有重複定義或者是確認 branch 的位址，使用new_search時，可直接將 symtab 用於搜尋，也可以事先對輸入的字串做整齊的處理，避免未預期到的搜尋問題發生。
## PASS1
```
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
        insert(label,locctr);
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
```
根據課本Fig.2.4的Two-Pass演算法中的 PASS1 翻譯成C語言的程式，由於老師事先已經將一些會使用到的函示定義好，本身並沒有甚麼難度。
### lookup
```
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
```
### operand_len
```
int operand_len () {
    int i, l;
    l = strlen(operand);
    // for (i=0; i<l; i++) printf("operand[%d] = [%c].\n", i, operand[i]);
    if (operand[0] == 'C') l -= 3;
    else if (operand[0] == 'X') l = (l-3) / 2;
    // printf("find_length([%s] = %d.\n", operand, l);
    return l;
}
```
### readline
```
int readline(){
    int i, j, l, x;

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
       printf("[%6X] Read a line: label=[%s], op=[%s], operand=[%s].\n",
           locctr, label, op, operand);
       return 1;
    } else {
       return 0;
    }
}
```
readline() 函式進行會輸出接收到的 locctr , label , opcode , operand 供寫程式的人做檢查，確定接收到的與預期相同，而在 readline() 中也有進行註解的檢查，一發現有註解則會回傳0，不是註解則回傳1。
### writeline()
```
void write_line(FILE* F){
    line[0] = '\0';
    char str[6];
    sprintf(str,"%X",locctr);
    strcat(line,str);
    strcat(line,"\t");
    strcat(line,label);
    strcat(line,"\t");
    strcat(line,op);
    strcat(line,"\t");
    strcat(line,operand);
    strcat(line,"\n");
    //printf("%s",line);
    fprintf(F,("%s\n",line));
}
```
此為因應程式所需而補上的函式，用於在 .txt 檔上寫入一行包含 locctr 的line，至此 Pass1 已完成。
在此也附上課本 Fig.2.4 的 Pass1。
![](https://i.imgur.com/Sn2NbhN.png)
![](https://i.imgur.com/542Wh09.png)
![](https://i.imgur.com/47wTHNM.png)



