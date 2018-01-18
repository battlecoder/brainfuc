/* #########################################################################
 * ## BRAINFUC                                                            ##
 * ##                                                                     ##
 * ## Simple implementation of Brainfuck for Arduino (AVR uC).            ##
 * ##                                                                     ## 
 * ## Adds 2 instructions for General Purpose I/O:                        ##
 * ##   : (colon)      Outputs value of current memory cell to digital    ##
 * ##                  pins 2 to 9.                                       ##
 * ##   ; (semi-colon) Reads the binary value of pins 2 to 9 and stores   ##
 * ##                  the result in the current memory cell.             ##
 * ##                                                                     ##
 * #########################################################################
 * ## version 0.1: First public release.                                  ##
 * ######################################################################### */

/* #########################################################################
 * ## BF RAM functions                                                    ##
 * ######################################################################### */
// Implement your RAM functions here. Using an external RAM will give you
// plenty of space for most programs.
// For testing purposes we will use a 1KB in-memory array.
const int     RAM_SIZE = 1024;
volatile byte RAM[RAM_SIZE];

byte bf_mem_get (word addr){
  if (addr >= RAM_SIZE) {
    Serial.print(F("WARNING: Attempt to read from invalid RAM address :0x"));
    Serial.println(addr, HEX);
    return 0;
  }
  return RAM[addr];
}

void bf_mem_set (word addr, byte v){ 
  if (addr >= RAM_SIZE) {
    Serial.print(F("WARNING: Attempt to write to invalid RAM address :0x"));
    Serial.println(addr, HEX);
    return;
  }
  RAM[addr] = v;
}

void bf_mem_clear (){ 
  memset((void *)RAM, 0, RAM_SIZE-1);
}

/* #########################################################################
 * ## Jump Stack functions                                                ##
 * ######################################################################### */
// This stack will be used to store jump-back instructions. Since a jump-forward
// due to a [ instruction may occur without going past the matching ] we can't rely
// on an stack for that, and manual scanning will be used for those cases.
const byte     BF_STACK_DEPTH = 64;
volatile byte  bf_lpstackptr  = 0;
volatile word  bf_lpstack[BF_STACK_DEPTH];

void bf_lpstack_push(word addr) {
  if (bf_lpstackptr >= BF_STACK_DEPTH - 1) {
    Serial.println(F("ERROR: Stack overflow! Couldn't push new loop-point jump address"));
    exit;
  }
  bf_lpstack[bf_lpstackptr] = addr;
  bf_lpstackptr++;
}

word bf_lpstack_pop() {
  if (bf_lpstackptr >= BF_STACK_DEPTH - 1) {
    Serial.println(F("ERROR: Stack underflow! Couldn't retrieve loop-point jump address"));
    exit;
  }
  bf_lpstackptr--;
  return bf_lpstack[bf_lpstackptr];
}

word bf_lpstack_peek() {
  if (bf_lpstackptr < 1) {
    Serial.println(F("ERROR: loop-point stack PEEK with no elements in it!"));
    exit;
  }
  return bf_lpstack[bf_lpstackptr-1];
}

/* #########################################################################
 * ## BRAINFUCK CODE                                                      ##
 * ######################################################################### */
// This example uses PROGMEM to store the code so we don't waste RAM
const char PROGMEM DEMO_BF_CODE[] = "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.";

// The BF implementation will call bf_code_at() constantly to read instructions.
// Since we stored our code in PROGMEM we have to use pgm_read_byte_near() to
// read it.
inline char bf_code_at(word addr) {
  return (char)pgm_read_byte_near(DEMO_BF_CODE + addr);
}

/* #########################################################################
 * ## Brainfuck implementation                                            ##
 * ######################################################################### */
volatile word bf_memptr  = 0;
volatile word bf_codeptr = 0;

bool bf_parse_next () {
  byte b;
  char c, instr;
  word p, bracketcount;

  instr = bf_code_at(bf_codeptr);
  /* Apparently a bunch of IFs is slightly faster than a
   *  switch() statement. Go figure. */
  if (instr == 0) return false;

  if (instr == '>') {
      bf_memptr++;

  }else if (instr == '<') {
      bf_memptr--;

  } else if (instr == '+') {
    b = bf_mem_get(bf_memptr);
    bf_mem_set(bf_memptr, b + 1);

  } else if (instr == '-') {
    b = bf_mem_get(bf_memptr);
    bf_mem_set(bf_memptr, b - 1);

  } else if(instr == '[') {
    if (bf_mem_get(bf_memptr) == 0) {
      // Jump past matching ]
      bracketcount = 0;
      p = bf_codeptr+1;
      while ((c = bf_code_at(p)) != 0) {
        if (c == '[') {
          bracketcount++;
        }else if (c == ']') {
          if (bracketcount == 0) break;
          bracketcount--;
        }
        p++;
      }
      if (bracketcount > 0 || c != ']'){
        Serial.println(F("WARNING: Matching ] not found for [ at address 0x"));
        Serial.println(bf_codeptr);
      } else{
        bf_codeptr = p;
      }
    }else {
      bf_lpstack_push (bf_codeptr);
    }

  } else if(instr == ']') {
    if (bf_mem_get(bf_memptr) != 0) {
      bf_codeptr = bf_lpstack_peek();
    }else {
      bf_lpstack_pop();
    }

  } else if(instr == '.') {
    Serial.write((char)bf_mem_get(bf_memptr));

  } else if(instr == ',') {
    while (!Serial.available());
    b = Serial.read();
    bf_mem_set(bf_memptr, b);

  } else if(instr == ':') {
    b = bf_mem_get(bf_memptr);
    // We could use direct register manipulation for faster read/write to ports, but that is
    // pretty processor-dependent and would only work in AVR processors that follow the same
    // pin mapping. Brainfuck is already slow, so I guess it won't hurt to do this the slow
    // but more compatible way.
    for (c=2; c < 10; c++) pinMode (c, OUTPUT);
    for (c=2; c < 10; c++) {
      digitalWrite (c, b & 1);
      b >>= 1;
    }

  } else if(instr == ';') {
    // Same here...
    b = 0;
    for (c=2; c < 10; c++) pinMode (c, INPUT);
    for (c=2; c < 10; c++) {
      if (digitalRead(c)) b |= 1;
      b <<= 1;
    }
    bf_mem_set(bf_memptr, b);
  }
  bf_codeptr++;
  return true;
}

/* #########################################################################
 * ## SETUP                                                               ##
 * ######################################################################### */
volatile bool parse_ok;

void setup() {
  Serial.begin(9600);
  bf_mem_clear();
  parse_ok = true;
}

/* #########################################################################
 * ## LOOP                                                                ##
 * ######################################################################### */
void loop() {
  // This will parse until no more instructions are left.
  if (parse_ok) parse_ok = bf_parse_next ();
}
