# Pseudocode language specifications for use in this compiler and virtual machine

Most of the language remains intact as to the documentation published by Cambridge International Assesment. However, some language features had to be slightly modified as to allow for the parsing of the language, as the pseudocode is very unstructured in places.\
It also serves the purpose of specifying which features are actually included in the grammar subset handled, and which are not (OOP).

## Syntax Specification

### Program
```
<program> ::= <declaration>*
```

### Types
```
<datatype> ::= <primitive type> | <reference type>

<primitive type> ::= "INTEGER" | "REAL" | "BOOLEAN" | "CHAR"

<reference type> ::= "STRING"
```

### Statements and Blocks
```
<block> ::= <new line> <declaration>+

<declaration> ::= <car declaration> | <statement> | <array declaration> | <subroutine declaration> | <constant declaration>

<var declaration> ::= "DECLARE" <expression name> ":" <datatype> <new line>

<constant declaration> ::= "CONSTANT" <expression name> <assignment operator> <literal> <new line>

<array declaration> ::= <array1D declaration> | <array2D declaration>

<array1D declaration> ::= "DECLARE" <expression name> ":" "ARRAY" "[" <expression> ":" <expression> "]" "OF" <datatype> <new line>

<array2D declaration> ::= "DECLARE" <expression name> ":" "ARRAY" "[" <expression> ":" <expression> "," <expression> ":" <expression> "]" "OF" <datatype> <new line>

<subroutine declaration> ::= <function declaration> | <procedure declaration>

<function declaration> ::= "FUNCTION" <subroutine name> "(" <parameter list>? ")" "RETURNS" <datatype> <block> "ENDFUNCTION" <new line>

<procedure declaration> ::= "PROCEDURE" <subroutine name> "(" <parameter list>? ")" <block> "ENDPROCEDURE" <new line>

<statement> ::= <if statement> | <case statement> | <for statement> | <while statement> | <repeat statement> | <return statement> | <call statement> |
                <expression statement> | <input statement> | <output statement> | <openfile statement> | <closefile statement> | <readfile statement> | <writefile statement>

<if statement> ::= "IF" <expression> "THEN" <block> ( "ELSE" <block> )? "ENDIF" <new line>

<case statement> ::= "CASE" <constant expression> "OF" <case block> "ENDCASE" <new line>

<case block> ::= ( ( <new line> <expression> ":" <statement> <block>? ) | ( <new line> "OTHERWISE" ":" <statement> <block>? ) )*

<for statement> ::= "FOR" <expression name> <assignment operator> <expression> "TO" <expression> ( "STEP" <expression> )? <block> "NEXT" <expression name> <new line>

<while statement> ::= "WHILE" <expression> "DO" <block> "ENDWHILE" <new line>

<repeat statement> ::= "REPEAT" <block> "UNTIL" <expression> <new line>

<return statement> ::= "RETURN" <expression> <new line>

<call statement> ::= "CALL" <subroutine name> "(" <argument list>? ")" <new line>

<expression statement> ::= <expression> <new line>

<input statement> ::= "INPUT" <expression name> <new line>

<output statement> ::= "OUTPUT" <expression list> <new line>

<openfile statement> ::= "OPENFILE" <literal> "FOR" <file access spec> <new line>

<closefile statement> ::= "CLOSEFILE" <literal> <new line>

<readfile statement> ::= "READFILE" <literal>, <expression name>

<writefile statement> ::= "WRITEFILE" <literal>, <expression list>
```

### Expressions
```
<expression list> ::= <expression> ( "," <expression> )*

<constant expression> ::= <expression>

<expression> ::= <assignment expression>

<assignment expression> ::= <or expression> | <assignment>

<assignment> ::= <left side> <assignment operator> <assignment expression>

<left side> ::= <expression name> | <array access>

<or expression> ::= <and expression> ( "OR" <and expression> )*

<and expression> ::= <equality> ( "AND" <equality> )*

<equality> ::= <relational> ( ( "=" | "<>" ) <relational> )*

<relational> ::= <additive> ( ( ">" | "<" | ">=" | "<=" ) <additive> )*

<additive> ::= <factor> ( ( "+" | "-" ) <factor> )*

<factor> ::= <power> ( ( "*" | "/" | "MOD" | "DIV" ) <power> )*

<power> ::= <unary> ( "^" <unary> )*

<unary> ::= ( ( "+" | "-" | "NOT" ) <unary> ) | <concat expression>

<concat expression> ::= <primary> ( "&" <primary> )*

<primary> ::= <literal> | <expression name> | <function call> | ( "(" <expression> ")" ) | <array access>

<function call> ::= <subroutine name> "(" <argument list>? ")"

<argument list> ::= <expression list>

<array access> ::= <array1D access> | <array2D access>

<array1D access> ::= <expression name> "[" <expression> "]"

<array2D access> ::= <expression name> "[" <expression> "," <expression> "]"
```

### Tokens
```
<expression name> ::= <identifier>

<subroutine name> ::= <identifier>

<literal> ::= <int literal> | <real literal> | <char literal> | <string literal> | <boolean literal>

<int literal> ::= <digits>

<real literal> ::= <digits> "." <digits>

<boolean literal> ::= "TRUE" | "FALSE"

<char literal> ::= "'" <character> "'"

<string literal> ::= """ <string chars> """

<new line> ::= \n | EOF

<keyword> ::= "AND" | "APPEND" | "ARRAY" | "BOOLEAN" | "BYREF" | "CALL" | "CASE" | "CHAR" | "CLOSEFILE" | "CONSTANT" | "DECLARE" | "DIV" | "DO" | "ELSE" | "ENDCASE" |
              "ENDFUNCTION" | "ENDIF" | "ENDPROCEDURE" | "ENDWHILE" | "FALSE" | "FOR" | "FUNCTION" | "IF" | "INPUT" | "INTEGER" | "MOD" | "NEXT" | "NOT" | "OF" |
              "OPENFILE" | "OR" | "OTHERWISE" | "OUTPUT" | "PROCEDURE" | "READ" | "READFILE" | "REAL" | "REPEAT" | "RETURN" | "RETURNS" | "STEP" | "STRING" | "THEN" |
              "TO" | "TRUE" | "UNTIL" | "WHILE" | "WRITE" | "WRITEFILE"

<identifer> ::= Any alphanumeric string that starts with a letter and then contains any combination of letters and numbers but is also not a <keyword>
```

### Miscellaneous
```
<assignment operator> ::= "<-"

<parameter list> ::= "BYREF"? <parameter> ( "," "BYREF"? <parameter> )*

<parameter> ::= <expression name> ":" ( <datatype> | ( "ARRAY" ( ( "[" "]" ) | ( "[" "," "]" ) ) "OF" <datatype> ) )

<file access spec> ::= "READ" | "WRITE" | "APPEND"

<digits> ::= <digit>+

<digit> ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"

<character> ::= Any character except for '

<string chars> ::= <string char>*

<string char> ::= Any character except for "
```

## Internal representation

### Data Types

#### - INTEGER 

Represents a whole number, also known as an integer, or an `int` in most programming languages.\
Is represented internally using 4 bytes on a C stack.

#### - REAL

Represents a decimal number, also known as a `double` in C-like languages.\
Is represented internally using 8 bytes on a C stack.

#### - CHAR

Represents an ASCII character.\
Is represented using a single byte on a C stack.

#### - BOOLEAN

Represents a logic value, either TRUE or FALSE.\
Is represented internally as a single byte where 0 is FALSE and any other, usually 1, is TRUE.

#### - STRING

Represents a chain of characters that forms a piece of text.\
Represented internally as an object on the heap, and an 8 byte pointer to it on the operation stack.

#### - ARRAY

Represents a fixed size list of primitive datatypes.\
Represented internally as an object on the heap, and an 8 byte pointer to it on the operation stack.

#### - FILE

Represents an open file.\
Represented internally as an object on the heap, and an 8 byte pointer to it on the operation stack.

## Example programs

###
