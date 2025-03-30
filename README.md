# BrainFck Interpretor and Compiler 


## Compiler Dependencies (Not required for interpretor) 
* ### x86_64 Macos and Linux 
  - Nasm
  - GNU Linker (ld) 
* ### Arm64 Linux
  - GNU assembler
  - GNU Linker (ld)
* ### x86_64 Windows
  - Nasm
  - gcc

## Getting Started
### Clone the Repo
```sh 
https://github.com/Brady-Brandt/brainfck.git
```
### Create the bin folder 
```sh
mkdir bin
```
### Run Make (Uses gcc) 
```sh
make 
```

## Usage 
### Interpretor 
hello.bf
```sh
>++++++++[<+++++++++>-]<.>++++[<+++++++>-]<+.+++++++..+++.>>++++++[<+++++++>-]<+
+.------------.>++++++[<+++++++++>-]<+.<.+++.------.--------.>>>++++[<++++++++>-
]<+.
```
```sh
bin/brainfck hello.bf
```
Output 
```sh
Hello, World!
```
### Instruction Shorthand 
Repeated Instructions can be replaced with a number followed by the instruction. This doesn't work for the '[' and ']' instructions. 

The Hello World program above can be simplified to: 
```sh
>8+[<9+>-]<.>4+[<7+>-]<+.7+..3+.>>6+[<7+>-]<+
+.12-.>6+[<9+>-]<+.<.3+.6-.8-.3>4+[<8+>-
]<+.
```

### Compiler 
To use the compiler make sure you are using one of the supported platforms above and have the correct dependencies installed. 

### To activate the compiler use the -c or the -o flag 

```sh
bin/brainfck -c test.bf
```
Produces an executable file a.out 
### Specify Output File -o flag
```sh
bin/brainfck -o test test.bf
```


