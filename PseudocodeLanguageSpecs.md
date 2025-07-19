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

<constant declaration> ::= "CONSTANT" <expression name> "=" <literal> <new line>

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

<comment> ::= Anything preceeded by "//"
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

## Built-in functions

These functions are all built into the VM in C. However, a header for a pseudocode equivalent is shown.

### - SUBSTRING
```
FUNCTION SUBSTRING(Str:STRING, InitPos:INTEGER, Length:INTEGER) RETURNS STRING
```
Takes a string, an initial position and a length and returns the substring of set length starting at that position. String is 1-indexed.

### - LENGTH
```
FUNCTION LENGTH(Str:STRING) RETURNS INTEGER
```
Takes a string and returns its length.

### - LCASE
```
FUNCTION LCASE(Str:STRING) RETURNS STRING
```
Takes a string and returns its lower case equivalent.

### - UCASE
```
FUNCTION UCASE(Str:STRING) RETURNS STRING
```
Takes a string and returns its upper case equivalent.

### - RANDOMBETWEEN
```
FUNCTION RANDOMBETWEEN(Min:INTEGER, Max:INTEGER) RETURNS INTEGER
```
Takes an integer range and returns a random integer in the range [Min, Max]

### - RND
```
FUNCTION RND() RETURNS REAL
```
Returns a random real in the range [0, 1]

### - INT
```
FUNCTION INT(x:REAL) RETURNS INTEGER
```
Takes a real number x, and returns its truncated integer.

### - EOF
```
FUNCTION EOF(Filename:STRING) RETURNS BOOLEAN
```
Takes a file name and returns TRUE if the end of the file has been reached. Returns FALSE otherwise. The file name must be known at compile time, via a string literal.

### - CHARAT
```
FUNCTION CHARAT(Str:STRING, Pos:INTEGER) RETURNS CHAR
```
Takes a string and returns the character at position Pos. String is 1-indexed.

## Example programs

### Prime Number Generator
```
FUNCTION IsPrime(n:INTEGER) RETURNS BOOLEAN
IF n <= 1 THEN
RETURN FALSE
ENDIF
FOR i <- 2 TO INT(n^0.5)
IF n MOD i = 0 THEN
RETURN FALSE
ENDIF
NEXT i
RETURN TRUE
ENDFUNCTION

// Main program
DECLARE Limit:INTEGER
OUTPUT "Enter the upper limit for prime numbers:"
INPUT Limit
OUTPUT "Prime numbers up to ", Limit, ":"
FOR num <- 2 TO Limit
IF IsPrime(num) THEN
OUTPUT num
ENDIF
NEXT num
```

### Palindrome Checker
```
FUNCTION CleanString(str:STRING) RETURNS STRING
	DECLARE CleanStr : STRING
	CleanStr <- ""
	FOR i <- 1 TO LENGTH(str)
		IF CHARAT(str, i) >= 'a' AND CHARAT(str, i) <= 'z' THEN
			CleanStr <- CleanStr & SUBSTRING(str, i, 1)
		ENDIF
	NEXT i
	RETURN CleanStr
ENDFUNCTION

FUNCTION IsPalindrome(str:STRING) RETURNS BOOLEAN
	DECLARE CleanStr : STRING
	CleanStr <- CleanString(LCASE(str))

	DECLARE Left : INTEGER
	DECLARE Right : INTEGER
	Left <- 1
	Right <- LENGTH(CleanStr)

	WHILE Left < Right DO
		IF CHARAT(CleanStr, Left) <> CHARAT(CleanStr, Right) THEN
			RETURN FALSE
		ENDIF
		Left <- Left + 1
		Right <- Right - 1
	ENDWHILE
	
	RETURN TRUE
ENDFUNCTION

//main program
DECLARE Text:STRING
OUTPUT "Enter a word or phrase to check if it's a palindrome:"
INPUT Text

IF IsPalindrome(Text) THEN
	OUTPUT "<", Text, "> is a palindrome."
ELSE
	OUTPUT "<", Text, "> is not a palindrome."
ENDIF
```

### Binary Search
```
FUNCTION BinarySearch(A:ARRAY[] OF INTEGER, Target:INTEGER, n:INTEGER) RETURNS INTEGER
	
	DECLARE Left:INTEGER
	DECLARE Right:INTEGER
	DECLARE Mid:INTEGER

	Left <- 1
	Right <- n
	
	WHILE Left <= Right DO
		Mid <- (Left + Right) DIV 2

		IF A[Mid] = Target THEN
			RETURN Mid
		ELSE
			IF A[Mid] < Target THEN
				Left <- Mid + 1
			ELSE
				Right <- Mid - 1
			ENDIF
		ENDIF
	ENDWHILE

	RETURN -1
ENDFUNCTION

// MAIN PROGRAM

CONSTANT ArraySize = 10
DECLARE Numbers : ARRAY[1 : ArraySize] OF INTEGER

OUTPUT "Enter ", ArraySize, " numbers in ascending order:"
FOR i <- 1 TO ArraySize
	INPUT Numbers[i]
NEXT i

DECLARE Target : INTEGER
OUTPUT "Enter the number to search for:"
INPUT Target

DECLARE Result : INTEGER
Result <- BinarySearch(Numbers, Target, ArraySize)

IF Result = -1 THEN
	OUTPUT Target, " was not found in the array."
ELSE
	OUTPUT Target, " was found at index ", Result, "."
ENDIF
```

### Matrix Multiplication
```
PROCEDURE MultiplyMatrices(A:ARRAY [,] OF INTEGER, B:ARRAY [,] OF INTEGER, C:ARRAY [,] OF INTEGER, m:INTEGER, n:INTEGER, p:INTEGER)
	FOR i <- 1 TO m
		FOR j <- 1 TO p
			C[i, j] <- 0
			FOR k <- 1 TO n
				C[i, j] <- C[i, j] + A[i, k] * B[k, j]
			NEXT k
		NEXT j
	NEXT i
ENDPROCEDURE

CONSTANT M = 2
CONSTANT N = 3
CONSTANT P = 2

DECLARE MatrixA : ARRAY [1:M, 1:N] OF INTEGER
DECLARE MatrixB : ARRAY [1:N, 1:P] OF INTEGER
DECLARE Result : ARRAY [1:M, 1:P] OF INTEGER

OUTPUT "Enter elements for Matrix A (", M, "x", N, "):"
FOR i <- 1 TO M
	FOR j <- 1 TO N
		OUTPUT "num: "
		INPUT MatrixA[i, j]
	NEXT j
NEXT i

OUTPUT "Enter elements for Matrix B (", N, "x", P, "):"
FOR i <- 1 TO N
	FOR j <- 1 TO P
		OUTPUT "num: "
		INPUT MatrixB[i, j]
	NEXT j
NEXT i

CALL MultiplyMatrices(MatrixA, MatrixB, Result, M, N, P)

OUTPUT "Resultant Matrix:"
FOR i <- 1 TO M
	FOR j <- 1 TO P
		OUTPUT Result[i, j], " "
	NEXT j
	OUTPUT ""
NEXT i
```

### Student Record System
```
DECLARE Name : STRING
DECLARE ID : STRING
DECLARE Grade : STRING

PROCEDURE SaveStudentRecord(Name:STRING, ID:STRING, Grade:STRING)
	OPENFILE "Students.txt" FOR APPEND
	WRITEFILE "Students.txt", Name & "," & ID & "," & Grade
	CLOSEFILE "Students.txt"
ENDPROCEDURE

PROCEDURE DisplayAllRecords()
	DECLARE Record:STRING
	OPENFILE "Students.txt" FOR READ
	WHILE NOT EOF("Students.txt") DO
		READFILE "Students.txt", Record
		OUTPUT Record
	ENDWHILE
	CLOSEFILE "Students.txt"
ENDPROCEDURE

DECLARE Choice : INTEGER
Choice <- 0

REPEAT
	OUTPUT "Student Record System"
	OUTPUT "1. Add Student Record"
	OUTPUT "2. Display All Records"
	OUTPUT "3. Exit"
	OUTPUT "Enter your choice (1-3):"
	INPUT Choice

	CASE Choice OF
		1:
			OUTPUT "Enter student name:"
			INPUT Name
			OUTPUT "Enter student ID:"
			INPUT ID
			OUTPUT "Enter student grade:"
			INPUT Grade
			CALL SaveStudentRecord(Name, ID, Grade)
			OUTPUT "Record saved successfully"
		2:
			OUTPUT "Student Records:"
			CALL DisplayAllRecords()
		3:
			OUTPUT "Exiting program. Goodbye!"
		OTHERWISE: OUTPUT "Invalid choice. Please try again."
	ENDCASE

UNTIL Choice = 3
```

### Simple Banking System
```
CONSTANT MaxAccounts = 100

DECLARE AccountNumbers : ARRAY[1 : MaxAccounts] OF STRING
DECLARE Balances : ARRAY[1 : MaxAccounts] OF REAL

DECLARE CurrentAccounts : INTEGER
CurrentAccounts <- 0

PROCEDURE CreateAccount(AccountNumber : STRING, InitialBalance : REAL)
	IF CurrentAccounts < MaxAccounts THEN
		CurrentAccounts <- CurrentAccounts + 1
		AccountNumbers[CurrentAccounts] <- AccountNumber
		Balances[CurrentAccounts] <- InitialBalance
		OUTPUT "Account created successfully."
	ELSE
		OUTPUT "Error: Maximum number of accounts reached."
	ENDIF
ENDPROCEDURE

FUNCTION FindAccountIndex(AccountNumber : STRING) RETURNS INTEGER
	FOR i <- 1 TO CurrentAccounts
		IF AccountNumbers[i] = AccountNumber THEN
			RETURN i
		ENDIF
	NEXT i
	RETURN -1
ENDFUNCTION

PROCEDURE Deposit(AccountNumber:STRING, Amount:REAL)
	DECLARE Index : INTEGER
	Index <- FindAccountIndex(AccountNumber)
	IF Index <> -1 THEN
		Balances[Index] <- Balances[Index] + Amount
		OUTPUT "Deposit successful. New balance: ", Balances[Index]
	ELSE
		OUTPUT "Error: Account not found."
	ENDIF
ENDPROCEDURE

PROCEDURE Withdraw(AccountNumber:STRING, Amount:REAL)
	DECLARE Index : INTEGER
	Index <- FindAccountIndex(AccountNumber)
	IF Index <> -1 THEN
		IF Balances[Index] >= Amount THEN
			Balances[Index] <- Balances[Index] - Amount
			OUTPUT "Withdrawal successful. New balance: ", Balances[Index]
		ELSE
			OUTPUT "Error: Insufficient funds."
		ENDIF
	ELSE
		OUTPUT "Error: Account not found."
	ENDIF
ENDPROCEDURE

PROCEDURE CheckBalance(AccountNumber:STRING)
	DECLARE Index : INTEGER
	Index <- FindAccountIndex(AccountNumber)
	IF Index <> -1 THEN
		OUTPUT "Current balance: ", Balances[Index]
	ELSE
		OUTPUT "Error: Account not found."
	ENDIF
ENDPROCEDURE

// Main program
DECLARE Choice : INTEGER
Choice <- 5

REPEAT
	OUTPUT "Simple Banking System"
	OUTPUT "1. Create Account"
	OUTPUT "2. Deposit"
	OUTPUT "3. Withdraw"
	OUTPUT "4. Check Balance"
	OUTPUT "5. Exit"
	OUTPUT "Enter your choice (1-5):"
	INPUT Choice

	CASE Choice OF
		1:
			DECLARE AccountNumber:STRING
			DECLARE Amount : REAL
			OUTPUT "Enter account number:"
			INPUT AccountNumber
			OUTPUT "Enter initial balance:"
			INPUT Amount
			OUTPUT Amount
			CALL CreateAccount(AccountNumber, Amount)
		2:
			DECLARE AccountNumber:STRING
			DECLARE Amount : REAL
			OUTPUT "Enter account number:"
			INPUT AccountNumber
			OUTPUT "Enter deposit amount:"
			INPUT Amount
			CALL Deposit(AccountNumber, Amount)
		3:
			DECLARE AccountNumber:STRING
			DECLARE Amount : REAL
			OUTPUT "Enter account number:"
			INPUT AccountNumber
			OUTPUT "Enter withdrawal amount:"
			INPUT Amount
			CALL Withdraw(AccountNumber, Amount)
		4:
			DECLARE AccountNumber:STRING
			OUTPUT "Enter account number:"
			INPUT AccountNumber
			CALL CheckBalance(AccountNumber)
		5:
			OUTPUT "Thank you for using the Simple Banking System. Goodbye!"
		OTHERWISE:
			OUTPUT "Invalid choice. Please try again."
	ENDCASE
UNTIL Choice = 5
```
