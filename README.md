# POSIX-Based Shell with Pipelines, Redirection, and Job Control

A lightweight UNIX-style shell written in C, built to explore how real shells like bash and zsh work under the hood.  
The goal was to replicate core shell behavior using low-level system calls, without relying on existing frameworks.

---

## Overview

This shell supports most of the features you would expect from a basic command interpreter:

* Running external programs
* Chained pipelines (`cmd1 | cmd2 | cmd3`)
* Input and output redirection (`<`, `>`, `>>`)
* Background execution (`&`)
* Built-in commands (`cd`, `pwd`, `exit`, `history`)
* Basic job control (`jobs`, `fg`)
* Command history and tab-completion using the GNU Readline library
* Signal handling so the shell remains responsive instead of exiting with Ctrl-C

Everything is implemented with standard POSIX system calls and minimal external dependencies.

---

## Key Concepts Demonstrated

* Process creation and replacement using `fork()` and `execvp()`
* Inter-process communication using anonymous pipes
* File descriptor routing with `dup2()` for redirection
* Synchronous and asynchronous process management with `waitpid()`
* Maintaining internal job tables for background processes
* Using the Readline library to provide history and line editing
* Parsing and tokenization of commands without external libraries

---

## Requirements

This project targets Linux-like environments and depends on the Readline development library.

Ubuntu / Debian:
```bash
sudo apt update
sudo apt install build-essential libreadline-dev
```

Fedora:
```bash
sudo dnf install readline-devel
```

Arch:
```bash
sudo pacman -S readline
```

macOS (Homebrew):
```bash
brew install readline
```

---

## Building

Clone and compile:

```bash
gcc superShell.c -o supershell -lreadline
```

If compilation succeeds, run:

```bash
./supershell
```

---

## Usage Examples

Basic commands:
```
ls
pwd
cd Downloads
```

Pipes:
```
ls -l | grep .c | wc -l
```

Redirection:
```
echo hello > note.txt
cat < note.txt
echo more >> note.txt
```

Background jobs:
```
sleep 5 &
jobs
fg
```
---

## Disclaimer

This shell is not intended to replace bash, zsh, or any production shell.  
It is a learning-focused implementation designed to demonstrate how core terminal functionality is built from the ground up.

Pull requests and extensions are welcome if you want to build on it.
