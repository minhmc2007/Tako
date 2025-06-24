![tako-logo](logo.png)


# 🐙 Tako Language

**Tako** is a small, flexible interpreted scripting language made for tiny operating systems, embedded devices, or just having fun.  
Created by [@minhmc2007](https://github.com/minhmc2007), Tako is designed to be minimal, hackable, and educational — written entirely in C with no dependencies (expect compiler).

---

## ✨ Features

- 🧠 C-style scripting: `set x = 10`, `add x 5`, `if x > 10`
- 📜 Human-readable syntax, easy to learn and write
- 🔁 Supports control flow: `if`, `loop`, `end`
- 🖨️ Powerful `print` system: strings, variables, combined
- ⚙️ Self-contained interpreter: `tako.c`
- ⚠️ (Optional) Barebones compiler (`compiler.c`) for ELF output — limited feature support

---

## 🔧 Syntax Example

```tako
# Set a variable
set x = 10

# Add to it
add x 5

# Conditional
if x > 10
  print "x is bigger than 10" x
end

# Looping
loop 3
  print "Looping..." x
  add x 1
end

# Final output
print "Final x:" x
```

---

🚀 Usage

🧠 Run Scripts (Recommended)

Compile and run using the interpreter:

gcc tako.c -o tako
./tako script.tako

🛠️ Compile to Native ELF Binary (⚠️ Experimental)

> ❗ The compiler (compiler.c) only supports very basic scripts. For real projects, use tako.c.



gcc compiler.c -o compiler
./compiler script.tako output_binary
chmod +x output_binary
./output_binary


---

📁 File Types

Extension	Description

.tako	Tako language source code
.asm	(Optional) assembly output
.o	ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped 


---


🐙 Why “Tako”?

“Tako” (たこ) means octopus in Japanese — small, smart, and flexible.
Just like this language: lightweight, expandable, and surprisingly useful 🐙.


---

👤 Author

Made by @minhmc2007

Part of the Blue Archive Linux / TinyDOS / OS dev journey!



---

🪪 License

MIT License — open source and hack-friendly.
