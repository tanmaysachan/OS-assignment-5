8150 #include "syscall.h"
8151 #include "traps.h"
8152 
8153 #define SYSCALL(name) \
8154   .globl name; \
8155   name: \
8156     movl $SYS_ ## name, %eax; \
8157     int $T_SYSCALL; \
8158     ret
8159 
8160 SYSCALL(fork)
8161 SYSCALL(exit)
8162 SYSCALL(wait)
8163 SYSCALL(waitx)
8164 SYSCALL(pipe)
8165 SYSCALL(read)
8166 SYSCALL(write)
8167 SYSCALL(close)
8168 SYSCALL(kill)
8169 SYSCALL(exec)
8170 SYSCALL(open)
8171 SYSCALL(mknod)
8172 SYSCALL(unlink)
8173 SYSCALL(fstat)
8174 SYSCALL(link)
8175 SYSCALL(mkdir)
8176 SYSCALL(chdir)
8177 SYSCALL(dup)
8178 SYSCALL(getpid)
8179 SYSCALL(sbrk)
8180 SYSCALL(sleep)
8181 SYSCALL(uptime)
8182 SYSCALL(setpriority)
8183 
8184 
8185 
8186 
8187 
8188 
8189 
8190 
8191 
8192 
8193 
8194 
8195 
8196 
8197 
8198 
8199 
