8250 // Shell.
8251 
8252 #include "types.h"
8253 #include "user.h"
8254 #include "fcntl.h"
8255 
8256 // Parsed command representation
8257 #define EXEC  1
8258 #define REDIR 2
8259 #define PIPE  3
8260 #define LIST  4
8261 #define BACK  5
8262 
8263 #define MAXARGS 10
8264 
8265 struct cmd {
8266   int type;
8267 };
8268 
8269 struct execcmd {
8270   int type;
8271   char *argv[MAXARGS];
8272   char *eargv[MAXARGS];
8273 };
8274 
8275 struct redircmd {
8276   int type;
8277   struct cmd *cmd;
8278   char *file;
8279   char *efile;
8280   int mode;
8281   int fd;
8282 };
8283 
8284 struct pipecmd {
8285   int type;
8286   struct cmd *left;
8287   struct cmd *right;
8288 };
8289 
8290 struct listcmd {
8291   int type;
8292   struct cmd *left;
8293   struct cmd *right;
8294 };
8295 
8296 struct backcmd {
8297   int type;
8298   struct cmd *cmd;
8299 };
8300 int fork1(void);  // Fork but panics on failure.
8301 void panic(char*);
8302 struct cmd *parsecmd(char*);
8303 
8304 // Execute cmd.  Never returns.
8305 void
8306 runcmd(struct cmd *cmd)
8307 {
8308   int p[2];
8309   struct backcmd *bcmd;
8310   struct execcmd *ecmd;
8311   struct listcmd *lcmd;
8312   struct pipecmd *pcmd;
8313   struct redircmd *rcmd;
8314 
8315   if(cmd == 0)
8316     exit();
8317 
8318   switch(cmd->type){
8319   default:
8320     panic("runcmd");
8321 
8322   case EXEC:
8323     ecmd = (struct execcmd*)cmd;
8324     if(ecmd->argv[0] == 0)
8325       exit();
8326     exec(ecmd->argv[0], ecmd->argv);
8327     printf(2, "exec %s failed\n", ecmd->argv[0]);
8328     break;
8329 
8330   case REDIR:
8331     rcmd = (struct redircmd*)cmd;
8332     close(rcmd->fd);
8333     if(open(rcmd->file, rcmd->mode) < 0){
8334       printf(2, "open %s failed\n", rcmd->file);
8335       exit();
8336     }
8337     runcmd(rcmd->cmd);
8338     break;
8339 
8340   case LIST:
8341     lcmd = (struct listcmd*)cmd;
8342     if(fork1() == 0)
8343       runcmd(lcmd->left);
8344     wait();
8345     runcmd(lcmd->right);
8346     break;
8347 
8348 
8349 
8350   case PIPE:
8351     pcmd = (struct pipecmd*)cmd;
8352     if(pipe(p) < 0)
8353       panic("pipe");
8354     if(fork1() == 0){
8355       close(1);
8356       dup(p[1]);
8357       close(p[0]);
8358       close(p[1]);
8359       runcmd(pcmd->left);
8360     }
8361     if(fork1() == 0){
8362       close(0);
8363       dup(p[0]);
8364       close(p[0]);
8365       close(p[1]);
8366       runcmd(pcmd->right);
8367     }
8368     close(p[0]);
8369     close(p[1]);
8370     wait();
8371     wait();
8372     break;
8373 
8374   case BACK:
8375     bcmd = (struct backcmd*)cmd;
8376     if(fork1() == 0)
8377       runcmd(bcmd->cmd);
8378     break;
8379   }
8380   exit();
8381 }
8382 
8383 int
8384 getcmd(char *buf, int nbuf)
8385 {
8386   printf(2, "$ ");
8387   memset(buf, 0, nbuf);
8388   gets(buf, nbuf);
8389   if(buf[0] == 0) // EOF
8390     return -1;
8391   return 0;
8392 }
8393 
8394 
8395 
8396 
8397 
8398 
8399 
8400 int
8401 main(void)
8402 {
8403   static char buf[100];
8404   int fd;
8405 
8406   // Ensure that three file descriptors are open.
8407   while((fd = open("console", O_RDWR)) >= 0){
8408     if(fd >= 3){
8409       close(fd);
8410       break;
8411     }
8412   }
8413 
8414   // Read and run input commands.
8415   while(getcmd(buf, sizeof(buf)) >= 0){
8416     if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
8417       // Chdir must be called by the parent, not the child.
8418       buf[strlen(buf)-1] = 0;  // chop \n
8419       if(chdir(buf+3) < 0)
8420         printf(2, "cannot cd %s\n", buf+3);
8421       continue;
8422     }
8423     if(fork1() == 0)
8424       runcmd(parsecmd(buf));
8425     wait();
8426   }
8427   exit();
8428 }
8429 
8430 void
8431 panic(char *s)
8432 {
8433   printf(2, "%s\n", s);
8434   exit();
8435 }
8436 
8437 int
8438 fork1(void)
8439 {
8440   int pid;
8441 
8442   pid = fork();
8443   if(pid == -1)
8444     panic("fork");
8445   return pid;
8446 }
8447 
8448 
8449 
8450 // Constructors
8451 
8452 struct cmd*
8453 execcmd(void)
8454 {
8455   struct execcmd *cmd;
8456 
8457   cmd = malloc(sizeof(*cmd));
8458   memset(cmd, 0, sizeof(*cmd));
8459   cmd->type = EXEC;
8460   return (struct cmd*)cmd;
8461 }
8462 
8463 struct cmd*
8464 redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
8465 {
8466   struct redircmd *cmd;
8467 
8468   cmd = malloc(sizeof(*cmd));
8469   memset(cmd, 0, sizeof(*cmd));
8470   cmd->type = REDIR;
8471   cmd->cmd = subcmd;
8472   cmd->file = file;
8473   cmd->efile = efile;
8474   cmd->mode = mode;
8475   cmd->fd = fd;
8476   return (struct cmd*)cmd;
8477 }
8478 
8479 struct cmd*
8480 pipecmd(struct cmd *left, struct cmd *right)
8481 {
8482   struct pipecmd *cmd;
8483 
8484   cmd = malloc(sizeof(*cmd));
8485   memset(cmd, 0, sizeof(*cmd));
8486   cmd->type = PIPE;
8487   cmd->left = left;
8488   cmd->right = right;
8489   return (struct cmd*)cmd;
8490 }
8491 
8492 
8493 
8494 
8495 
8496 
8497 
8498 
8499 
8500 struct cmd*
8501 listcmd(struct cmd *left, struct cmd *right)
8502 {
8503   struct listcmd *cmd;
8504 
8505   cmd = malloc(sizeof(*cmd));
8506   memset(cmd, 0, sizeof(*cmd));
8507   cmd->type = LIST;
8508   cmd->left = left;
8509   cmd->right = right;
8510   return (struct cmd*)cmd;
8511 }
8512 
8513 struct cmd*
8514 backcmd(struct cmd *subcmd)
8515 {
8516   struct backcmd *cmd;
8517 
8518   cmd = malloc(sizeof(*cmd));
8519   memset(cmd, 0, sizeof(*cmd));
8520   cmd->type = BACK;
8521   cmd->cmd = subcmd;
8522   return (struct cmd*)cmd;
8523 }
8524 // Parsing
8525 
8526 char whitespace[] = " \t\r\n\v";
8527 char symbols[] = "<|>&;()";
8528 
8529 int
8530 gettoken(char **ps, char *es, char **q, char **eq)
8531 {
8532   char *s;
8533   int ret;
8534 
8535   s = *ps;
8536   while(s < es && strchr(whitespace, *s))
8537     s++;
8538   if(q)
8539     *q = s;
8540   ret = *s;
8541   switch(*s){
8542   case 0:
8543     break;
8544   case '|':
8545   case '(':
8546   case ')':
8547   case ';':
8548   case '&':
8549   case '<':
8550     s++;
8551     break;
8552   case '>':
8553     s++;
8554     if(*s == '>'){
8555       ret = '+';
8556       s++;
8557     }
8558     break;
8559   default:
8560     ret = 'a';
8561     while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
8562       s++;
8563     break;
8564   }
8565   if(eq)
8566     *eq = s;
8567 
8568   while(s < es && strchr(whitespace, *s))
8569     s++;
8570   *ps = s;
8571   return ret;
8572 }
8573 
8574 int
8575 peek(char **ps, char *es, char *toks)
8576 {
8577   char *s;
8578 
8579   s = *ps;
8580   while(s < es && strchr(whitespace, *s))
8581     s++;
8582   *ps = s;
8583   return *s && strchr(toks, *s);
8584 }
8585 
8586 
8587 
8588 
8589 
8590 
8591 
8592 
8593 
8594 
8595 
8596 
8597 
8598 
8599 
8600 struct cmd *parseline(char**, char*);
8601 struct cmd *parsepipe(char**, char*);
8602 struct cmd *parseexec(char**, char*);
8603 struct cmd *nulterminate(struct cmd*);
8604 
8605 struct cmd*
8606 parsecmd(char *s)
8607 {
8608   char *es;
8609   struct cmd *cmd;
8610 
8611   es = s + strlen(s);
8612   cmd = parseline(&s, es);
8613   peek(&s, es, "");
8614   if(s != es){
8615     printf(2, "leftovers: %s\n", s);
8616     panic("syntax");
8617   }
8618   nulterminate(cmd);
8619   return cmd;
8620 }
8621 
8622 struct cmd*
8623 parseline(char **ps, char *es)
8624 {
8625   struct cmd *cmd;
8626 
8627   cmd = parsepipe(ps, es);
8628   while(peek(ps, es, "&")){
8629     gettoken(ps, es, 0, 0);
8630     cmd = backcmd(cmd);
8631   }
8632   if(peek(ps, es, ";")){
8633     gettoken(ps, es, 0, 0);
8634     cmd = listcmd(cmd, parseline(ps, es));
8635   }
8636   return cmd;
8637 }
8638 
8639 
8640 
8641 
8642 
8643 
8644 
8645 
8646 
8647 
8648 
8649 
8650 struct cmd*
8651 parsepipe(char **ps, char *es)
8652 {
8653   struct cmd *cmd;
8654 
8655   cmd = parseexec(ps, es);
8656   if(peek(ps, es, "|")){
8657     gettoken(ps, es, 0, 0);
8658     cmd = pipecmd(cmd, parsepipe(ps, es));
8659   }
8660   return cmd;
8661 }
8662 
8663 struct cmd*
8664 parseredirs(struct cmd *cmd, char **ps, char *es)
8665 {
8666   int tok;
8667   char *q, *eq;
8668 
8669   while(peek(ps, es, "<>")){
8670     tok = gettoken(ps, es, 0, 0);
8671     if(gettoken(ps, es, &q, &eq) != 'a')
8672       panic("missing file for redirection");
8673     switch(tok){
8674     case '<':
8675       cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
8676       break;
8677     case '>':
8678       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
8679       break;
8680     case '+':  // >>
8681       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
8682       break;
8683     }
8684   }
8685   return cmd;
8686 }
8687 
8688 
8689 
8690 
8691 
8692 
8693 
8694 
8695 
8696 
8697 
8698 
8699 
8700 struct cmd*
8701 parseblock(char **ps, char *es)
8702 {
8703   struct cmd *cmd;
8704 
8705   if(!peek(ps, es, "("))
8706     panic("parseblock");
8707   gettoken(ps, es, 0, 0);
8708   cmd = parseline(ps, es);
8709   if(!peek(ps, es, ")"))
8710     panic("syntax - missing )");
8711   gettoken(ps, es, 0, 0);
8712   cmd = parseredirs(cmd, ps, es);
8713   return cmd;
8714 }
8715 
8716 struct cmd*
8717 parseexec(char **ps, char *es)
8718 {
8719   char *q, *eq;
8720   int tok, argc;
8721   struct execcmd *cmd;
8722   struct cmd *ret;
8723 
8724   if(peek(ps, es, "("))
8725     return parseblock(ps, es);
8726 
8727   ret = execcmd();
8728   cmd = (struct execcmd*)ret;
8729 
8730   argc = 0;
8731   ret = parseredirs(ret, ps, es);
8732   while(!peek(ps, es, "|)&;")){
8733     if((tok=gettoken(ps, es, &q, &eq)) == 0)
8734       break;
8735     if(tok != 'a')
8736       panic("syntax");
8737     cmd->argv[argc] = q;
8738     cmd->eargv[argc] = eq;
8739     argc++;
8740     if(argc >= MAXARGS)
8741       panic("too many args");
8742     ret = parseredirs(ret, ps, es);
8743   }
8744   cmd->argv[argc] = 0;
8745   cmd->eargv[argc] = 0;
8746   return ret;
8747 }
8748 
8749 
8750 // NUL-terminate all the counted strings.
8751 struct cmd*
8752 nulterminate(struct cmd *cmd)
8753 {
8754   int i;
8755   struct backcmd *bcmd;
8756   struct execcmd *ecmd;
8757   struct listcmd *lcmd;
8758   struct pipecmd *pcmd;
8759   struct redircmd *rcmd;
8760 
8761   if(cmd == 0)
8762     return 0;
8763 
8764   switch(cmd->type){
8765   case EXEC:
8766     ecmd = (struct execcmd*)cmd;
8767     for(i=0; ecmd->argv[i]; i++)
8768       *ecmd->eargv[i] = 0;
8769     break;
8770 
8771   case REDIR:
8772     rcmd = (struct redircmd*)cmd;
8773     nulterminate(rcmd->cmd);
8774     *rcmd->efile = 0;
8775     break;
8776 
8777   case PIPE:
8778     pcmd = (struct pipecmd*)cmd;
8779     nulterminate(pcmd->left);
8780     nulterminate(pcmd->right);
8781     break;
8782 
8783   case LIST:
8784     lcmd = (struct listcmd*)cmd;
8785     nulterminate(lcmd->left);
8786     nulterminate(lcmd->right);
8787     break;
8788 
8789   case BACK:
8790     bcmd = (struct backcmd*)cmd;
8791     nulterminate(bcmd->cmd);
8792     break;
8793   }
8794   return cmd;
8795 }
8796 
8797 
8798 
8799 
