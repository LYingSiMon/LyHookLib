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

### To do:
1.Fix Hook failure issue caused by API header having relative addresses
like this:https://github.com/HoShiMin/HookLib/issues/23

