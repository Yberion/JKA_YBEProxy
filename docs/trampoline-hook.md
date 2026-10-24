---
tags: JKA_YBEProxy
---

# JKA_YBEProxy Trampoline hook

## 1 - Introduction

What we want to do is to create a trampoline hook of a function in Jedi Academy.

This mean that, the original function will call our own function and at the end of our function, it can call the original one if needed.

In Jedi Academy it's a bit easier as the engine (let's say `jampDed`) already load a library (`jampgame`). It mean that we don't need to inject a library ourself into the engine.

## 2 - How the proxy works

Before continuing, we need to explain how the proxy works.

> [!WARNING]
> Keep in mind that this is a simplification, so this isn't really  1:1 with the code.

### 2.1 - Original Workflow

Basically, this is how `jampDed` and `jampgame` are communicating.

[![](images/FtLl2BB.png)](images/FtLl2BB.png)

### 2.2 - Proxy Workflow

This diagram will only show how the proxy communicate with `jampDed` and `jampgame` and intercept calls in both way. It is not showing the hooks and runtime patches part.

[![](images/Yus587M.png)](images/Yus587M.png)

With the proxy I am able to intercept the `GAME_INIT` call coming from `jampDed` which allow me to load `jampgame` through the proxy and runtime patch either `jampDed` or `jampgame`.

## 3 - Define the function to hook

Here is the function we want to hook.
Let's assume we're on 32 bits, which is the case with original `jampDed` and `jampgame`.

```c=
int addFive(const int number) {
  return number + 5;
}
```

`MSVC x86` ASM https://godbolt.org/z/Kh5x9Wo8a

```asm=
push   ebp                      ; 55
mov    ebp,esp                  ; 8b ec
mov    eax,DWORD PTR [ebp+0x8]  ; 8b 45 08
add    eax,0x5                  ; 83 c0 05
pop    ebp                      ; 5d
ret                             ; c3
```

The function we are going to call in the trampoline is the following one:

```c=
int addOne(const int number) {
  const int addFiveResult = original_addFive(number);
  
  return addFiveResult + 1;
}
```

To achieve the creation of the trampoline there is some steps to follow.

### 3.1 - Get the original instruction length to save

Now that we have our original function, we want to execute our own function (`addOne`) and call the original one inside (`original_addFive`). For that we need to place a `jmp` (jump) at the start of `addFive`.

The length of the `jmp` instruction is 5 bytes.

```asm=
jmp 0x00000000                  ; e9 = jmp = 1
                                ; 0x00000000 = 4 
```

This mean that we will need at least 5 bytes at the top of the original function.

The address `0x00000000` will be the address of our own function, this way, when calling `addFive`, it will directly jump (`jmp`) to our own function.

If we check the asm code, we will see a problem:

```asm=
push   ebp                      ; 55 = 1
mov    ebp,esp                  ; 8b ec = 2
mov    eax,DWORD PTR [ebp+0x8]  ; 8b 45 08 = 3
                                ; total = 6
...
```

> [!WARNING]
> We have a total of 6 bytes since we **can't** cut in the middle of an instruction.

To solve the problem, instead of taking 5 bytes for the `jmp`, we will take 6 bytes.

### 3.2 - Save the original instructions

Now that we have the length (6 bytes) of the original bytes we have to replace, if we want to call the original function, we need to save those bytes.

For that, we need to allocate a memory block that will contain the 6 bytes and copy them inside this block.

[![](images/VN5dcvH.png)](images/VN5dcvH.png)

In fact, we need more than 6 bytes.

Remember, we want to jump back to the original function.
So we will have to continue where we stopped, which is 6 bytes after the start of `addFive`.

So for that, the allocated block will need to contain the 6 bytes + 5 bytes for a jump (`jmp`), we will see that later.

### 3.3 - Jump to `addOne` function

So, we saved the 6 bytes that we're going to replace, we can now place the `jmp` instruction at the top of `addFive`.

The asm will look like this:

```asm=
jmp    addOne                   ; addOne = address of the function
add    eax,0x5                  ; 83 c0 05
pop    ebp                      ; 5d
ret                             ; c3
```

[![](images/5qUO4ZG.png)](images/5qUO4ZG.png)

### 3.4 - Call the original function

Now that `addFive` directly jump to `addOne` we need a way to call the original `addFive` since we want the result of the call.

Remember, we allocated a block of memory that contain the **6 + 5 bytes**.

The address of this block is going to be `original_addFive`.

The last 5 bytes of this block are going to contain the `jmp` instruction that will continue to the start of the original function + 6 bytes.

Don't forget that we replaced the first 6 bytes with a jump to `addOne` and we don't want to execute it again.

[![](images/q5nUpJE.png)](images/q5nUpJE.png)

Here we go, if we call `addFive`, it will in fact do a `+6` (as it will call `addOne` and `addFive`).
