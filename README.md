# LyHookLib
This is a kernel hook library developed based on HookLib: https://github.com/HoShiMin/HookLib , aimed at solving some of its own problems. As the author of HookLib is no longer updating, I hope to improve it through my own efforts.

### Why am I doing this:
When I use the ept hook, I first use HookLib to create an inline hook, and then install the ept to take over read and write behavior, and return the physical page that was not Hooked to the guest.When the address interval between the handler and the api exceeds+-2gb, HookLib uses jmp [rip] to jump to the Handle position. So this caused a problem in the ept because I would return a physical page without a Hook to the guest, so at this point, the value read from the rip in jmp [rip] is incorrect, so it will jump to a strange position, triggering bsod.

### How did I fix this error:
Replace the original jmp [rip] instruction with assembly code like this:
```
typedef struct
{
    unsigned char opcode1[1];               // 50                       | push rax
    unsigned char opcode2[2];               // 48 b8                    | mov rax, ?
    unsigned long long address;             // 00 00 00 00 00 00 00 00  | 
    unsigned char opcode3[4];               // 48 87 04 24              | xchg QWORD PTR[rsp], rax
    unsigned char opcode4[1];               // c3                       | ret
} PushRet;
```

### Support relative addresses:
1.mov reg,[address]
```
nt!NtSetContextThread:
fffff806`2e0ca340 4c8bdc          mov     r11,rsp
fffff806`2e0ca343 49895b18        mov     qword ptr [r11+18h],rbx
fffff806`2e0ca347 55              push    rbp
fffff806`2e0ca348 56              push    rsi
fffff806`2e0ca349 57              push    rdi
fffff806`2e0ca34a 4883ec60        sub     rsp,60h
fffff806`2e0ca34e 488b052b10b6ff  mov     rax,qword ptr [nt!_security_cookie (fffff806`2dc2b380)] 
fffff806`2e0ca355 4833c4          xor     rax,rsp


; mov rax,qword ptr [nt!_security_cookie (fffff806`2dc2b380)] , will be replaced as:
; ------------
; mov    rax,0xfffff806`2dc2b380
; mov    rax,[rax];

```
2.je address
```
nt!KeYieldExecution:
fffff805`630ff830 4055            push    rbp
fffff805`630ff832 4883ec40        sub     rsp,40h
fffff805`630ff836 8be9            mov     ebp,ecx
fffff805`630ff838 f7c1feffffff    test    ecx,0FFFFFFFEh
fffff805`630ff83e 740b            je      nt!KeYieldExecution+0x1b (fffff805`630ff84b)
fffff805`630ff840 b80d0000c0      mov     eax,0C000000Dh
fffff805`630ff845 4883c440        add     rsp,40h
fffff805`630ff849 5d              pop     rbp

; je nt!KeYieldExecution+0x1b (fffff805`630ff84b) , will be replaced as:
; ------------------
;     je jmp_y
;     jmp jmp_n
; jmp_y:
;	  push rax
;	  mov rax, fffff805`630ff84b
;	  xchg QWORD PTR[rsp], rax
;	  ret
; jmp_n:
;	  mov     eax,0C000000Dh
;	  add     rsp,40h
;	  pop     rbp
```

