# gptcom
__A command line AI client for Linux written C using the OpenAI API__

gptcom is a small, one file, command-line executable that accepts Gpt prompts and returns
text responses to the command shell.

All arguments typed after the command are input as the one prompt to Gpt.

When executed with no arguments a GUI text input box opens to accept and edit your typed
text input.

gptcom uses three environment variables:

- GPTKEY=your OpenAI API key
- GPTMOD=the OpenAI Gpt model name to use
- USER=your Linux user id

example:
```bash
export GPTMOD="gpt-4"
```

A running text log is kept: /home/$USER/.config/gptcom.log

__zenity__ must be installed on your system:  
```bash
sudo apt install zenity
```
  