%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>

#include "syntax_tree.h"
#include "syntax_analyzer.h"

int lines;
int pos_start;
int pos_end;
int comment_begin,comment_end;

void pass_node(char *text){
     yylval.node = new_syntax_tree_node(text);
}

/*****************声明和选项设置  end*****************/

%}



%%
 /* to do for students */
 /* two cases for you, pass_node will send flex's token to bison */
 //. { pos_start = pos_end; pos_end++; return ERROR; }
  //\s { pos_end += 1; pos_start=pos_end;}
 //\t { pos_end += 4; pos_start=pos_end;}
\+ 	{pos_start = pos_end; pos_end += 1; pass_node(yytext); return ADD;}
\- 	{pos_start = pos_end; pos_end += 1; pass_node(yytext); return SUB;}
"*"  {pos_start = pos_end; pos_end += 1; pass_node(yytext); return MUL;}
\/  {pos_start = pos_end; pos_end += 1; pass_node(yytext); return DIV;}
\n { lines++;pos_start=0; pos_end=1; }
;  {pos_start = pos_end; pos_end += 1; pass_node(yytext); return SEMI;}
,  {pos_start = pos_end; pos_end += 1; pass_node(yytext); return COMMA;}
" " { pos_end += strlen(yytext); pos_start=pos_end;}
\t { pos_end += strlen(yytext); pos_start=pos_end;}
int {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return INT;}
float {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return FLOAT;}
void {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return VOID;}
if {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return IF;}
else {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return ELSE;}
while {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return WHILE;}
return {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return RETURN;}
\> {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return MTHAN;}
\< {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return LTHAN;}
= {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return EQUAL;}
\>= {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return MTHANE;}
\<= {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return LTHANE;}
== {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return EQUALE;}
!= {pos_start = pos_end;pos_end += strlen(yytext);pass_node(yytext); return EQUALE;}
[a-zA-Z]+ {pos_start = pos_end;pos_end+=strlen(yytext);pass_node(yytext);return ID;}
\( {pos_start = pos_end; pos_end+=1; pass_node(yytext); return LSBRA;}
\) {pos_start = pos_end; pos_end+=1; pass_node(yytext); return RSBRA;}
\[ {pos_start = pos_end; pos_end+=1; pass_node(yytext); return LMBRA;}
\] {pos_start = pos_end; pos_end+=1; pass_node(yytext); return RMBRA;}
\{ {pos_start = pos_end; pos_end+=1; pass_node(yytext); return LLBRA;}
\} {pos_start = pos_end; pos_end+=1; pass_node(yytext); return RLBRA;}
0|[1-9][0-9]* {pos_start=pos_end;pos_end+=strlen(yytext);pass_node(yytext);return INTEGER;}
0|[0-9]+\.[0-9]*|[0-9]*\.[0-9]+ {pos_start=pos_end;pos_end+=strlen(yytext);pass_node(yytext);return FLOATPOINT;}
\/[*][^*]*[*]+([^\/\*][^*]*[*]+)*\/ {pos_start=0;pos_end=1;for(int i=0;i<strlen(yytext);i++) {if(yytext[i]=='\n') lines++; }}
\/[*].+ {pos_start=0;pos_end=1;for(int i=0;i<strlen(yytext);i++) {if(yytext[i]=='\n') lines++; }}
. { pos_start = pos_end; pos_end++; return ERROR; }

 /****请在此补全所有flex的模式与动作  end******/
%%
