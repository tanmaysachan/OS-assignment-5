7500 // PC keyboard interface constants
7501 
7502 #define KBSTATP         0x64    // kbd controller status port(I)
7503 #define KBS_DIB         0x01    // kbd data in buffer
7504 #define KBDATAP         0x60    // kbd data port(I)
7505 
7506 #define NO              0
7507 
7508 #define SHIFT           (1<<0)
7509 #define CTL             (1<<1)
7510 #define ALT             (1<<2)
7511 
7512 #define CAPSLOCK        (1<<3)
7513 #define NUMLOCK         (1<<4)
7514 #define SCROLLLOCK      (1<<5)
7515 
7516 #define E0ESC           (1<<6)
7517 
7518 // Special keycodes
7519 #define KEY_HOME        0xE0
7520 #define KEY_END         0xE1
7521 #define KEY_UP          0xE2
7522 #define KEY_DN          0xE3
7523 #define KEY_LF          0xE4
7524 #define KEY_RT          0xE5
7525 #define KEY_PGUP        0xE6
7526 #define KEY_PGDN        0xE7
7527 #define KEY_INS         0xE8
7528 #define KEY_DEL         0xE9
7529 
7530 // C('A') == Control-A
7531 #define C(x) (x - '@')
7532 
7533 static uchar shiftcode[256] =
7534 {
7535   [0x1D] CTL,
7536   [0x2A] SHIFT,
7537   [0x36] SHIFT,
7538   [0x38] ALT,
7539   [0x9D] CTL,
7540   [0xB8] ALT
7541 };
7542 
7543 static uchar togglecode[256] =
7544 {
7545   [0x3A] CAPSLOCK,
7546   [0x45] NUMLOCK,
7547   [0x46] SCROLLLOCK
7548 };
7549 
7550 static uchar normalmap[256] =
7551 {
7552   NO,   0x1B, '1',  '2',  '3',  '4',  '5',  '6',  // 0x00
7553   '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
7554   'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 0x10
7555   'o',  'p',  '[',  ']',  '\n', NO,   'a',  's',
7556   'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 0x20
7557   '\'', '`',  NO,   '\\', 'z',  'x',  'c',  'v',
7558   'b',  'n',  'm',  ',',  '.',  '/',  NO,   '*',  // 0x30
7559   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
7560   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
7561   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
7562   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
7563   [0x9C] '\n',      // KP_Enter
7564   [0xB5] '/',       // KP_Div
7565   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7566   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7567   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7568   [0x97] KEY_HOME,  [0xCF] KEY_END,
7569   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7570 };
7571 
7572 static uchar shiftmap[256] =
7573 {
7574   NO,   033,  '!',  '@',  '#',  '$',  '%',  '^',  // 0x00
7575   '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
7576   'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 0x10
7577   'O',  'P',  '{',  '}',  '\n', NO,   'A',  'S',
7578   'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 0x20
7579   '"',  '~',  NO,   '|',  'Z',  'X',  'C',  'V',
7580   'B',  'N',  'M',  '<',  '>',  '?',  NO,   '*',  // 0x30
7581   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
7582   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
7583   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
7584   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
7585   [0x9C] '\n',      // KP_Enter
7586   [0xB5] '/',       // KP_Div
7587   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7588   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7589   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7590   [0x97] KEY_HOME,  [0xCF] KEY_END,
7591   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7592 };
7593 
7594 
7595 
7596 
7597 
7598 
7599 
7600 static uchar ctlmap[256] =
7601 {
7602   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
7603   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
7604   C('Q'),  C('W'),  C('E'),  C('R'),  C('T'),  C('Y'),  C('U'),  C('I'),
7605   C('O'),  C('P'),  NO,      NO,      '\r',    NO,      C('A'),  C('S'),
7606   C('D'),  C('F'),  C('G'),  C('H'),  C('J'),  C('K'),  C('L'),  NO,
7607   NO,      NO,      NO,      C('\\'), C('Z'),  C('X'),  C('C'),  C('V'),
7608   C('B'),  C('N'),  C('M'),  NO,      NO,      C('/'),  NO,      NO,
7609   [0x9C] '\r',      // KP_Enter
7610   [0xB5] C('/'),    // KP_Div
7611   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7612   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7613   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7614   [0x97] KEY_HOME,  [0xCF] KEY_END,
7615   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7616 };
7617 
7618 
7619 
7620 
7621 
7622 
7623 
7624 
7625 
7626 
7627 
7628 
7629 
7630 
7631 
7632 
7633 
7634 
7635 
7636 
7637 
7638 
7639 
7640 
7641 
7642 
7643 
7644 
7645 
7646 
7647 
7648 
7649 
