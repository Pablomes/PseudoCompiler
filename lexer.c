#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "common.h"

Node* createNode() {
    Node* node = malloc(sizeof(Node));
    if (node == NULL) exit(-1);

    node->isFinal = false;
    node->keyword = TOK_IDENTIFIER;
    for (int i = 0; i < 26; i++) {
        node->trans[i] = NULL;
    }

    return node;
}

Node* createTrie() {
    Node* node0 = createNode();
    Node* node1 = createNode();
    Node* node2 = createNode();
    Node* node3 = createNode();
    node3->isFinal = true;
    node3->keyword = (TokenType)TOK_AND;
    node2->trans[3] = node3;
    node1->trans[13] = node2;
    Node* node4 = createNode();
    Node* node5 = createNode();
    Node* node6 = createNode();
    Node* node7 = createNode();
    Node* node8 = createNode();
    node8->isFinal = true;
    node8->keyword = (TokenType)TOK_APPEND;
    node7->trans[3] = node8;
    node6->trans[13] = node7;
    node5->trans[4] = node6;
    node4->trans[15] = node5;
    node1->trans[15] = node4;
    Node* node9 = createNode();
    Node* node10 = createNode();
    Node* node11 = createNode();
    Node* node12 = createNode();
    node12->isFinal = true;
    node12->keyword = (TokenType)TOK_ARRAY;
    node11->trans[24] = node12;
    node10->trans[0] = node11;
    node9->trans[17] = node10;
    node1->trans[17] = node9;
    node0->trans[0] = node1;
    Node* node13 = createNode();
    Node* node14 = createNode();
    Node* node15 = createNode();
    Node* node16 = createNode();
    Node* node17 = createNode();
    Node* node18 = createNode();
    Node* node19 = createNode();
    node19->isFinal = true;
    node19->keyword = (TokenType)TOK_BOOLEAN;
    node18->trans[13] = node19;
    node17->trans[0] = node18;
    node16->trans[4] = node17;
    node15->trans[11] = node16;
    node14->trans[14] = node15;
    node13->trans[14] = node14;
    Node* node20 = createNode();
    Node* node21 = createNode();
    Node* node22 = createNode();
    Node* node23 = createNode();
    node23->isFinal = true;
    node23->keyword = (TokenType)TOK_BYREF;
    node22->trans[5] = node23;
    node21->trans[4] = node22;
    node20->trans[17] = node21;
    node13->trans[24] = node20;
    node0->trans[1] = node13;
    Node* node24 = createNode();
    Node* node25 = createNode();
    Node* node26 = createNode();
    Node* node27 = createNode();
    node27->isFinal = true;
    node27->keyword = (TokenType)TOK_CALL;
    node26->trans[11] = node27;
    node25->trans[11] = node26;
    Node* node28 = createNode();
    Node* node29 = createNode();
    node29->isFinal = true;
    node29->keyword = (TokenType)TOK_CASE;
    node28->trans[4] = node29;
    node25->trans[18] = node28;
    node24->trans[0] = node25;
    Node* node30 = createNode();
    Node* node31 = createNode();
    Node* node32 = createNode();
    node32->isFinal = true;
    node32->keyword = (TokenType)TOK_CHAR;
    node31->trans[17] = node32;
    node30->trans[0] = node31;
    node24->trans[7] = node30;
    Node* node33 = createNode();
    Node* node34 = createNode();
    Node* node35 = createNode();
    Node* node36 = createNode();
    Node* node37 = createNode();
    Node* node38 = createNode();
    Node* node39 = createNode();
    Node* node40 = createNode();
    node40->isFinal = true;
    node40->keyword = (TokenType)TOK_CLOSEFILE;
    node39->trans[4] = node40;
    node38->trans[11] = node39;
    node37->trans[8] = node38;
    node36->trans[5] = node37;
    node35->trans[4] = node36;
    node34->trans[18] = node35;
    node33->trans[14] = node34;
    node24->trans[11] = node33;
    Node* node41 = createNode();
    Node* node42 = createNode();
    Node* node43 = createNode();
    Node* node44 = createNode();
    Node* node45 = createNode();
    Node* node46 = createNode();
    Node* node47 = createNode();
    node47->isFinal = true;
    node47->keyword = (TokenType)TOK_CONSTANT;
    node46->trans[19] = node47;
    node45->trans[13] = node46;
    node44->trans[0] = node45;
    node43->trans[19] = node44;
    node42->trans[18] = node43;
    node41->trans[13] = node42;
    node24->trans[14] = node41;
    node0->trans[2] = node24;
    Node* node48 = createNode();
    Node* node49 = createNode();
    Node* node50 = createNode();
    Node* node51 = createNode();
    Node* node52 = createNode();
    Node* node53 = createNode();
    Node* node54 = createNode();
    node54->isFinal = true;
    node54->keyword = (TokenType)TOK_DECLARE;
    node53->trans[4] = node54;
    node52->trans[17] = node53;
    node51->trans[0] = node52;
    node50->trans[11] = node51;
    node49->trans[2] = node50;
    node48->trans[4] = node49;
    Node* node55 = createNode();
    Node* node56 = createNode();
    node56->isFinal = true;
    node56->keyword = (TokenType)TOK_DIV;
    node55->trans[21] = node56;
    node48->trans[8] = node55;
    Node* node57 = createNode();
    node57->isFinal = true;
    node57->keyword = (TokenType)TOK_DO;
    node48->trans[14] = node57;
    node0->trans[3] = node48;
    Node* node58 = createNode();
    Node* node59 = createNode();
    Node* node60 = createNode();
    Node* node61 = createNode();
    node61->isFinal = true;
    node61->keyword = (TokenType)TOK_ELSE;
    node60->trans[4] = node61;
    node59->trans[18] = node60;
    node58->trans[11] = node59;
    Node* node62 = createNode();
    Node* node63 = createNode();
    Node* node64 = createNode();
    Node* node65 = createNode();
    Node* node66 = createNode();
    Node* node67 = createNode();
    node67->isFinal = true;
    node67->keyword = (TokenType)TOK_ENDCASE;
    node66->trans[4] = node67;
    node65->trans[18] = node66;
    node64->trans[0] = node65;
    node63->trans[2] = node64;
    Node* node68 = createNode();
    Node* node69 = createNode();
    Node* node70 = createNode();
    Node* node71 = createNode();
    Node* node72 = createNode();
    Node* node73 = createNode();
    Node* node74 = createNode();
    Node* node75 = createNode();
    node75->isFinal = true;
    node75->keyword = (TokenType)TOK_ENDFUNCTION;
    node74->trans[13] = node75;
    node73->trans[14] = node74;
    node72->trans[8] = node73;
    node71->trans[19] = node72;
    node70->trans[2] = node71;
    node69->trans[13] = node70;
    node68->trans[20] = node69;
    node63->trans[5] = node68;
    Node* node76 = createNode();
    Node* node77 = createNode();
    node77->isFinal = true;
    node77->keyword = (TokenType)TOK_ENDIF;
    node76->trans[5] = node77;
    node63->trans[8] = node76;
    Node* node78 = createNode();
    Node* node79 = createNode();
    Node* node80 = createNode();
    Node* node81 = createNode();
    Node* node82 = createNode();
    Node* node83 = createNode();
    Node* node84 = createNode();
    Node* node85 = createNode();
    Node* node86 = createNode();
    node86->isFinal = true;
    node86->keyword = (TokenType)TOK_ENDPROCEDURE;
    node85->trans[4] = node86;
    node84->trans[17] = node85;
    node83->trans[20] = node84;
    node82->trans[3] = node83;
    node81->trans[4] = node82;
    node80->trans[2] = node81;
    node79->trans[14] = node80;
    node78->trans[17] = node79;
    node63->trans[15] = node78;
    Node* node87 = createNode();
    Node* node88 = createNode();
    Node* node89 = createNode();
    Node* node90 = createNode();
    Node* node91 = createNode();
    node91->isFinal = true;
    node91->keyword = (TokenType)TOK_ENDWHILE;
    node90->trans[4] = node91;
    node89->trans[11] = node90;
    node88->trans[8] = node89;
    node87->trans[7] = node88;
    node63->trans[22] = node87;
    node62->trans[3] = node63;
    node58->trans[13] = node62;
    node0->trans[4] = node58;
    Node* node92 = createNode();
    Node* node93 = createNode();
    Node* node94 = createNode();
    Node* node95 = createNode();
    Node* node96 = createNode();
    node96->isFinal = true;
    node96->keyword = (TokenType)TOK_FALSE;
    node95->trans[4] = node96;
    node94->trans[18] = node95;
    node93->trans[11] = node94;
    node92->trans[0] = node93;
    Node* node97 = createNode();
    Node* node98 = createNode();
    node98->isFinal = true;
    node98->keyword = (TokenType)TOK_FOR;
    node97->trans[17] = node98;
    node92->trans[14] = node97;
    Node* node99 = createNode();
    Node* node100 = createNode();
    Node* node101 = createNode();
    Node* node102 = createNode();
    Node* node103 = createNode();
    Node* node104 = createNode();
    Node* node105 = createNode();
    node105->isFinal = true;
    node105->keyword = (TokenType)TOK_FUNCTION;
    node104->trans[13] = node105;
    node103->trans[14] = node104;
    node102->trans[8] = node103;
    node101->trans[19] = node102;
    node100->trans[2] = node101;
    node99->trans[13] = node100;
    node92->trans[20] = node99;
    node0->trans[5] = node92;
    Node* node106 = createNode();
    Node* node107 = createNode();
    node107->isFinal = true;
    node107->keyword = (TokenType)TOK_IF;
    node106->trans[5] = node107;
    Node* node108 = createNode();
    Node* node109 = createNode();
    Node* node110 = createNode();
    Node* node111 = createNode();
    node111->isFinal = true;
    node111->keyword = (TokenType)TOK_INPUT;
    node110->trans[19] = node111;
    node109->trans[20] = node110;
    node108->trans[15] = node109;
    Node* node112 = createNode();
    Node* node113 = createNode();
    Node* node114 = createNode();
    Node* node115 = createNode();
    Node* node116 = createNode();
    node116->isFinal = true;
    node116->keyword = (TokenType)TOK_INTEGER;
    node115->trans[17] = node116;
    node114->trans[4] = node115;
    node113->trans[6] = node114;
    node112->trans[4] = node113;
    node108->trans[19] = node112;
    node106->trans[13] = node108;
    node0->trans[8] = node106;
    Node* node117 = createNode();
    Node* node118 = createNode();
    Node* node119 = createNode();
    node119->isFinal = true;
    node119->keyword = (TokenType)TOK_MOD;
    node118->trans[3] = node119;
    node117->trans[14] = node118;
    node0->trans[12] = node117;
    Node* node120 = createNode();
    Node* node121 = createNode();
    Node* node122 = createNode();
    Node* node123 = createNode();
    node123->isFinal = true;
    node123->keyword = (TokenType)TOK_NEXT;
    node122->trans[19] = node123;
    node121->trans[23] = node122;
    node120->trans[4] = node121;
    Node* node124 = createNode();
    Node* node125 = createNode();
    node125->isFinal = true;
    node125->keyword = (TokenType)TOK_NOT;
    node124->trans[19] = node125;
    node120->trans[14] = node124;
    node0->trans[13] = node120;
    Node* node126 = createNode();
    Node* node127 = createNode();
    node127->isFinal = true;
    node127->keyword = (TokenType)TOK_OF;
    node126->trans[5] = node127;
    Node* node128 = createNode();
    Node* node129 = createNode();
    Node* node130 = createNode();
    Node* node131 = createNode();
    Node* node132 = createNode();
    Node* node133 = createNode();
    Node* node134 = createNode();
    node134->isFinal = true;
    node134->keyword = (TokenType)TOK_OPENFILE;
    node133->trans[4] = node134;
    node132->trans[11] = node133;
    node131->trans[8] = node132;
    node130->trans[5] = node131;
    node129->trans[13] = node130;
    node128->trans[4] = node129;
    node126->trans[15] = node128;
    Node* node135 = createNode();
    node135->isFinal = true;
    node135->keyword = (TokenType)TOK_OR;
    node126->trans[17] = node135;
    Node* node136 = createNode();
    Node* node137 = createNode();
    Node* node138 = createNode();
    Node* node139 = createNode();
    Node* node140 = createNode();
    Node* node141 = createNode();
    Node* node142 = createNode();
    Node* node143 = createNode();
    node143->isFinal = true;
    node143->keyword = (TokenType)TOK_OTHERWISE;
    node142->trans[4] = node143;
    node141->trans[18] = node142;
    node140->trans[8] = node141;
    node139->trans[22] = node140;
    node138->trans[17] = node139;
    node137->trans[4] = node138;
    node136->trans[7] = node137;
    node126->trans[19] = node136;
    Node* node144 = createNode();
    Node* node145 = createNode();
    Node* node146 = createNode();
    Node* node147 = createNode();
    Node* node148 = createNode();
    node148->isFinal = true;
    node148->keyword = (TokenType)TOK_OUTPUT;
    node147->trans[19] = node148;
    node146->trans[20] = node147;
    node145->trans[15] = node146;
    node144->trans[19] = node145;
    node126->trans[20] = node144;
    node0->trans[14] = node126;
    Node* node149 = createNode();
    Node* node150 = createNode();
    Node* node151 = createNode();
    Node* node152 = createNode();
    Node* node153 = createNode();
    Node* node154 = createNode();
    Node* node155 = createNode();
    Node* node156 = createNode();
    Node* node157 = createNode();
    node157->isFinal = true;
    node157->keyword = (TokenType)TOK_PROCEDURE;
    node156->trans[4] = node157;
    node155->trans[17] = node156;
    node154->trans[20] = node155;
    node153->trans[3] = node154;
    node152->trans[4] = node153;
    node151->trans[2] = node152;
    node150->trans[14] = node151;
    node149->trans[17] = node150;
    node0->trans[15] = node149;
    Node* node158 = createNode();
    Node* node159 = createNode();
    Node* node160 = createNode();
    Node* node161 = createNode();
    Node* node162 = createNode();
    Node* node163 = createNode();
    Node* node164 = createNode();
    Node* node165 = createNode();
    node165->isFinal = true;
    node165->keyword = (TokenType)TOK_READFILE;
    node164->trans[4] = node165;
    node163->trans[11] = node164;
    node162->trans[8] = node163;
    node161->trans[5] = node162;
    node161->isFinal = true;
    node161->keyword = (TokenType)TOK_READ;
    node160->trans[3] = node161;
    Node* node166 = createNode();
    node166->isFinal = true;
    node166->keyword = (TokenType)TOK_REAL;
    node160->trans[11] = node166;
    node159->trans[0] = node160;
    Node* node167 = createNode();
    Node* node168 = createNode();
    Node* node169 = createNode();
    Node* node170 = createNode();
    node170->isFinal = true;
    node170->keyword = (TokenType)TOK_REPEAT;
    node169->trans[19] = node170;
    node168->trans[0] = node169;
    node167->trans[4] = node168;
    node159->trans[15] = node167;
    Node* node171 = createNode();
    Node* node172 = createNode();
    Node* node173 = createNode();
    Node* node174 = createNode();
    Node* node175 = createNode();
    node175->isFinal = true;
    node175->keyword = (TokenType)TOK_RETURNS;
    node174->trans[18] = node175;
    node174->isFinal = true;
    node174->keyword = (TokenType)TOK_RETURN;
    node173->trans[13] = node174;
    node172->trans[17] = node173;
    node171->trans[20] = node172;
    node159->trans[19] = node171;
    node158->trans[4] = node159;
    node0->trans[17] = node158;
    Node* node176 = createNode();
    Node* node177 = createNode();
    Node* node178 = createNode();
    Node* node179 = createNode();
    node179->isFinal = true;
    node179->keyword = (TokenType)TOK_STEP;
    node178->trans[15] = node179;
    node177->trans[4] = node178;
    Node* node180 = createNode();
    Node* node181 = createNode();
    Node* node182 = createNode();
    Node* node183 = createNode();
    node183->isFinal = true;
    node183->keyword = (TokenType)TOK_STRING;
    node182->trans[6] = node183;
    node181->trans[13] = node182;
    node180->trans[8] = node181;
    node177->trans[17] = node180;
    node176->trans[19] = node177;
    node0->trans[18] = node176;
    Node* node184 = createNode();
    Node* node185 = createNode();
    Node* node186 = createNode();
    Node* node187 = createNode();
    node187->isFinal = true;
    node187->keyword = (TokenType)TOK_THEN;
    node186->trans[13] = node187;
    node185->trans[4] = node186;
    node184->trans[7] = node185;
    Node* node188 = createNode();
    node188->isFinal = true;
    node188->keyword = (TokenType)TOK_TO;
    node184->trans[14] = node188;
    Node* node189 = createNode();
    Node* node190 = createNode();
    Node* node191 = createNode();
    node191->isFinal = true;
    node191->keyword = (TokenType)TOK_TRUE;
    node190->trans[4] = node191;
    node189->trans[20] = node190;
    node184->trans[17] = node189;
    node0->trans[19] = node184;
    Node* node192 = createNode();
    Node* node193 = createNode();
    Node* node194 = createNode();
    Node* node195 = createNode();
    Node* node196 = createNode();
    node196->isFinal = true;
    node196->keyword = (TokenType)TOK_UNTIL;
    node195->trans[11] = node196;
    node194->trans[8] = node195;
    node193->trans[19] = node194;
    node192->trans[13] = node193;
    node0->trans[20] = node192;
    Node* node197 = createNode();
    Node* node198 = createNode();
    Node* node199 = createNode();
    Node* node200 = createNode();
    Node* node201 = createNode();
    node201->isFinal = true;
    node201->keyword = (TokenType)TOK_WHILE;
    node200->trans[4] = node201;
    node199->trans[11] = node200;
    node198->trans[8] = node199;
    node197->trans[7] = node198;
    Node* node202 = createNode();
    Node* node203 = createNode();
    Node* node204 = createNode();
    Node* node205 = createNode();
    Node* node206 = createNode();
    Node* node207 = createNode();
    Node* node208 = createNode();
    Node* node209 = createNode();
    node209->isFinal = true;
    node209->keyword = (TokenType)TOK_WRITEFILE;
    node208->trans[4] = node209;
    node207->trans[11] = node208;
    node206->trans[8] = node207;
    node205->trans[5] = node206;
    node205->isFinal = true;
    node205->keyword = (TokenType)TOK_WRITE;
    node204->trans[4] = node205;
    node203->trans[19] = node204;
    node202->trans[8] = node203;
    node197->trans[17] = node202;
    node0->trans[22] = node197;

    return node0;
}

void freeNode(Node* node) {
    for (int i = 0; i < 26; i++) {
        if (node->trans[i] != NULL) freeNode(node->trans[i]);
    }

    free(node);
}

void freeTrie(Lexer* lexer) {
    freeNode(lexer->keywordTrie);
}

void initLexer(Lexer* lexer, const char* source) {
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->col = 1;
    lexer->keywordTrie = createTrie();
    lexer->array = initTokenArray();
}

void freeLexer(Lexer* lexer) {
    freeTokenArray(lexer->array);
    freeTrie(lexer);
    //free(lexer);
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isKeywordAlpha(char c) {
    return (c >= 'A' && c <= 'Z');
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static bool isAtEnd(Lexer* lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer* lexer) {
    lexer->current++;
    lexer->col++;
    return lexer->current[-1];
}

static char peek(Lexer* lexer) {
    return *lexer->current;
}

static char peekNext(Lexer* lexer) {
    if (isAtEnd(lexer)) return '\0';
    return lexer->current[1];
}

static bool match(Lexer* lexer, char expected) {
    if (isAtEnd(lexer)) return false;
    if (*lexer->current != expected) return false;
    lexer->current++;
    lexer->col++;
    return true;
}

static Token makeToken(Lexer* lexer, TokenType type) {
    Token token;
    token.type = type;
    token.start = lexer->start;
    token.length = (int)(lexer->current - lexer->start);
    token.line = lexer->line;
    token.col = lexer->col - token.length;
    return token;
}

static bool addNewToken(Lexer* lexer, TokenType type) {
    Token tok = makeToken(lexer, type);
    return addToken(lexer->array, tok);
}

static Token errorToken(Lexer* lexer, const char* message) {
    Token token;
    token.type = TOK_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = lexer->line;
    token.col = lexer->col - 1;
    return token;
}

static bool addErrorToken(Lexer* lexer, const char* message) {
    return addToken(lexer->array, errorToken(lexer, message));
}

static void skipWhitespace(Lexer* lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '/':
                if (peekNext(lexer) == '/') {
                    while (peek(lexer) != '\n' && !isAtEnd(lexer)) advance(lexer);
                } else {
                    return;
                }
            default:
                return;
        }
    }
}

static TokenType checkKeyword(Lexer* lexer) {
    Node* currentNode = lexer->keywordTrie;
    const char* current = lexer->start;

    while (current < lexer->current) {
        if (!isKeywordAlpha(*current) || currentNode->trans[(int)(*current - 65)] == NULL) {
            return TOK_IDENTIFIER;
        }

        currentNode = currentNode->trans[(int)(*current - 65)];
        current++;
    }

    if (currentNode->isFinal) return currentNode->keyword;

    return TOK_IDENTIFIER;
}

static Token identifier(Lexer* lexer) {
    while (isAlpha(peek(lexer)) || isDigit(peek(lexer))) advance(lexer);
    return makeToken(lexer, checkKeyword(lexer));
}

static Token number(Lexer* lexer) {
    while (isDigit(peek(lexer))) advance(lexer);

    if (peek(lexer) == '.' && isDigit(peekNext(lexer))) {
        advance(lexer);

        while (isDigit(peek(lexer))) advance(lexer);

        return makeToken(lexer, TOK_REAL_LIT);
    }

    return makeToken(lexer, TOK_INT_LIT);
}

static Token string(Lexer* lexer) {
    while (peek(lexer) != '"' && !isAtEnd(lexer)) {
        if (peek(lexer) == '\n')  { lexer->line++; lexer->col = 1; }
        advance(lexer);
    }

    if (isAtEnd(lexer)) return errorToken(lexer, "Unterminated string.");

    advance(lexer);
    return makeToken(lexer, TOK_STRING_LIT);
}

static Token character(Lexer* lexer) {
    if (!isAlpha(peek(lexer)) && !isDigit(peek(lexer))) {
        return errorToken(lexer, "Expected character literal.");
    }

    advance(lexer);

    if (!match(lexer, '\'')) {
        while (peek(lexer) != '\'' && !isAtEnd(lexer)) {
            if (peek(lexer) == '\n') { lexer->line++; lexer->col = 1; }
            advance(lexer);
        }

        if (isAtEnd(lexer)) return errorToken(lexer, "Unterminated character literal.");

        advance(lexer);
        return errorToken(lexer, "Expected single character literal.");
    }

    return makeToken(lexer, TOK_CHAR_LIT);
}

static Token scanToken(Lexer* lexer) {
    skipWhitespace(lexer);
    lexer->start = lexer->current;

    if (isAtEnd(lexer)) return makeToken(lexer, TOK_EOF);

    char c = advance(lexer);

    if (isAlpha(c)) return identifier(lexer);
    if (isDigit(c)) return number(lexer);

    switch (c) {
        case '(': return makeToken(lexer, TOK_LEFT_PAREN);
        case ')': return makeToken(lexer, TOK_RIGHT_PAREN);
        case '[': return makeToken(lexer, TOK_LEFT_BRACKET);
        case ']': return makeToken(lexer, TOK_RIGHT_BRACKET);
        case ',': return makeToken(lexer, TOK_COMMA);
        case '.': return makeToken(lexer, TOK_DOT);
        case ':': return makeToken(lexer, TOK_COLON);
        case '-': return makeToken(lexer, TOK_MINUS);
        case '+': return makeToken(lexer, TOK_PLUS);
        case '/': return makeToken(lexer, TOK_SLASH);
        case '*': return makeToken(lexer, TOK_STAR);
        case '^': return makeToken(lexer, TOK_CARAT);
        case '=': return makeToken(lexer, TOK_EQUAL);
        case '&': return makeToken(lexer, TOK_AMPERSAND);
        case '\n':
            lexer->line++;
            lexer->col = 1;
            return makeToken(lexer, TOK_NEW_LINE);
        case '<': {
            TokenType type = TOK_LESS;
            if (match(lexer, '=')) type = TOK_LESS_EQUAL;
            else if (match(lexer, '>')) type = TOK_NOT_EQUAL;
            else if (match(lexer, '-')) type = TOK_ASSIGN;

            return makeToken(lexer, type);
        }
        case '>':
            return makeToken(lexer, match(lexer, '=') ? TOK_GREATER_EQUAL : TOK_GREATER);
        case '"': return string(lexer);
        case '\'': return character(lexer);
    }

    return errorToken(lexer, "Unexpected character.");
}

bool scanSource(Lexer* lexer) {
    Token token;

    do {
        token = scanToken(lexer);
        bool res = addToken(lexer->array, token);

        if (!res) return false;
    } while (token.type != TOK_EOF);

    return true;
}

void printTokens(Lexer* lexer) {
    printTokenArray(lexer->array);
}